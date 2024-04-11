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

#include "backends/graphics/metal/pipelines/shader.h"
#include "backends/graphics/metal/shader.h"
#include "backends/graphics/metal/framebuffer.h"
#include <Metal/Metal.hpp>

namespace Metal {

// A 4 elements with 2 components vector of floats
static const int kCoordinatesSize = 4 * 2 * sizeof(float);

ShaderPipeline::ShaderPipeline(MTL::Device *metalDevice, MTL::Function *shader)
	: _metalDevice(metalDevice), _activeShader(shader) {
	MTL::VertexDescriptor* vertexDescriptor = MTL::VertexDescriptor::alloc()->init();
	vertexDescriptor->layouts()->object(30)->setStride(sizeof(Vertex));
	vertexDescriptor->layouts()->object(30)->setStepRate(1);
	vertexDescriptor->layouts()->object(30)->setStepFunction(MTL::VertexStepFunctionPerVertex);
	// position
	vertexDescriptor->attributes()->object(0)->setFormat(MTL::VertexFormatFloat2);
	vertexDescriptor->attributes()->object(0)->setOffset(0);
	vertexDescriptor->attributes()->object(0)->setBufferIndex(30);
	// texCoord
	vertexDescriptor->attributes()->object(1)->setFormat(MTL::VertexFormatFloat2);
	vertexDescriptor->attributes()->object(1)->setOffset(sizeof(simd_float2));
	vertexDescriptor->attributes()->object(1)->setBufferIndex(30);

	_pipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
	_pipelineDescriptor->setVertexFunction(ShaderMan.query(ShaderManager::kDefaultVertexShader));
	_pipelineDescriptor->setFragmentFunction(_activeShader);
	_pipelineDescriptor->setVertexDescriptor(vertexDescriptor);
		
	MTL::RenderPipelineColorAttachmentDescriptor *renderbufferAttachment = _pipelineDescriptor->colorAttachments()->object(0);
	renderbufferAttachment->setPixelFormat(MTL::PixelFormat::PixelFormatRGBA8Unorm);

	vertexDescriptor->release();
	_activeShader->release();
}

ShaderPipeline::~ShaderPipeline() {
	_activeShader->release();
	_pipelineDescriptor->vertexDescriptor()->release();
	//_pipelineDescriptor->release();
	//OpenGL::Shader::freeBuffer(_coordsVBO);
	//OpenGL::Shader::freeBuffer(_texcoordsVBO);
	//OpenGL::Shader::freeBuffer(_colorVBO);
}

void ShaderPipeline::activateInternal() {
	Pipeline::activateInternal();
	
	NS::Error* error = nullptr;

	_pipeLineState = _metalDevice->newRenderPipelineState(_pipelineDescriptor, &error);
	if (!_pipeLineState)
	{
		__builtin_printf( "%s", error->localizedDescription()->utf8String() );
		assert( false );
	}

	//if (OpenGLContext.multitextureSupported) {
	//	GL_CALL(glActiveTexture(GL_TEXTURE0));
	//}

	//_activeShader->use();

	//GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, _colorVBO));
	//GL_CALL(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(_colorAttributes), _colorAttributes));
	//GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void ShaderPipeline::deactivateInternal() {
	//_pipeLineState->release();
	Pipeline::deactivateInternal();
}

void ShaderPipeline::setColor(float r, float g, float b, float a) {
	float *dst = _colorAttributes;
	for (uint i = 0; i < 4; ++i) {
		*dst++ = r;
		*dst++ = g;
		*dst++ = b;
		*dst++ = a;
	}
}

void ShaderPipeline::drawTextureInternal(const MetalTexture &texture, const float *coordinates, const float *texcoords) {
	assert(isActive());
	NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();
	auto *renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
	auto *attachment = renderPassDescriptor->colorAttachments()->object(0);
	attachment->setClearColor(MTL::ClearColor(0, 0, 0, 1));
	attachment->setLoadAction((MTL::LoadAction)_loadAction);
	attachment->setStoreAction(MTL::StoreActionStore);
	attachment->setTexture(_activeFramebuffer->getTargetTexture());
	
	Vertex vertices[] = {
		{{coordinates[0], coordinates[1]}, {texcoords[0], texcoords[1]}}, // Vertex 0
		{{coordinates[2], coordinates[3]}, {texcoords[2], texcoords[3]}}, // Vertex 1
		{{coordinates[4], coordinates[5]}, {texcoords[4], texcoords[5]}}, // Vertex 2
		{{coordinates[6], coordinates[7]}, {texcoords[6], texcoords[7]}}  // Vertex 3
	};

	unsigned short indices[] = {
		0, 1, 2,
		0, 2, 3
	};
	
	MTL::Buffer *vertexPositionsBuffer = _metalDevice->newBuffer(vertices, sizeof(vertices), MTL::ResourceStorageModeShared);
	MTL::Buffer *indexBuffer = _metalDevice->newBuffer(indices, sizeof(indices), MTL::ResourceStorageModeShared);

	MTL::RenderCommandEncoder *encoder = _commandBuffer->renderCommandEncoder(renderPassDescriptor);
	encoder->setRenderPipelineState(_pipeLineState);
	encoder->setBlendColor(_colorAttributes[0], _colorAttributes[1], _colorAttributes[2], _colorAttributes[3]);
	// reference to the layout buffer in vertexDescriptor
	encoder->setVertexBuffer(vertexPositionsBuffer, 0, 30);
	// This texture can now be referred to by index with the attribute [[texture(0)]] in a shader function’s parameter list.
	encoder->setFragmentTexture(texture.getMetalTexture(), 0);
	if (_paletteTexture) {
		encoder->setFragmentTexture(_paletteTexture->getMetalTexture(), 1);
	}
	if (_viewport) {
		encoder->setViewport(*_viewport);
	}
	encoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, 6, MTL::IndexTypeUInt16, indexBuffer, 0);
	encoder->endEncoding();
	renderPassDescriptor->release();
	vertexPositionsBuffer->release();
	indexBuffer->release();
	pPool->release();
}

void ShaderPipeline::setProjectionMatrix(const Math::Matrix4 &projectionMatrix) {
	//assert(isActive());

	//_activeShader->setUniform("projection", projectionMatrix);
}

} // End of namespace OpenGL

