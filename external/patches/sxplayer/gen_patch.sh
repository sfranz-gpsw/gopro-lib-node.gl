#!/bin/bash
mkdir -p /tmp/sxplayer-patch/orig /tmp/sxplayer-patch/new
tar xzf sxplayer-9.6.0.tar.gz -C /tmp/sxplayer-patch/orig
cp -rf sxplayer-9.6.0 /tmp/sxplayer-patch/new/.
cd /tmp/sxplayer-patch/orig/
diff -u sxplayer-9.6.0 ../new/sxplayer-9.6.0 > ../sxplayer.patch
cd -
cp /tmp/sxplayer-patch/sxplayer.patch external/patches/sxplayer/.
