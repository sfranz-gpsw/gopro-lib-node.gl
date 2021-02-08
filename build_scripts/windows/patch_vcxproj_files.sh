#!/bin/bash
set -x
if [ $1 == "--set-runtime-library" ]; then
    find $3 -name '*.vcxproj' | xargs -I {} sed -i -e 's/<RuntimeLibrary>.*<\/RuntimeLibrary>/<RuntimeLibrary>'$2'<\/RuntimeLibrary>/g' {}
elif [ $1 == "--set-multiprocessor-compilation" ]; then
    find $3 -name '*.vcxproj' | xargs -I {} sed -i -e 's/[ ]*<\/ClCompile>/<MultiProcessorCompilation>'$2'<\/MultiProcessorCompilation>\n<\/ClCompile>/g' {}
fi
