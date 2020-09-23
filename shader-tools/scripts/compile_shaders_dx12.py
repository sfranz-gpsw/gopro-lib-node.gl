import sys
from shader_tools import *
            
paths = ['ngfx/data/shaders', 'NodeGL/data/shaders', 'NodeGL/pynodegl-utils/pynodegl_utils/examples/shaders']
extensions=['.vert', '.frag', '.comp']
glslFiles = addFiles(paths, extensions)
if len(sys.argv) == 2:
	glslFiles = filterFiles(glslFiles, sys.argv[1])
	
outDir = 'cmake-build-debug'

defines = '-DGRAPHICS_BACKEND_DIRECTX12=1'
spvFiles = compileShaders(glslFiles, defines, outDir, 'glsl')
spvMapFiles = generateShaderMaps(glslFiles, outDir, 'glsl')
hlslFiles = convertShaders(spvFiles, outDir, 'hlsl')
hlslPatchFiles = glob.glob(f"patches/*.hlsl.patch")
applyPatches(hlslPatchFiles, outDir)
hlsllibFiles = compileShaders(hlslFiles, defines, outDir, 'hlsl')
hlslMapFiles = generateShaderMaps(hlslFiles, outDir, 'hlsl')
hlslMapPatchFiles = glob.glob(f"patches/*.hlsl.map.patch")
applyPatches(hlslMapPatchFiles, outDir)
