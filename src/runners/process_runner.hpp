#pragma once

#include <filesystem>
#include <string>

#include "core/types.hpp"

namespace noc::runners {

class ProcessRunner {
public:
    core::ExecutionResult execute(
        const core::LanguageSpec& spec,
        const core::ExecutionRequest& request,
        const std::filesystem::path& session_dir) const;

private:
    static std::string capture_command(const std::string& command, const std::filesystem::path& working_dir, int& exit_code);
    static std::string resolve_template(
        const std::string& value,
        const std::filesystem::path& source_path,
        const std::filesystem::path& binary_path,
        const std::filesystem::path& working_dir);
};

}  // namespace noc::runners
