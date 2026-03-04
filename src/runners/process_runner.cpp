#include "runners/process_runner.hpp"

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace noc::runners {
namespace {

void replace_all(std::string& target, const std::string& needle, const std::string& replacement) {
    std::size_t position = 0;
    while ((position = target.find(needle, position)) != std::string::npos) {
        target.replace(position, needle.size(), replacement);
        position += replacement.size();
    }
}

std::string read_pipe(FILE* pipe) {
    std::array<char, 4096> buffer{};
    std::ostringstream output;
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output << buffer.data();
    }
    return output.str();
}

}  // namespace

core::ExecutionResult ProcessRunner::execute(
    const core::LanguageSpec& spec,
    const core::ExecutionRequest& request,
    const std::filesystem::path& session_dir) const {
    std::error_code ec;
    std::filesystem::create_directories(session_dir, ec);

    const auto source_path = session_dir / spec.source_name;
    {
        std::ofstream output(source_path, std::ios::binary);
        output << request.code;
    }

    core::ExecutionResult result;
    result.session_id = request.session_id;
    result.language = request.language;

    if (spec.kind == core::LanguageKind::Preview) {
        result.success = true;
        result.exit_code = 0;
        result.output = request.code;
        result.artifact_path = source_path.string();
        return result;
    }

#ifdef _WIN32
    const auto binary_name = std::string("program.exe");
#else
    const auto binary_name = std::string("program.out");
#endif
    const auto binary_path = session_dir / binary_name;

    std::string combined_output;

    if (!spec.compile_command.empty()) {
        int compile_exit = -1;
        const auto compile_command = resolve_template(spec.compile_command, source_path, binary_path, session_dir);
        const auto compile_output = capture_command(compile_command, session_dir, compile_exit);
        if (!compile_output.empty()) {
            combined_output += compile_output;
            if (combined_output.back() != '\n') {
                combined_output += '\n';
            }
        }
        if (compile_exit != 0) {
            result.success = false;
            result.exit_code = compile_exit;
            result.error = compile_output.empty() ? "Compilation failed." : compile_output;
            result.output = combined_output;
            return result;
        }
    }

    int run_exit = -1;
    const auto run_command = resolve_template(spec.run_command, source_path, binary_path, session_dir);
    const auto run_output = capture_command(run_command, session_dir, run_exit);
    combined_output += run_output;

    result.success = (run_exit == 0);
    result.exit_code = run_exit;
    result.output = combined_output;
    result.error = result.success ? std::string{} : (run_output.empty() ? "Execution failed." : run_output);
    result.artifact_path = binary_path.string();
    return result;
}

std::string ProcessRunner::capture_command(const std::string& command, const std::filesystem::path& working_dir, int& exit_code) {
    std::error_code ec;
    const auto previous_dir = std::filesystem::current_path(ec);
    std::filesystem::current_path(working_dir, ec);

#ifdef _WIN32
    FILE* pipe = _popen((command + " 2>&1").c_str(), "r");
#else
    FILE* pipe = popen((command + " 2>&1").c_str(), "r");
#endif
    if (pipe == nullptr) {
        std::filesystem::current_path(previous_dir, ec);
        exit_code = -1;
        return "Unable to start process.";
    }

    const auto output = read_pipe(pipe);
#ifdef _WIN32
    exit_code = _pclose(pipe);
#else
    exit_code = pclose(pipe);
#endif

    std::filesystem::current_path(previous_dir, ec);
    return output;
}

std::string ProcessRunner::resolve_template(
    const std::string& value,
    const std::filesystem::path& source_path,
    const std::filesystem::path& binary_path,
    const std::filesystem::path& working_dir) {
    std::string resolved = value;
    replace_all(resolved, "{source}", source_path.string());
    replace_all(resolved, "{binary}", binary_path.string());
    replace_all(resolved, "{workdir}", working_dir.string());
    return resolved;
}

}  // namespace noc::runners
