#!/usr/bin/python3
import sys
from shader_tools import *

paths = ['ngfx/data/shaders', 'nodegl/data/shaders', 'nodegl/pynodegl-utils/pynodegl_utils/examples/shaders']
extensions=['.vert', '.frag', '.comp']
glslFiles = addFiles(paths, extensions)
if len(sys.argv) == 2:
	glslFiles = filterFiles(glslFiles, sys.argv[1])
	
outDir = 'cmake-build-debug'

defines = '-DGRAPHICS_BACKEND_VULKAN=1'
spvFiles = compileShaders(glslFiles, defines, outDir, 'glsl')
spvMapFiles = generateShaderMaps(glslFiles, outDir, 'glsl')
