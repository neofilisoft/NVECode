#pragma once

#include <string>
#include <vector>

namespace noc::core {

enum class LanguageKind {
    Script,
    Compiled,
    Preview,
};

struct LanguageSpec {
    std::string id;
    std::string display_name;
    std::string extension;
    std::string source_name;
    LanguageKind kind = LanguageKind::Script;
    std::string compile_command;
    std::string run_command;
};

struct ExecutionRequest {
    std::string session_id;
    std::string language;
    std::string code;
};

struct ExecutionResult {
    bool success = false;
    std::string session_id;
    std::string language;
    int exit_code = -1;
    std::string output;
    std::string error;
    std::string artifact_path;
};

struct SessionRecord {
    std::string session_id;
    std::string language;
    std::string state;
    int exit_code = -1;
    std::string output;
    std::string error;
    std::string artifact_path;
};

}  // namespace noc::core
