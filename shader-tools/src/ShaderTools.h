#pragma once
#include <ctime>
#include <string>
#include <vector>

class ShaderTools {
public:
    ShaderTools();
private:
    int cmd(std::string str);
    int compileShaderGLSL(std::string filename, const std::string& defines, const std::string& outDir,
                          std::vector<std::string>& outFiles);
    int convertShader(const std::string& file, const std::string& extraArgs, std::string outDir, std::string fmt, std::vector<std::string>& outFiles);
    bool findIncludeFile(const std::string& includeFilename, const std::vector<std::string> &includePaths,
        std::string& includeFile);
    std::time_t getmtime(const std::string& filename);
    std::string preprocess(const std::string& dataPath, const std::string& inFile);
    bool verbose = false;
    std::vector<std::string> defaultIncludePaths;
};
