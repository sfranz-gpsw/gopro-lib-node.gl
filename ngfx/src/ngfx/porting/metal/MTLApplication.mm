#include "MTLApplication.h"
#include "ngfx/core/DebugUtil.h"
#include "ngfx/porting/metal/MTLGraphicsContext.h"
#include "ngfx/porting/metal/MTLCommandBuffer.h"
#include "ngfx/porting/metal/MTLSurface.h"
#include <functional>
using namespace ngfx;
using namespace std::placeholders;

extern std::function<void(void*)> appInit, appPaint, appUpdate;

MTLApplication::MTLApplication(const std::string& appName,
        int width, int height, bool enableDepthStencil, bool offscreen)
    : BaseApplication(appName, width, height, enableDepthStencil, offscreen) {}

void MTLApplication::init() {
    if (offscreen) BaseApplication::init();
    else {
        appInit = [&](void* view) {
            graphicsContext.reset(GraphicsContext::create(appName.c_str(), enableDepthStencil, true));
            MTLSurface surface;
            MTKView* mtkView = (MTKView*)view;
            surface.view = mtkView;
            if (w != -1 && h != -1) {
                [mtkView setFrame: NSMakeRect(0, 0, w, h)];
            }
            CGSize mtkViewSize = [mtkView drawableSize];
            surface.w = mtkViewSize.width;
            surface.h = mtkViewSize.height;
            graphicsContext->setSurface(&surface);
            graphics.reset(Graphics::create(graphicsContext.get()));
            onInit();
        };
        appPaint = [&](void* view) {
            paint();
        };
        appUpdate = [&](void*) { onUpdate(); };
    }
}

void MTLApplication::run() {
    init();
    const char* argv[] = {""};
    NSApplicationMain(1, argv);
}


void MTLApplication::paint() {
    auto ctx = mtl(graphicsContext.get());
    MTLCommandBuffer commandBuffer;
    commandBuffer.v = [ctx->mtlCommandQueue commandBuffer];
    onRecordCommandBuffer(&commandBuffer);
    if (!offscreen) {
        MTKView* mtkView = (MTKView*)mtl(ctx->surface)->view;
        [commandBuffer.v presentDrawable:mtkView.currentDrawable];
    }
    [commandBuffer.v commit];
    if (offscreen) graphics->waitIdle(&commandBuffer);
}
