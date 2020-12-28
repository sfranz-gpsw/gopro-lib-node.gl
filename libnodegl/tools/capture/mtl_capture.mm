#include "capture.h"
#include "config.h"
#include "ngfx/core/FileUtil.h"
#include <Metal/Metal.h>
#include <string>
using namespace std;

struct mtl_capture_t {
    void init() {}
    void begin_capture() {
        NSError *error;
        MTLCaptureManager *captureManager = [MTLCaptureManager sharedCaptureManager];
        MTLCaptureDescriptor *captureDescriptor = [[MTLCaptureDescriptor alloc] init];
#if defined(BACKEND_VK) and defined(BACKEND_NGFX)
        extern id<MTLDevice> _mvk_mtl_device, _ngfx_mtl_device;
        captureDescriptor.captureObject = (_mvk_mtl_device) ? _mvk_mtl_device : _ngfx_mtl_device;
#endif
        captureDescriptor.destination = MTLCaptureDestinationDeveloperTools;
        if (![captureManager startCaptureWithDescriptor: captureDescriptor error:&error]) {
            NSLog(@"Failed to start capture, error %@", error);
        }
    }
    void end_capture() {
        MTLCaptureManager *captureManager = [MTLCaptureManager sharedCaptureManager];
        [captureManager stopCapture];
    }
};
static mtl_capture_t mtl_capture;

void init_capture() { mtl_capture.init(); }
void begin_capture() { mtl_capture.begin_capture(); }
void end_capture() { mtl_capture.end_capture(); }

