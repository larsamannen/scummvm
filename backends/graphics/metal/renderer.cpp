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

#include "backends/graphics/metal/renderer.h"

#include <Metal/Metal.hpp>
#include <simd/simd.h>

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
		using namespace metal;

		struct v2f
		{
			float4 position [[position]];
			half3 color;
		};

		v2f vertex vertexMain( uint vertexId [[vertex_id]],
							   device const float3* positions [[buffer(0)]],
							   device const float3* colors [[buffer(1)]] )
		{
			v2f o;
			o.position = float4( positions[ vertexId ], 1.0 );
			o.color = half3 ( colors[ vertexId ] );
			return o;
		}

		half4 fragment fragmentMain( v2f in [[stage_in]] )
		{
			return half4( in.color, 1.0 );
		}
	)";

	NS::Error* error = nullptr;
	MTL::Library* library = _device->newLibrary( NS::String::string(shaderSrc, UTF8StringEncoding), nullptr, &error );
	if (!library)
	{
		__builtin_printf( "%s", error->localizedDescription()->utf8String() );
		assert( false );
	}

	MTL::Function* vertexFunction = library->newFunction( NS::String::string("vertexMain", UTF8StringEncoding) );
	MTL::Function* fragmentFunction = library->newFunction( NS::String::string("fragmentMain", UTF8StringEncoding) );

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
	const size_t NumVertices = 3;

	simd::float3 positions[NumVertices] =
	{
		{ -0.8f,  0.8f, 0.0f },
		{  0.0f, -0.8f, 0.0f },
		{ +0.8f,  0.8f, 0.0f }
	};

	simd::float3 colors[NumVertices] =
	{
		{  1.0, 0.3f, 0.2f },
		{  0.8f, 1.0, 0.0f },
		{  0.8f, 0.0f, 1.0 }
	};

	const size_t positionsDataSize = NumVertices * sizeof( simd::float3 );
	const size_t colorDataSize = NumVertices * sizeof( simd::float3 );

	MTL::Buffer* vertexPositionsBuffer = _device->newBuffer(positionsDataSize, MTL::ResourceStorageModeManaged);
	MTL::Buffer* vertexColorsBuffer = _device->newBuffer(colorDataSize, MTL::ResourceStorageModeManaged);

	_vertexPositionsBuffer = vertexPositionsBuffer;
	_vertexColorsBuffer = vertexColorsBuffer;

	memcpy( _vertexPositionsBuffer->contents(), positions, positionsDataSize );
	memcpy( _vertexColorsBuffer->contents(), colors, colorDataSize );

	_vertexPositionsBuffer->didModifyRange(NS::Range::Make( 0, _vertexPositionsBuffer->length()));
	_vertexColorsBuffer->didModifyRange(NS::Range::Make( 0, _vertexColorsBuffer->length()));
}

void Renderer::draw(MTL::Drawable *drawable, MTL::Texture *texture)
{
	NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

	MTL::CommandBuffer* pCmd = _commandQueue->commandBuffer();
	
	auto *renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
	auto *attachment = renderPassDescriptor->colorAttachments()->object(0);
	attachment->setClearColor(MTL::ClearColor(1, 0, 0, 1));
	attachment->setLoadAction(MTL::LoadActionClear);
	attachment->setTexture(texture);

	MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder(renderPassDescriptor);

	pEnc->setRenderPipelineState(_pipeLineState);
	pEnc->setVertexBuffer(_vertexPositionsBuffer, 0, 0);
	pEnc->setVertexBuffer(_vertexColorsBuffer, 0, 1);
	pEnc->drawPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3) );

	pEnc->endEncoding();
	pCmd->presentDrawable(drawable);
	pCmd->commit();

	pPool->release();
}
