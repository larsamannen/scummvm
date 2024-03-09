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
		using namespace metal;
		typedef enum AAPLVertexInputIndex
		{
			AAPLVertexInputIndexVertices     = 0,
			AAPLVertexInputIndexViewportSize = 1,
		} AAPLVertexInputIndex;
		typedef enum AAPLTextureIndex
		{
			AAPLTextureIndexBaseColor = 0,
		} AAPLTextureIndex;
		typedef struct
		{
			// Positions in pixel space. A value of 100 indicates 100 pixels from the origin/center.
			vector_float2 position;

			// 2D texture coordinate
			vector_float2 textureCoordinate;
		} AAPLVertex;
		struct RasterizerData
		{
			// The [[position]] attribute qualifier of this member indicates this value is
			// the clip space position of the vertex when this structure is returned from
			// the vertex shader
			float4 position [[position]];

			// Since this member does not have a special attribute qualifier, the rasterizer
			// will interpolate its value with values of other vertices making up the triangle
			// and pass that interpolated value to the fragment shader for each fragment in
			// that triangle.
			float2 textureCoordinate;
		};
 
		vertex RasterizerData
		vertexShader(uint vertexID [[ vertex_id ]],
					 constant AAPLVertex *vertexArray [[ buffer(AAPLVertexInputIndexVertices) ]],
					 constant vector_uint2 *viewportSizePointer  [[ buffer(AAPLVertexInputIndexViewportSize) ]])

		{

			RasterizerData out;

			// Index into the array of positions to get the current vertex.
			//   Positions are specified in pixel dimensions (i.e. a value of 100 is 100 pixels from
			//   the origin)
			float2 pixelSpacePosition = vertexArray[vertexID].position.xy;

			// Get the viewport size and cast to float.
			float2 viewportSize = float2(*viewportSizePointer);

			// To convert from positions in pixel space to positions in clip-space,
			//  divide the pixel coordinates by half the size of the viewport.
			// Z is set to 0.0 and w to 1.0 because this is 2D sample.
			out.position = vector_float4(0.0, 0.0, 0.0, 1.0);
			out.position.xy = pixelSpacePosition / (viewportSize / 2.0);

			 // Pass the input textureCoordinate straight to the output RasterizerData. This value will be
			 //   interpolated with the other textureCoordinate values in the vertices that make up the
			 //   triangle.
			 out.textureCoordinate = vertexArray[vertexID].textureCoordinate;

			 return out;
		 }

		 // Fragment function
		 fragment float4
		 samplingShader(RasterizerData in [[stage_in]],
						texture2d<half> colorTexture [[ texture(AAPLTextureIndexBaseColor) ]])
		 {
			 constexpr sampler textureSampler (mag_filter::linear,
											   min_filter::linear);

			 // Sample the texture to obtain a color
			 const half4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);

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

	MTL::Function* vertexFunction = library->newFunction( NS::String::string("vertexShader", UTF8StringEncoding) );
	MTL::Function* fragmentFunction = library->newFunction( NS::String::string("samplingShader", UTF8StringEncoding) );

	MTL::RenderPipelineDescriptor* pipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
	pipelineDescriptor->setVertexFunction(vertexFunction);
	pipelineDescriptor->setFragmentFunction(fragmentFunction);
	pipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm);

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
		// Pixel positions, Texture coordinates
		{ {  250,  -250 },  { 1.f, 1.f } },
		{ { -250,  -250 },  { 0.f, 1.f } },
		{ { -250,   250 },  { 0.f, 0.f } },

		{ {  250,  -250 },  { 1.f, 1.f } },
		{ { -250,   250 },  { 0.f, 0.f } },
		{ {  250,   250 },  { 1.f, 0.f } },
	};
	
	_numVertices = sizeof(quadVertices) / sizeof(AAPLVertex);

	MTL::Buffer* vertexPositionsBuffer = _device->newBuffer(sizeof(quadVertices), MTL::ResourceStorageModeManaged);

	_vertexPositionsBuffer = vertexPositionsBuffer;

	_vertexPositionsBuffer->didModifyRange(NS::Range::Make( 0, _vertexPositionsBuffer->length()));
}

void Renderer::draw(CA::MetalDrawable *drawable, Graphics::Surface &surface)
{
	if (drawable == nullptr)
		return;
	NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

	MTL::CommandBuffer* pCmd = _commandQueue->commandBuffer();
	
	auto *renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
	auto *attachment = renderPassDescriptor->colorAttachments()->object(0);
	attachment->setClearColor(MTL::ClearColor(1, 0, 0, 1));
	attachment->setLoadAction(MTL::LoadActionClear);
	attachment->setTexture(drawable->texture());

	//MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder(renderPassDescriptor);
	
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
	//pEnc->setVertexBuffer(_vertexPositionsBuffer, 0, 0);
	//pEnc->setVertexBytes(&_viewportSize, sizeof(_viewportSize), 1);
	//pEnc->setVertexBuffer(_vertexColorsBuffer, 0, 1);
	//pEnc->setFragmentTexture(texture, 0);
	//pEnc->drawPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(_numVertices) );

	//pEnc->endEncoding();
	pCmd->presentDrawable(drawable);
	pCmd->commit();

	pPool->release();
}
