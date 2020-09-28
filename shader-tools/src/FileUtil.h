#pragma once
#include <string>
#include <vector>

namespace ngfx {
class FileUtil {
public:
    static time_t getmtime(const std::string& filename);
    static bool srcFileChanged(const std::string& srcFileName, const std::string& targetFileName);
    static std::string readFile(const std::string& path);
    static std::vector<std::string> splitExt(const std::string& filename);
    static std::vector<std::string> findFiles(const std::string& path);
    static std::vector<std::string> findFiles(const std::string& path, const std::string& ext);
    static std::vector<std::string> filterFiles(const std::vector<std::string>& files, const std::string& fileFilter);
    static std::vector<std::string> findFiles(const std::vector<std::string>& paths, const std::vector<std::string>& extensions);
};
};
