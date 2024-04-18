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

#define FORBIDDEN_SYMBOL_ALLOW_ALL

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include "backends/graphics/metal/render2d.h"
#include "backends/graphics/metal/shader.h"


namespace Metal {

Render2d::Render2d(MTL::Device* device)
: _device(device->retain()), _cursorViewPort(new MTL::Viewport())
{
	_commandQueue = _device->newCommandQueue();
	buildShaders();
	buildBuffers();
}

Render2d::~Render2d()
{
	_vertexPositionsBuffer->release();
	_indexBuffer->release();
	_pipeLineState->release();
	_commandQueue->release();
	_device->release();
}

void Render2d::buildShaders()
{
	MTL::VertexDescriptor* defaultVertexDescriptor = MTL::VertexDescriptor::alloc()->init();
	defaultVertexDescriptor->layouts()->object(30)->setStride(sizeof(Vertex));
	defaultVertexDescriptor->layouts()->object(30)->setStepRate(1);
	defaultVertexDescriptor->layouts()->object(30)->setStepFunction(MTL::VertexStepFunctionPerVertex);
	defaultVertexDescriptor->attributes()->object(0)->setFormat(MTL::VertexFormatFloat2);
	defaultVertexDescriptor->attributes()->object(0)->setOffset(0);
	defaultVertexDescriptor->attributes()->object(0)->setBufferIndex(30);
	defaultVertexDescriptor->attributes()->object(1)->setFormat(MTL::VertexFormatFloat2);
	defaultVertexDescriptor->attributes()->object(1)->setOffset(sizeof(simd_float2));
	defaultVertexDescriptor->attributes()->object(1)->setBufferIndex(30);
	
	MTL::RenderPipelineDescriptor* defaultPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
	defaultPipelineDescriptor->setVertexFunction(ShaderMan.query(ShaderManager::kDefaultVertexShader));
	defaultPipelineDescriptor->setFragmentFunction(ShaderMan.query(ShaderManager::kDefaultFragmentShader));
	defaultPipelineDescriptor->setVertexDescriptor(defaultVertexDescriptor);
	
	MTL::RenderPipelineDescriptor* clut8PipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
	clut8PipelineDescriptor->setVertexFunction(ShaderMan.query(ShaderManager::kDefaultVertexShader));
	clut8PipelineDescriptor->setFragmentFunction(ShaderMan.query(ShaderManager::kCLUT8LookUpFragmentShader));
	clut8PipelineDescriptor->setVertexDescriptor(defaultVertexDescriptor);
	
	MTL::RenderPipelineColorAttachmentDescriptor *renderbufferAttachment = defaultPipelineDescriptor->colorAttachments()->object(0);
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

	NS::Error* error = nullptr;
	_pipeLineState = _device->newRenderPipelineState(defaultPipelineDescriptor, &error);
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

	defaultPipelineDescriptor->release();
	clut8PipelineDescriptor->release();
}

void Render2d::buildBuffers()
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

void Render2d::draw2dTexture(const MetalTexture &texture, const float *coordinates, const float *texcoords)
{
	NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();
	
	MTL::CommandBuffer* pCmd = _commandQueue->commandBuffer();
#if 0
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
#endif
	pPool->release();
}

} // end namespace Metal
