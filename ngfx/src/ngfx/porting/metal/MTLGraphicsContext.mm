#include "ngfx/porting/metal/MTLGraphicsContext.h"
#include "ngfx/porting/metal/MTLSurface.h"
#include "ngfx/core/DebugUtil.h"
#include <Foundation/Foundation.h>
using namespace ngfx;
using namespace std;

void MTLGraphicsContext::create(const char* appName, bool enableDepthStencil, bool debug) {
    this->enableDepthStencil = enableDepthStencil;
    this->debug = debug;
    mtlDevice.create();
    mtlCommandQueue = [mtlDevice.v newCommandQueue];
}
MTLGraphicsContext::~MTLGraphicsContext() {}

void MTLGraphicsContext::setSurface(Surface *surface) {
    defaultOffscreenSurfaceFormat = PixelFormat(MTLPixelFormatRGBA8Unorm);
    MTLSurface *mtl_surface = mtl(surface);
    MTKView *mtkView = nullptr;
    if (surface && !surface->offscreen) {
        offscreen = false;
        NSView *view = mtl_surface->view;
        if ([view class] == [MTKView class]) {
            mtkView = (MTKView*)view;
            mtkView.device = mtlDevice.v;
        }
        CAMetalLayer *layer = mtl_surface->getMetalLayer();
        mtlSurfaceFormat = layer.pixelFormat;
        surfaceFormat = PixelFormat(mtlSurfaceFormat);
    } else {
        offscreen = true;
        surfaceFormat = defaultOffscreenSurfaceFormat;
    }
    if (surface && numSamples != 1) {
        TODO("");
    }
    if (surface && enableDepthStencil) {
        depthFormat = PixelFormat(MTLPixelFormatDepth24Unorm_Stencil8);
        if (!surface->offscreen && mtkView) mtkView.depthStencilPixelFormat = ::MTLPixelFormat(depthFormat);
        else if (!surface->offscreen) {
            mtl_surface->depthStencilTexture.reset(new MTLDepthStencilTexture);
            mtl_surface->depthStencilTexture->create(this, mtl_surface->w, mtl_surface->h);
        }
        if (numSamples != 1) {
            TODO("");
        }
    }
    std::optional<AttachmentDescription> depthAttachmentDescription;
    if (enableDepthStencil) depthAttachmentDescription = { depthFormat };
    else depthAttachmentDescription = nullopt;
    if (surface && !surface->offscreen) {
        RenderPassConfig onscreenRenderPassConfig = {
            { { surfaceFormat, IMAGE_LAYOUT_UNDEFINED, IMAGE_LAYOUT_PRESENT_SRC } },
            depthAttachmentDescription, false, numSamples
        };
        mtlDefaultRenderPass = (MTLRenderPass*)getRenderPass(onscreenRenderPassConfig);
    }
    defaultOffscreenSurfaceFormat = PixelFormat(MTLPixelFormatRGBA8Unorm);
    RenderPassConfig offscreenRenderPassConfig = {
        { { defaultOffscreenSurfaceFormat } }, depthAttachmentDescription, false, numSamples
    };
    mtlDefaultOffscreenRenderPass = (MTLRenderPass*)getRenderPass(offscreenRenderPassConfig);
    if (surface && !surface->offscreen) {
        CAMetalLayer* metalLayer = mtl_surface->getMetalLayer();
        numSwapchainImages = metalLayer.maximumDrawableCount;
        createSwapchainFramebuffers(metalLayer.drawableSize.width, metalLayer.drawableSize.height);
    }
    createBindings();
    this->surface = surface;
}

RenderPass* MTLGraphicsContext::getRenderPass(RenderPassConfig config) {
    for (auto& r : mtlRenderPassCache) {
        if (r->config == config) return &r->mtlRenderPass;
    }
    auto renderPassData = make_unique<MTLRenderPassData>();
    auto result = &renderPassData->mtlRenderPass;
    mtlRenderPassCache.emplace_back(std::move(renderPassData));
    return result;
}

void MTLGraphicsContext::createSwapchainFramebuffers(uint32_t w, uint32_t h) {
    mtlSwapchainFramebuffers.resize(numSwapchainImages);
    for (auto& fb : mtlSwapchainFramebuffers) {
        fb.w = w;
        fb.h = h;
    }
}

void MTLGraphicsContext::createBindings() {
    device = &mtlDevice;
    pipelineCache = &mtlPipelineCache;
    swapchainFramebuffers.resize(numSwapchainImages);
    for (int j = 0; j<mtlSwapchainFramebuffers.size(); j++)
        swapchainFramebuffers[j] = &mtlSwapchainFramebuffers[j];
    defaultRenderPass = mtlDefaultRenderPass;
    defaultOffscreenRenderPass = mtlDefaultOffscreenRenderPass;
}

CommandBuffer* MTLGraphicsContext::drawCommandBuffer(int32_t index) {
    mtlDrawCommandBuffer.v = [mtlCommandQueue commandBuffer];
    return &mtlDrawCommandBuffer;
}
CommandBuffer* MTLGraphicsContext::copyCommandBuffer() {
    mtlCopyCommandBuffer.v = [mtlCommandQueue commandBuffer];
    return &mtlCopyCommandBuffer;
}
CommandBuffer* MTLGraphicsContext::computeCommandBuffer() {
    mtlComputeCommandBuffer.v = [mtlCommandQueue commandBuffer];
    return &mtlComputeCommandBuffer;
}

void MTLGraphicsContext::submit(CommandBuffer* commandBuffer) {
    auto mtlCommandBuffer = mtl(commandBuffer);
    [mtlCommandBuffer->v commit];
    if (commandBuffer == &mtlComputeCommandBuffer) {
        [mtlCommandBuffer->v waitUntilCompleted];
    }
}

GraphicsContext* GraphicsContext::create(const char* appName, bool enableDepthStencil, bool debug) {
    LOG("debug: %s", (debug)?"true": "false");
    auto mtlGraphicsContext = new MTLGraphicsContext();
    mtlGraphicsContext->create(appName, enableDepthStencil, debug);
    return mtlGraphicsContext;
}
