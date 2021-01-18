#!/usr/bin/env python3

import os
import sys
import shutil

if len(sys.argv) < 2:
    exit(1)

dst = sys.argv[-1]
if len(sys.argv) > 2:
    if not os.path.isdir(dst):
        exit(1)

for src in sys.argv[1:-1]:
    shutil.copy2(src, dst)
