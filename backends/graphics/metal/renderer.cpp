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


Renderer::Renderer(MTL::Device* device)
: _device(device->retain())
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
			float4 position [[attribute(0)]];
			float2 texCoord [[attribute(1)]];
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
 	)";

	NS::Error* error = nullptr;
	MTL::Library* library = _device->newLibrary( NS::String::string(shaderSrc, UTF8StringEncoding), nullptr, &error );
	if (!library)
	{
		__builtin_printf( "%s", error->localizedDescription()->utf8String() );
		assert( false );
	}

	MTL::Function* vertexFunction = library->newFunction( NS::String::string("vertexFunction", UTF8StringEncoding) );
	MTL::Function* fragmentFunction = library->newFunction( NS::String::string("fragmentFunction", UTF8StringEncoding) );
	
	MTL::VertexDescriptor* vertexDescriptor = MTL::VertexDescriptor::alloc()->init();
	vertexDescriptor->layouts()->object(30)->setStride(sizeof(Vertex));
	vertexDescriptor->layouts()->object(30)->setStepRate(1);
	vertexDescriptor->layouts()->object(30)->setStepFunction(MTL::VertexStepFunctionPerVertex);
	vertexDescriptor->attributes()->object(0)->setFormat(MTL::VertexFormatFloat4);
	vertexDescriptor->attributes()->object(0)->setOffset(0);
	vertexDescriptor->attributes()->object(0)->setBufferIndex(30);
	vertexDescriptor->attributes()->object(1)->setFormat(MTL::VertexFormatFloat2);
	vertexDescriptor->attributes()->object(1)->setOffset(sizeof(simd_float2));
	vertexDescriptor->attributes()->object(1)->setBufferIndex(30);
	
	MTL::RenderPipelineDescriptor* pipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
	pipelineDescriptor->setVertexFunction(vertexFunction);
	pipelineDescriptor->setFragmentFunction(fragmentFunction);
	pipelineDescriptor->setVertexDescriptor(vertexDescriptor);
	pipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormat::PixelFormatRGBA8Unorm);

	_pipeLineState = _device->newRenderPipelineState( pipelineDescriptor, &error );
	if (!_pipeLineState)
	{
		__builtin_printf( "%s", error->localizedDescription()->utf8String() );
		assert( false );
	}

	vertexFunction->release();
	fragmentFunction->release();
	pipelineDescriptor->release();
	library->release();
}

void Renderer::buildBuffers()
{
	//Create vertex buffer
/*	static float quadVertex[] = {
		-1.0f,  1.0f, 0.0f, //vertex 0
		 1.0f, -1.0f, 0.0f, //vertex 1
		-1.0f, -1.0f, 0.0f, //vertex 2
		-1.0f,  1.0f, 0.0f, //vertex 0
		 1.0f,  1.0f, 0.0f, //vertex 2
		 1.0f, -1.0f, 0.0f  //vertex 3
	};
	
	static float texCoord[] = {
		
	};*/
	
	Vertex vertices[] = {
		{{-0.5, -0.5}, {0.0f, 1.0f}}, // Vertex 0
		{{ 0.5, -0.5}, {1.0f, 1.0f}}, // Vertex 1
		{{0.5,  0.5}, {1.0f, 0.0f}}, // Vertex 2
		{{-0.5,  0.5}, {0.0f, 0.0f}} // Vertex 3
	};
	
	unsigned short indices[] = {
		0, 1, 2,
		0, 2, 3
	};
	

	MTL::Buffer* vertexBuffer = _device->newBuffer(vertices, sizeof(vertices), MTL::ResourceStorageModeShared);
	MTL::Buffer* indexBuffer = _device->newBuffer(indices, sizeof(indices), MTL::ResourceStorageModeShared);
	//MTL::Buffer* texCoordBuffer = _device->newBuffer(quadVertex, sizeof(quadVertex), MTL::ResourceStorageModeManaged);
	_vertexPositionsBuffer = vertexBuffer;
	_indexBuffer = indexBuffer;
	//_vertexPositionsBuffer->didModifyRange(NS::Range::Make(0, _vertexPositionsBuffer->length()));
	//_indexBuffer->didModifyRange(NS::Range::Make(0, _indexBuffer->length()));
}

void Renderer::draw(CA::MetalDrawable *drawable, MTL::Texture *texture)
{
	if (drawable == nullptr)
		return;
	NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

	MTL::CommandBuffer* pCmd = _commandQueue->commandBuffer();
	
	//MTL::BlitCommandEncoder *encoder = pCmd->blitCommandEncoder();
	//encoder->copyFromTexture(texture, drawable->texture());
	//encoder->copyFromTexture(texture, 0, 0, drawable->texture(), 0, 0, 1, 1);
	//encoder->copyFromTexture(texture, 0, 0, MTL::Origin(0, 0, 0), MTL::Size(texture->width(), texture->height(), texture->depth()), drawable->texture(), 0, 0, MTL::Origin(0, 0, 0));
	//encoder->endEncoding();

	auto *renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
	auto *attachment = renderPassDescriptor->colorAttachments()->object(0);
	attachment->setClearColor(MTL::ClearColor(0, 0, 0, 1));
	attachment->setLoadAction(MTL::LoadActionClear);
	attachment->setStoreAction(MTL::StoreActionStore);
	attachment->setTexture(drawable->texture());

	MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder(renderPassDescriptor);
	
	//MTL::TextureDescriptor *d = MTL::TextureDescriptor::alloc()->init();
	//d->setWidth(surface.w);
	//d->setHeight(surface.h);
	//d->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
	//d->setTextureType(MTL::TextureType2D);
	//d->setStorageMode(MTL::StorageModeManaged);
	//d->setUsage(MTL::ResourceUsageSample | MTL::ResourceUsageRead | MTL::ResourceUsageWrite);
	
	//MTL::Texture *texture = _device->newTexture(d);
	
	//texture->replaceRegion(MTL::Region(0, 0, 0, surface.w, surface.h, 1), 0, surface.getPixels(), surface.pitch);

	//MTL::Viewport viewPort = MTL::Viewport();
	//viewPort.originX = 0.0f;
	//viewPort.originY = 0.0f;
	//viewPort.width = texture->width();
	//viewPort.height = texture->height();
	//_viewportSize.x = texture->width();
	//_viewportSize.y = texture->height();
	//pEnc->setViewport(viewPort);
	pEnc->setRenderPipelineState(_pipeLineState);
	pEnc->setVertexBuffer(_vertexPositionsBuffer, 0, 30);
	//pEnc->setVertexBytes(&_viewportSize, sizeof(_viewportSize), 1);
	//pEnc->setVertexBuffer(_vertexColorsBuffer, 0, 1);
	pEnc->setFragmentTexture(texture, 0); // This texture can now be referred to by index with the attribute [[texture(0)]] in a shader function’s parameter list.
	pEnc->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, 6, MTL::IndexTypeUInt16, _indexBuffer, 0);

	pEnc->endEncoding();
	//pCmd->addCompletedHandler(^void( MTL::CommandBuffer* pCmd ){

	//});

	pCmd->presentDrawable(drawable);
	pCmd->commit();
	//pCmd->waitUntilCompleted();
	pPool->release();
}
