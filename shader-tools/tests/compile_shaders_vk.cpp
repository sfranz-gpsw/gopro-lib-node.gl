#include <vector>
#include <string>
#include "ShaderTools.h"
using namespace std;

int main(int argc, char** argv) {
    const vector<string> paths = { "ngfx/data/shaders", "nodegl/data/shaders", "nodegl/pynodegl-utils/pynodegl_utils/examples/shaders" };
    const vector<string> extensions = {".vert", ".frag", ".comp"};
    auto glslFiles = addFiles(paths, extensions);
    if (argc == 2) glslFiles = filterFiles(glslFiles, argv[1]);
    string outDir = "cmake-build-debug";
    string defines = "-DGRAPHICS_BACKEND_VULKAN=1";
    auto spvFiles = compileShaders(glslFiles, defines, outDir, "glsl");
    auto spvMapFiles = generateShaderMaps(glslFiles, outDir, "glsl");
    return 0;
}
