/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <simd/simd.h>
#include "graphics/surface.h"
#include "backends/graphics/metal/renderer.h"

namespace Metal {

Renderer::Renderer(MTL::Device* device)
: _device(device->retain()), _cursorViewPort(new MTL::Viewport())
{
	_commandQueue = _device->newCommandQueue();
	buildShaders();
	buildBuffers();
}

Renderer::~Renderer()
{
	_vertexPositionsBuffer->release();
	_indexBuffer->release();
	_pipeLineState->release();
	_commandQueue->release();
	_device->release();
}

void Renderer::buildShaders()
{
	using NS::StringEncoding::UTF8StringEncoding;
	
	const char* shaderSrc = R"(
	#include <metal_stdlib>
	#include <simd/simd.h>
	using namespace metal;
  
	struct Vertex
	{
		float4 position   [[attribute(0)]];
		float2 texCoord   [[attribute(1)]];
	};
  
	struct VertexOut
	{
		float4 position [[position]];
		float2 texCoord;
	};
  
	vertex VertexOut vertexFunction(Vertex in [[stage_in]])
	{
		VertexOut out;
		out.position = in.position;
		out.texCoord = in.texCoord;
		return out;
	}
  
	fragment float4 fragmentFunction(VertexOut in [[stage_in]],
		texture2d<float> colorTexture [[texture(0)]])
	{
		constexpr sampler colorSampler (mip_filter::linear, mag_filter::linear, min_filter::linear);
		// Sample the texture to obtain a color
		float4 color = colorTexture.sample(colorSampler, in.texCoord);
  
		// return the color of the texture
		return color;
	}

	fragment float4 clut8FragmentFunction(VertexOut in [[stage_in]],
		texture2d<float> colorTexture [[texture(0)]],
		texture2d<float> palette [[texture(1)]])
	{
		constexpr sampler colorSampler (mip_filter::linear, mag_filter::linear, min_filter::linear);
 
		const float adjustFactor = 255.0 / 256.0 + 1.0 / (2.0 * 256.0);
 
		// Sample the texture to obtain a color
		float4 index = colorTexture.sample(colorSampler, in.texCoord);
		float4 color = palette.sample(colorSampler, float2(index.r * adjustFactor, 0.0f));

		// return the color of the texture
		return color;
	})";
	
	NS::Error* error = nullptr;
	MTL::Library* library = _device->newLibrary( NS::String::string(shaderSrc, UTF8StringEncoding), nullptr, &error );
	if (!library)
	{
		__builtin_printf( "%s", error->localizedDescription()->utf8String() );
		assert( false );
	}
	
	MTL::Function* vertexFunction = library->newFunction( NS::String::string("vertexFunction", UTF8StringEncoding) );
	MTL::Function* fragmentFunction = library->newFunction( NS::String::string("fragmentFunction", UTF8StringEncoding) );
	MTL::Function* clut8FragmentFunction = library->newFunction( NS::String::string("clut8FragmentFunction", UTF8StringEncoding) );
	
	MTL::VertexDescriptor* vertexDescriptor = MTL::VertexDescriptor::alloc()->init();
	vertexDescriptor->layouts()->object(30)->setStride(sizeof(Vertex));
	vertexDescriptor->layouts()->object(30)->setStepRate(1);
	vertexDescriptor->layouts()->object(30)->setStepFunction(MTL::VertexStepFunctionPerVertex);
	vertexDescriptor->attributes()->object(0)->setFormat(MTL::VertexFormatFloat2);
	vertexDescriptor->attributes()->object(0)->setOffset(0);
	vertexDescriptor->attributes()->object(0)->setBufferIndex(30);
	vertexDescriptor->attributes()->object(1)->setFormat(MTL::VertexFormatFloat2);
	vertexDescriptor->attributes()->object(1)->setOffset(sizeof(simd_float2));
	vertexDescriptor->attributes()->object(1)->setBufferIndex(30);
	
	MTL::RenderPipelineDescriptor* pipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
	pipelineDescriptor->setVertexFunction(vertexFunction);
	pipelineDescriptor->setFragmentFunction(fragmentFunction);
	pipelineDescriptor->setVertexDescriptor(vertexDescriptor);
	
	MTL::RenderPipelineDescriptor* clut8PipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
	clut8PipelineDescriptor->setVertexFunction(vertexFunction);
	clut8PipelineDescriptor->setFragmentFunction(clut8FragmentFunction);
	clut8PipelineDescriptor->setVertexDescriptor(vertexDescriptor);
	
	MTL::RenderPipelineColorAttachmentDescriptor *renderbufferAttachment = pipelineDescriptor->colorAttachments()->object(0);
	renderbufferAttachment->setPixelFormat(MTL::PixelFormat::PixelFormatRGBA8Unorm);
	
	MTL::RenderPipelineColorAttachmentDescriptor *clut8RenderbufferAttachment = clut8PipelineDescriptor->colorAttachments()->object(0);
	clut8RenderbufferAttachment->setPixelFormat(MTL::PixelFormat::PixelFormatRGBA8Unorm);

	// Alpha Blending
	renderbufferAttachment->setBlendingEnabled(true);
	renderbufferAttachment->setRgbBlendOperation(MTL::BlendOperationAdd);
	renderbufferAttachment->setAlphaBlendOperation(MTL::BlendOperationAdd);
	renderbufferAttachment->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
	renderbufferAttachment->setSourceAlphaBlendFactor(MTL::BlendFactorSourceAlpha);
	renderbufferAttachment->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
	renderbufferAttachment->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
	
