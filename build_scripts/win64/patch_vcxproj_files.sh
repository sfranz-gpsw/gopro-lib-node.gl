#!/bin/bash
set -x
if [ $1 == "--set-runtime-library" ]; then
    find $3 -name '*.vcxproj' | xargs -I {} sed -i -e 's/<RuntimeLibrary>.*<\/RuntimeLibrary>/<RuntimeLibrary>'$2'<\/RuntimeLibrary>/g' {}
fi
