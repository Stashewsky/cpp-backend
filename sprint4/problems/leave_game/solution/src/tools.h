#pragma once

#include <filesystem>
#include <iostream>
#include <string>
#include <sstream>

namespace tools{

    using namespace std::literals;
    namespace fs = std::filesystem;

    // Возвращает true, если каталог p содержится внутри base_path.
    bool IsSubPath(fs::path path, fs::path base);

    // Decode url
    std::string DecodeUrl(const std::string& url);

    std::string GetMimeType(const std::string& path);

}

