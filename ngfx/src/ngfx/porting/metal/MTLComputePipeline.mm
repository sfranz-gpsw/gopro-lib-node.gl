#include "ngfx/porting/metal/MTLComputePipeline.h"
#include "ngfx/porting/metal/MTLComputeCommandEncoder.h"
#include "ngfx/porting/metal/MTLCommandBuffer.h"
#include "ngfx/porting/metal/MTLPipelineUtil.h"
#include "ngfx/porting/metal/MTLShaderModule.h"
#include "ngfx/porting/metal/MTLGraphicsContext.h"
using namespace ngfx;

void MTLComputePipeline::create(ngfx::MTLGraphicsContext *ctx, id<MTLFunction> computeFunction) { 
    NSError* error;
    auto device = ctx->mtlDevice.v;
    mtlPipelineState = [device newComputePipelineStateWithFunction:computeFunction error:&error];
    NSCAssert(mtlPipelineState, @"Failed to create pipeline state: %@", error);
}

ComputePipeline* ComputePipeline::create(GraphicsContext* graphicsContext,
         ComputeShaderModule* cs) {
    MTLComputePipeline* mtlComputePipeline = new MTLComputePipeline();

    auto& descriptorBindings = mtlComputePipeline->descriptorBindings;
    uint32_t numDescriptors =
        cs->descriptors.empty() ? 0 : cs->descriptors.back().set + 1;
    descriptorBindings.resize(numDescriptors);
    MTLPipelineUtil::parseDescriptors(cs->descriptors, descriptorBindings);
        
    mtlComputePipeline->create(mtl(graphicsContext), mtl(cs)->mtlFunction);
    return mtlComputePipeline;
}
