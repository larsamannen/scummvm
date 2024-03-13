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
	_vertexColorsBuffer->release();
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
			float2 texCoords [[attribute(1)]];
		};
  
		vertex Vertex vertex_main(Vertex vert [[stage_in]],
								  constant Uniforms &uniforms [[buffer(1)]])
		{
			Vertex outVert;
			outVert.position = vector_float4(0.0, 0.0, 0.0, 1.0);
			outVert.texCoords = vert.texCoords;
			return outVert;
		}

		fragment float4 fragment_main(Vertex vert [[stage_in]],
								texture2d<float> colorTexture [[texture(0)]])
		{
			constexpr sampler textureSampler (mag_filter::linear,
											  min_filter::linear);
			// Sample the texture to obtain a color
			const half4 colorSample = colorTexture.sample(textureSampler, in.texCoords);

			// return the color of the texture
			return float4(colorSample);
		}
 	)";

	NS::Error* error = nullptr;
	MTL::Library* library = _device->newLibrary( NS::String::string(shaderSrc, UTF8StringEncoding), nullptr, &error );
	if (!library)
	{
		__builtin_printf( "%s", error->localizedDescription()->utf8String() );
		assert( false );
	}

	MTL::Function* vertexFunction = library->newFunction( NS::String::string("vertex_main", UTF8StringEncoding) );
	MTL::Function* fragmentFunction = library->newFunction( NS::String::string("fragment_main", UTF8StringEncoding) );

	MTL::RenderPipelineDescriptor* pipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
	pipelineDescriptor->setVertexFunction(vertexFunction);
	pipelineDescriptor->setFragmentFunction(fragmentFunction);
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
	
	static const AAPLVertex quadVertices[] =
	{
		// Positions     ,  Colors
		{ {  0.5,  -0.5 },  { 1.0, 0.0, 0.0, 1.0 } },
		{ { -0.5,  -0.5 },  { 0.0, 1.0, 0.0, 1.0 } },
		{ {  0.0,   0.5 },  { 0.0, 0.0, 1.0, 0.0 } },
	};
	
	_numVertices = sizeof(quadVertices) / sizeof(AAPLVertex);

	MTL::Buffer* vertexPositionsBuffer = _device->newBuffer(sizeof(quadVertices), MTL::ResourceStorageModeManaged);

	_vertexPositionsBuffer = vertexPositionsBuffer;

	_vertexPositionsBuffer->didModifyRange(NS::Range::Make( 0, _vertexPositionsBuffer->length()));
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
	attachment->setTexture(texture);

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
	//pEnc->setRenderPipelineState(_pipeLineState);
	pEnc->setVertexBuffer(_vertexPositionsBuffer, 0, 0);
	//pEnc->setVertexBytes(&_viewportSize, sizeof(_viewportSize), 1);
	pEnc->setVertexBuffer(_vertexColorsBuffer, 0, 1);
	//pEnc->setFragmentTexture(texture, 0);
	//pEnc->drawPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(_numVertices) );

	pEnc->endEncoding();
	//pCmd->addCompletedHandler(^void( MTL::CommandBuffer* pCmd ){

	//});

	pCmd->presentDrawable(drawable);
	pCmd->commit();
	//pCmd->waitUntilCompleted();
	pPool->release();
}
