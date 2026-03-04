#pragma once

#include <filesystem>
#include <string>

#include "core/ide_core.hpp"

namespace noc::api {

class HttpServer {
public:
    HttpServer(core::IdeCore& core, std::filesystem::path web_root, std::string host, unsigned short port);
    bool run();

private:
    core::IdeCore& core_;
    std::filesystem::path web_root_;
    std::string host_;
    unsigned short port_;
};

}  // namespace noc::api