	_pipeLineState = _device->newRenderPipelineState( pipelineDescriptor, &error );
	if (!_pipeLineState)
	{
		__builtin_printf( "%s", error->localizedDescription()->utf8String() );
		assert( false );
	}
	_clut8PipeLineState = _device->newRenderPipelineState( clut8PipelineDescriptor, &error );
	if (!_clut8PipeLineState)
	{
		__builtin_printf( "%s", error->localizedDescription()->utf8String() );
		assert( false );
	}
	
	vertexFunction->release();
	fragmentFunction->release();
	clut8FragmentFunction->release();
	vertexDescriptor->release();
	pipelineDescriptor->release();
	library->release();
}

void Renderer::buildBuffers()
{
	Vertex vertices[] = {
		{{-1.0f, -1.0f}, {0.0f, 1.0f}}, // Vertex 0
		{{ 1.0f, -1.0f}, {1.0f, 1.0f}}, // Vertex 1
		{{ 1.0f,  1.0f}, {1.0f, 0.0f}}, // Vertex 2
		{{-1.0f,  1.0f}, {0.0f, 0.0f}}  // Vertex 3
	};
	
	unsigned short indices[] = {
		0, 1, 2,
		0, 2, 3
	};
	
	MTL::Buffer* vertexBuffer = _device->newBuffer(vertices, sizeof(vertices), MTL::ResourceStorageModeShared);
	MTL::Buffer* indexBuffer = _device->newBuffer(indices, sizeof(indices), MTL::ResourceStorageModeShared);
	_vertexPositionsBuffer = vertexBuffer;
	_indexBuffer = indexBuffer;
}

void Renderer::draw(CA::MetalDrawable *drawable, const MTL::Texture *gameTexture, const MTL::Texture *overlayTexture, const MTL::Texture *cursorTexture)
{
	NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();
	
	MTL::CommandBuffer* pCmd = _commandQueue->commandBuffer();
	
	auto *renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
	auto *attachment = renderPassDescriptor->colorAttachments()->object(0);
	attachment->setClearColor(MTL::ClearColor(0, 0, 0, 1));
	attachment->setLoadAction(MTL::LoadActionClear);
	attachment->setStoreAction(MTL::StoreActionStore);
	attachment->setTexture(drawable->texture());

	MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder(renderPassDescriptor);

	pEnc->setRenderPipelineState(_pipeLineState);
	pEnc->setVertexBuffer(_vertexPositionsBuffer, 0, 30); // reference to the layout buffer in vertexDescriptor

	pEnc->setFragmentTexture(gameTexture, 0); // This texture can now be referred to by index with the attribute [[texture(0)]] in a shader function’s parameter list.
	pEnc->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, 6, MTL::IndexTypeUInt16, _indexBuffer, 0);

	pEnc->setFragmentTexture(overlayTexture, 0); // This texture can now be referred to by index with the attribute [[texture(0)]] in a shader function’s parameter list.

	pEnc->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, 6, MTL::IndexTypeUInt16, _indexBuffer, 0);
	if (cursorTexture) {
		pEnc->setViewport(*_cursorViewPort);
		pEnc->setFragmentTexture(cursorTexture, 0); // This texture can now be referred to by index with the attribute [[texture(0)]] in a shader function’s parameter list.
		pEnc->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, 6, MTL::IndexTypeUInt16, _indexBuffer, 0);
	}
	pEnc->endEncoding();
	pCmd->presentDrawable(drawable);
	pCmd->commit();
	renderPassDescriptor->release();
	pPool->release();
}

void Renderer::drawTexture(const MTL::Texture *inTexture, const MTL::Texture *outTexture, const MTL::Texture *paletteTexture) {
	NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();
	
	MTL::CommandBuffer* pCmd = _commandQueue->commandBuffer();
	
	auto *renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
	auto *attachment = renderPassDescriptor->colorAttachments()->object(0);
	attachment->setClearColor(MTL::ClearColor(0, 0, 0, 1));
	attachment->setLoadAction(MTL::LoadActionClear);
	attachment->setStoreAction(MTL::StoreActionStore);
	attachment->setTexture(outTexture);
	
	MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder(renderPassDescriptor);

	pEnc->setRenderPipelineState(_clut8PipeLineState);
	pEnc->setVertexBuffer(_vertexPositionsBuffer, 0, 30); // reference to the layout buffer in vertexDescriptor

	pEnc->setFragmentTexture(inTexture, 0); // This texture can now be referred to by index with the attribute [[texture(0)]] in a shader function’s parameter list.
	pEnc->setFragmentTexture(paletteTexture, 1); // This texture can now be referred to by index with the attribute [[texture(1)]] in a shader function’s parameter list.
	pEnc->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, 6, MTL::IndexTypeUInt16, _indexBuffer, 0);
	
	pEnc->endEncoding();
	pCmd->commit();
	renderPassDescriptor->release();
	pPool->release();
}

void Renderer::setCursorViewport(int x, int y, int w, int h) {
	_cursorViewPort->originX = x;
	_cursorViewPort->originY = y;
	_cursorViewPort->width = w;
	_cursorViewPort->height = h;
}
} // end namespace Metal
