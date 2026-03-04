#include "core/ide_core.hpp"

#include <chrono>
#include <filesystem>
#include <system_error>

namespace noc::core {

IdeCore::IdeCore()
    : languages_{
          {"python", "Python", ".py", "main.py", LanguageKind::Script, "", "python -u \"{source}\""},
          {"c", "C", ".c", "main.c", LanguageKind::Compiled, "gcc \"{source}\" -O2 -o \"{binary}\"", "\"{binary}\""},
          {"cpp", "C++", ".cpp", "main.cpp", LanguageKind::Compiled, "g++ \"{source}\" -std=c++20 -O2 -o \"{binary}\"", "\"{binary}\""},
          {"csharp", "C#", ".cs", "Program.cs", LanguageKind::Compiled, "csc /nologo /out:\"{binary}\" \"{source}\"", "\"{binary}\""},
          {"java", "Java", ".java", "Main.java", LanguageKind::Compiled, "javac \"{source}\"", "java -cp \"{workdir}\" Main"},
          {"javascript", "JavaScript", ".js", "main.js", LanguageKind::Script, "", "node \"{source}\""},
          {"html", "HTML", ".html", "index.html", LanguageKind::Preview, "", ""},
          {"rust", "Rust", ".rs", "main.rs", LanguageKind::Compiled, "rustc \"{source}\" -o \"{binary}\"", "\"{binary}\""},
          {"lua", "Lua", ".lua", "main.lua", LanguageKind::Script, "", "lua \"{source}\""},
          {"zig", "Zig", ".zig", "main.zig", LanguageKind::Script, "", "zig run \"{source}\""},
          {"scala", "Scala", ".scala", "Main.scala", LanguageKind::Script, "", "scala \"{source}\""},
          {"ruby", "Ruby", ".rb", "main.rb", LanguageKind::Script, "", "ruby \"{source}\""},
          {"go", "Go", ".go", "main.go", LanguageKind::Script, "", "go run \"{source}\""},
          {"php", "PHP", ".php", "main.php", LanguageKind::Script, "", "php \"{source}\""},
      } {
    workspace_root_ = std::filesystem::current_path() / ".nocompiler_sessions";
    std::error_code ec;
    std::filesystem::create_directories(workspace_root_, ec);
}

const std::vector<LanguageSpec>& IdeCore::languages() const noexcept {
    return languages_;
}

ExecutionResult IdeCore::run(ExecutionRequest request) {
    if (request.session_id.empty()) {
        request.session_id = make_session_id();
    }

    const auto* spec = find_language(request.language);
    if (spec == nullptr) {
        ExecutionResult result;
        result.session_id = request.session_id;
        result.language = request.language;
        result.error = "Unsupported language.";
        return result;
    }

    {
        std::scoped_lock lock(mutex_);
        sessions_[request.session_id] = SessionRecord{request.session_id, request.language, "running", -1, "", "", ""};
    }

    const auto result = runner_.execute(*spec, request, ensure_session_dir(request.session_id));
    store_result(result, result.success ? (spec->kind == LanguageKind::Preview ? "preview-ready" : "completed") : "failed");
    return result;
}

std::optional<SessionRecord> IdeCore::status(const std::string& session_id) const {
    std::scoped_lock lock(mutex_);
    const auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return std::nullopt;
    }
    return it->second;
}

ExecutionResult IdeCore::stop(const std::string& session_id) {
    ExecutionResult result;
    result.session_id = session_id;
    result.error = "Stop is not available in synchronous mode.";

    std::scoped_lock lock(mutex_);
    const auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        it->second.state = "stop-unavailable";
        it->second.error = result.error;
    }
    return result;
}

std::filesystem::path IdeCore::preview_path(const std::string& session_id) const {
    const auto current = status(session_id);
    if (!current.has_value() || current->artifact_path.empty()) {
        return {};
    }
    return current->artifact_path;
}

std::string IdeCore::make_session_id() const {
    using namespace std::chrono;
    const auto ticks = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    std::scoped_lock lock(mutex_);
    ++counter_;
    return "session_" + std::to_string(ticks) + "_" + std::to_string(counter_);
}

const LanguageSpec* IdeCore::find_language(const std::string& language) const {
    for (const auto& spec : languages_) {
        if (spec.id == language) {
            return &spec;
        }
    }
    return nullptr;
}

std::filesystem::path IdeCore::ensure_session_dir(const std::string& session_id) const {
    auto safe_id = session_id;
    for (char& ch : safe_id) {
        const bool valid = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-';
        if (!valid) {
            ch = '_';
        }
    }

    const auto session_dir = workspace_root_ / safe_id;
    std::error_code ec;
    std::filesystem::create_directories(session_dir, ec);
    return session_dir;
}

void IdeCore::store_result(const ExecutionResult& result, const std::string& state) {
    std::scoped_lock lock(mutex_);
    sessions_[result.session_id] = SessionRecord{
        result.session_id,
        result.language,
        state,
        result.exit_code,
        result.output,
        result.error,
        result.artifact_path,
    };
}

}  // namespace noc::core
