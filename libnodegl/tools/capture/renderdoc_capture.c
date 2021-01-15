#include "capture.h"
#include "log.h"
#ifdef _WIN32
#include <Windows.h>
#include "renderdoc_app.h"
#else
#include "renderdoc.h"
#include <dlfcn.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
static RENDERDOC_API_1_4_0 *rdoc_api = NULL;
void* _capture_device = NULL; //TODO: remove

void init_capture(void) {
#ifdef _WIN32
    // At init, on windows
    HMODULE mod = LoadLibraryA("renderdoc.dll");
    assert(mod);
    pRENDERDOC_GetAPI RENDERDOC_GetAPI =
        (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
    int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_0, (void **)&rdoc_api);
    assert(ret == 1);
#else
    void *mod = dlopen("librenderdoc.so", RTLD_LAZY); //RTLD_NOW | RTLD_NOLOAD);
    if (!mod) fprintf(stderr, "could not load librenderdoc.so: %s", dlerror());
    pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
    int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_0, (void **)&rdoc_api);
    assert(ret == 1);
#endif
    fprintf(stderr, "renderdoc capture path: %s\n", rdoc_api->GetCaptureFilePathTemplate());
}

void begin_capture(void) {
    rdoc_api->StartFrameCapture(_capture_device, NULL);
}

void end_capture(void) {
    rdoc_api->EndFrameCapture(_capture_device, NULL);
}
