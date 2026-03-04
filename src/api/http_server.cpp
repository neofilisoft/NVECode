#include "api/http_server.hpp"

#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace noc::api {
namespace {

#ifdef _WIN32
using SocketHandle = SOCKET;
constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;
#else
using SocketHandle = int;
constexpr SocketHandle kInvalidSocket = -1;
#endif

struct HttpRequest {
    std::string method;
    std::string target;
    std::string body;
};

struct HttpResponse {
    int status = 200;
    std::string content_type = "application/json; charset=utf-8";
    std::string body;
};

void close_socket(SocketHandle socket_handle) {
#ifdef _WIN32
    closesocket(socket_handle);
#else
    close(socket_handle);
#endif
}

std::string status_text(int status) {
    switch (status) {
        case 200:
            return "OK";
        case 400:
            return "Bad Request";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        default:
            return "Internal Server Error";
    }
}

bool send_all(SocketHandle client, const std::string& payload) {
    std::size_t offset = 0;
    while (offset < payload.size()) {
#ifdef _WIN32
        const auto sent = send(client, payload.data() + static_cast<int>(offset), static_cast<int>(payload.size() - offset), 0);
#else
        const auto sent = send(client, payload.data() + offset, payload.size() - offset, 0);
#endif
        if (sent <= 0) {
            return false;
        }
        offset += static_cast<std::size_t>(sent);
    }
    return true;
}

std::optional<HttpRequest> read_request(SocketHandle client) {
    std::string buffer;
    std::array<char, 4096> chunk{};

    while (buffer.find("\r\n\r\n") == std::string::npos) {
#ifdef _WIN32
        const auto received = recv(client, chunk.data(), static_cast<int>(chunk.size()), 0);
#else
        const auto received = recv(client, chunk.data(), chunk.size(), 0);
#endif
        if (received <= 0) {
            return std::nullopt;
        }
        buffer.append(chunk.data(), static_cast<std::size_t>(received));
        if (buffer.size() > 2 * 1024 * 1024) {
            return std::nullopt;
        }
    }

    const auto header_end = buffer.find("\r\n\r\n");
    const auto headers = buffer.substr(0, header_end);
    std::size_t content_length = 0;

    std::istringstream header_stream(headers);
    std::string request_line;
    std::getline(header_stream, request_line);
    if (!request_line.empty() && request_line.back() == '\r') {
        request_line.pop_back();
    }

    std::string header_line;
    while (std::getline(header_stream, header_line)) {
        if (!header_line.empty() && header_line.back() == '\r') {
            header_line.pop_back();
        }
        const auto colon = header_line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        auto key = header_line.substr(0, colon);
        auto value = header_line.substr(colon + 1);
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
            value.erase(value.begin());
        }
        for (char& ch : key) {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        if (key == "content-length") {
            content_length = static_cast<std::size_t>(std::stoul(value));
        }
    }

    while (buffer.size() < header_end + 4 + content_length) {
#ifdef _WIN32
        const auto received = recv(client, chunk.data(), static_cast<int>(chunk.size()), 0);
#else
        const auto received = recv(client, chunk.data(), chunk.size(), 0);
#endif
        if (received <= 0) {
            return std::nullopt;
        }
        buffer.append(chunk.data(), static_cast<std::size_t>(received));
    }

    std::istringstream request_stream(request_line);
    HttpRequest request;
    request_stream >> request.method >> request.target;
    request.body = buffer.substr(header_end + 4, content_length);
    return request;
}

std::string escape_json(std::string_view input) {
    std::string output;
    output.reserve(input.size() + 16);
    for (const char ch : input) {
        switch (ch) {
            case '\\':
                output += "\\\\";
                break;
            case '"':
                output += "\\\"";
                break;
            case '\n':
                output += "\\n";
                break;
            case '\r':
                output += "\\r";
                break;
            case '\t':
                output += "\\t";
                break;
            default:
                output.push_back(ch);
                break;
        }
    }
    return output;
}

std::optional<std::string> read_json_string(const std::string& json, const std::string& key) {
    const auto token = std::string{"\""} + key + "\"";
    auto position = json.find(token);
    if (position == std::string::npos) {
        return std::nullopt;
    }

    position = json.find(':', position + token.size());
    if (position == std::string::npos) {
        return std::nullopt;
    }
    ++position;

    while (position < json.size() && std::isspace(static_cast<unsigned char>(json[position]))) {
        ++position;
    }
    if (position >= json.size() || json[position] != '"') {
        return std::nullopt;
    }
    ++position;

    std::string value;
    bool escaped = false;
    while (position < json.size()) {
        const char ch = json[position++];
        if (escaped) {
            switch (ch) {
                case 'n':
                    value.push_back('\n');
                    break;
                case 'r':
                    value.push_back('\r');
                    break;
                case 't':
                    value.push_back('\t');
                    break;
                default:
                    value.push_back(ch);
                    break;
            }
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') {
            return value;
        }
        value.push_back(ch);
    }

    return std::nullopt;
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::string query_value(std::string_view target, std::string_view key) {
    const auto query_start = target.find('?');
    if (query_start == std::string_view::npos) {
        return {};
    }

    const auto query = target.substr(query_start + 1);
    const auto token = std::string(key) + "=";
    std::size_t current = 0;

    while (current < query.size()) {
        const auto next = query.find('&', current);
        const auto pair = query.substr(current, next == std::string_view::npos ? query.size() - current : next - current);
        if (pair.substr(0, token.size()) == token) {
            return std::string(pair.substr(token.size()));
        }
        if (next == std::string_view::npos) {
            break;
        }
        current = next + 1;
    }

    return {};
}

HttpResponse json_response(std::string body, int status = 200) {
    return HttpResponse{status, "application/json; charset=utf-8", std::move(body)};
}

HttpResponse text_response(std::string body, std::string content_type, int status = 200) {
    return HttpResponse{status, std::move(content_type), std::move(body)};
}

HttpResponse route_request(const HttpRequest& request, core::IdeCore& core, const std::filesystem::path& web_root) {
    if (request.method == "GET" && (request.target == "/" || request.target == "/index.html")) {
        const auto html = read_file(web_root / "index.html");
        if (html.empty()) {
            return text_response("index.html not found", "text/plain; charset=utf-8", 404);
        }
        return text_response(html, "text/html; charset=utf-8");
    }

    if (request.method == "GET" && request.target == "/api/languages") {
        std::ostringstream payload;
        payload << "{\"success\":true,\"languages\":[";
        bool first = true;
        for (const auto& language : core.languages()) {
            if (!first) {
                payload << ',';
            }
            first = false;
            payload << "{\"id\":\"" << escape_json(language.id) << "\",\"name\":\"" << escape_json(language.display_name) << "\"}";
        }
        payload << "]}";
        return json_response(payload.str());
    }

    if (request.method == "GET" && request.target.rfind("/api/status", 0) == 0) {
        const auto session_id = query_value(request.target, "session_id");
        const auto current = core.status(session_id);
        if (!current.has_value()) {
            return json_response("{\"success\":false,\"error\":\"Session not found.\"}", 404);
        }

        std::ostringstream payload;
        payload << "{\"success\":true,\"session_id\":\"" << escape_json(current->session_id)
                << "\",\"language\":\"" << escape_json(current->language)
                << "\",\"state\":\"" << escape_json(current->state)
                << "\",\"exit_code\":" << current->exit_code
                << ",\"output\":\"" << escape_json(current->output)
                << "\",\"error\":\"" << escape_json(current->error)
                << "\",\"artifact_path\":\"" << escape_json(current->artifact_path) << "\"}";
        return json_response(payload.str());
    }

    if (request.method == "GET" && request.target.rfind("/preview", 0) == 0) {
        const auto session_id = query_value(request.target, "session_id");
        const auto preview = core.preview_path(session_id);
        if (preview.empty() || !std::filesystem::exists(preview)) {
            return text_response("Preview not found", "text/plain; charset=utf-8", 404);
        }
        return text_response(read_file(preview), "text/html; charset=utf-8");
    }

    if (request.method == "POST" && request.target == "/api/run") {
        const auto language = read_json_string(request.body, "language");
        const auto code = read_json_string(request.body, "code");
        if (!language.has_value() || !code.has_value()) {
            return json_response("{\"success\":false,\"error\":\"language and code are required.\"}", 400);
        }

        core::ExecutionRequest execution_request;
        execution_request.language = *language;
        execution_request.code = *code;
        execution_request.session_id = read_json_string(request.body, "session_id").value_or(std::string{});

        const auto result = core.run(std::move(execution_request));
        std::ostringstream payload;
        payload << "{\"success\":" << (result.success ? "true" : "false")
                << ",\"session_id\":\"" << escape_json(result.session_id)
                << "\",\"language\":\"" << escape_json(result.language)
                << "\",\"exit_code\":" << result.exit_code
                << ",\"output\":\"" << escape_json(result.output)
                << "\",\"error\":\"" << escape_json(result.error)
                << "\",\"artifact_path\":\"" << escape_json(result.artifact_path) << "\"}";
        return json_response(payload.str(), result.success ? 200 : 400);
    }

    if (request.method == "POST" && request.target == "/api/stop") {
        const auto session_id = read_json_string(request.body, "session_id");
        if (!session_id.has_value()) {
            return json_response("{\"success\":false,\"error\":\"session_id is required.\"}", 400);
        }

        const auto result = core.stop(*session_id);
        std::ostringstream payload;
        payload << "{\"success\":false,\"session_id\":\"" << escape_json(result.session_id)
                << "\",\"error\":\"" << escape_json(result.error) << "\"}";
        return json_response(payload.str(), 400);
    }

    if (request.method == "GET" && request.target == "/health") {
        return json_response("{\"success\":true,\"service\":\"NOCompiler\"}");
    }

    return json_response("{\"success\":false,\"error\":\"Route not found.\"}", 404);
}

}  // namespace

HttpServer::HttpServer(core::IdeCore& core, std::filesystem::path web_root, std::string host, unsigned short port)
    : core_(core), web_root_(std::move(web_root)), host_(std::move(host)), port_(port) {}

bool HttpServer::run() {
    (void)host_;

#ifdef _WIN32
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        return false;
    }
#endif

    SocketHandle server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server == kInvalidSocket) {
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port_);
    address.sin_addr.s_addr = INADDR_ANY;

    const int reuse = 1;
#ifdef _WIN32
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
#else
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif

    if (bind(server, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
        close_socket(server);
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    if (listen(server, 16) != 0) {
        close_socket(server);
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    while (true) {
        sockaddr_in client_address{};
#ifdef _WIN32
        int client_size = sizeof(client_address);
#else
        socklen_t client_size = sizeof(client_address);
#endif
        const auto client = accept(server, reinterpret_cast<sockaddr*>(&client_address), &client_size);
        if (client == kInvalidSocket) {
            continue;
        }

        const auto request = read_request(client);
        if (!request.has_value()) {
            close_socket(client);
            continue;
        }

        const auto response = route_request(*request, core_, web_root_);
        std::ostringstream payload;
        payload << "HTTP/1.1 " << response.status << ' ' << status_text(response.status) << "\r\n"
                << "Content-Type: " << response.content_type << "\r\n"
                << "Content-Length: " << response.body.size() << "\r\n"
                << "Connection: close\r\n\r\n"
                << response.body;
        send_all(client, payload.str());
        close_socket(client);
    }

    return true;
}

}  // namespace noc::api
