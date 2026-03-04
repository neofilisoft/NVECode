#pragma once

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/types.hpp"
#include "runners/process_runner.hpp"

namespace noc::core {

class IdeCore {
public:
    IdeCore();

    const std::vector<LanguageSpec>& languages() const noexcept;
    ExecutionResult run(ExecutionRequest request);
    std::optional<SessionRecord> status(const std::string& session_id) const;
    ExecutionResult stop(const std::string& session_id);
    std::filesystem::path preview_path(const std::string& session_id) const;

private:
    std::string make_session_id() const;
    const LanguageSpec* find_language(const std::string& language) const;
    std::filesystem::path ensure_session_dir(const std::string& session_id) const;
    void store_result(const ExecutionResult& result, const std::string& state);

    std::vector<LanguageSpec> languages_;
    mutable std::mutex mutex_;
    mutable std::uint64_t counter_ = 0;
    std::unordered_map<std::string, SessionRecord> sessions_;
    std::filesystem::path workspace_root_;
    noc::runners::ProcessRunner runner_;
};

}  // namespace noc::core
