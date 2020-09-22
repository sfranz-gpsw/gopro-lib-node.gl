#include "porting/metal/MTLDevice.h"
#include "DebugUtil.h"
using namespace ngfx;

void MTLDevice::create() {
    v = MTLCreateSystemDefaultDevice();
    NSCAssert(v, @"Failed to create metal device");
}
