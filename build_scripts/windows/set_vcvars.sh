#!/usr/bin/bash

if [[ -z "${VCVARS64}" ]]; then
  export VCVARS64='"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"'
fi
# Set Visual Studio environment variables
WSLENV=VCVARS64/w cmd.exe /C %VCVARS64% \&\& wsl.exe
