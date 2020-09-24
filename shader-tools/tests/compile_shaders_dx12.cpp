#include <vector>
#include <string>
using namespace std;

int main(int argc, char** argv) {
    const vector<string> paths = { "ngfx/data/shaders", "nodegl/data/shaders", "nodegl/pynodegl-utils/pynodegl_utils/examples/shaders" };
    const vector<string> extensions = {".vert", ".frag", ".comp"};
    auto glslFiles = addFiles(paths, extensions);
    if (argc == 2) glslFiles = filterFiles(glslFiles, argv[1]);
    string outDir = "cmake-build-debug";
    string defines = "-DGRAPHICS_BACKEND_DIRECTX12=1";
    auto spvFiles = compileShaders(glslFiles, defines, outDir, "glsl");
    auto spvMapFiles = generateShaderMaps(glslFiles, outDir, "glsl");
    auto hlslFiles = convertShaders(spvFiles, outDir, 'hlsl');
    auto hlslPatchFiles = glob.glob(f"patches/*.hlsl.patch");
    applyPatches(hlslPatchFiles, outDir);
    auto hlsllibFiles = compileShaders(hlslFiles, defines, outDir, "hlsl");
    auto hlslMapFiles = generateShaderMaps(hlslFiles, outDir, 'hlsl');
    auto hlslMapPatchFiles = glob.glob(f"patches/*.hlsl.map.patch");
    applyPatches(hlslMapPatchFiles, outDir);
    return 0;
}

