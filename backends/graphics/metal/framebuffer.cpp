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

#include "backends/graphics/metal/framebuffer.h"
#include "backends/graphics/metal/pipelines/pipeline.h"
#include "backends/graphics/metal/texture.h"
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

namespace Metal {

Framebuffer::Framebuffer(MTL::Device *device)
	: _metalDevice(device), _viewport(new MTL::Viewport()), _projectionMatrix(),
	  _pipeline(nullptr), _clearColor(),
	  _blendState(kBlendModeDisabled), _scissorTestState(false), _scissorBox() {
}

void Framebuffer::activate(Pipeline *pipeline) {
	assert(pipeline);
	_pipeline = pipeline;

	applyViewport();
	applyProjectionMatrix();
	applyClearColor();
	applyBlendState();
	applyScissorTestState();
	applyScissorBox();

	activateInternal();
}

void Framebuffer::deactivate() {
	deactivateInternal();
	
	_pipeline = nullptr;
}

void Framebuffer::setClearColor(float r, float g, float b, float a) {
	_clearColor[0] = r;
	_clearColor[1] = g;
	_clearColor[2] = b;
	_clearColor[3] = a;

	// Directly apply changes when we are active.
	if (isActive()) {
		applyClearColor();
	}
}

void Framebuffer::enableBlend(BlendMode mode) {
	_blendState = mode;

	// Directly apply changes when we are active.
	if (isActive()) {
		applyBlendState();
	}
}

void Framebuffer::enableScissorTest(bool enable) {
	_scissorTestState = enable;

	// Directly apply changes when we are active.
	if (isActive()) {
		applyScissorTestState();
	}
}

void Framebuffer::setScissorBox(int x, int y, int w, int h) {
	_scissorBox[0] = x;
	_scissorBox[1] = y;
	_scissorBox[2] = w;
	_scissorBox[3] = h;

	// Directly apply changes when we are active.
	if (isActive()) {
		applyScissorBox();
	}
}

void Framebuffer::setViewport(int x, int y, int w, int h) {
	_viewport->originX = x;
	_viewport->originY = y;
	_viewport->width = w;
	_viewport->height = h;

	// Directly apply changes when we are active.
	if (isActive()) {
		applyViewport();
	}
}

void Framebuffer::applyViewport() {
	_pipeline->setViewport(_viewport->originX, _viewport->originY, _viewport->width, _viewport->height);
}

void Framebuffer::applyProjectionMatrix() {
	assert(_pipeline);
	_pipeline->setProjectionMatrix(_projectionMatrix);
}

void Framebuffer::applyClearColor() {
	//assert(_renderPassColorAttachmentDescriptor);
	//_renderPassColorAttachmentDescriptor->setClearColor(MTL::ClearColor(_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]));
}

void Framebuffer::applyBlendState() {
	_pipeline->setBlendMode(_blendState);
}

void Framebuffer::applyScissorTestState() {
	if (_scissorTestState) {
		//GL_CALL(glEnable(GL_SCISSOR_TEST));
	} else {
		//GL_CALL(glDisable(GL_SCISSOR_TEST));
	}
}

void Framebuffer::applyScissorBox() {
	//GL_CALL(glScissor(_scissorBox[0], _scissorBox[1], _scissorBox[2], _scissorBox[3]));
}

void Framebuffer::copyRenderStateFrom(const Framebuffer &other, uint copyMask) {
	if (copyMask & kCopyMaskClearColor) {
		memcpy(_clearColor, other._clearColor, sizeof(_clearColor));
	}
	if (copyMask & kCopyMaskBlendState) {
		_blendState = other._blendState;
	}
	if (copyMask & kCopyMaskScissorState) {
		_scissorTestState = other._scissorTestState;
	}
	if (copyMask & kCopyMaskScissorBox) {
		memcpy(_scissorBox, other._scissorBox, sizeof(_scissorBox));
	}

	if (isActive()) {
		applyClearColor();
		applyBlendState();
		applyScissorTestState();
		applyScissorBox();
	}
}

//
// Render to texture target implementation
//

TextureTarget::TextureTarget(MTL::Device *device) : _texture(new MetalTexture(device, MTL::PixelFormatRGBA8Unorm, (MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead))), Framebuffer(device), _needUpdate(true) {
}

TextureTarget::~TextureTarget() {
	_texture->destroy();
}

void TextureTarget::activateInternal() {
	// Allocate framebuffer object if necessary.
	if (!_targetTexture) {
//		_targetTexture = _metalDevice->
	//	GL_CALL(glGenFramebuffers(1, &_glFBO));
		_needUpdate = true;
	}

	// Attach destination texture to FBO.
	_targetTexture = _texture->getMetalTexture();

	//GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, _glFBO));

	// If required attach texture to FBO.
	if (_needUpdate) {
		//GL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texture->getGLTexture(), 0));
		_needUpdate = false;
	}
}

void TextureTarget::destroy() {
	_texture->destroy();

}

void TextureTarget::create() {
	_texture->create();
	_needUpdate = true;
}

bool TextureTarget::setSize(uint width, uint height) {
	if (!_texture->setSize(width, height)) {
		return false;
	}

	// Set viewport dimensions.
	_viewport->originX = 0;
	_viewport->originY = 0;
	_viewport->width = width;
	_viewport->height = height;

	// Setup orthogonal projection matrix.
	_projectionMatrix(0, 0) =  2.0f / (float)width;
	_projectionMatrix(0, 1) =  0.0f;
	_projectionMatrix(0, 2) =  0.0f;
	_projectionMatrix(0, 3) =  0.0f;

	_projectionMatrix(1, 0) =  0.0f;
	_projectionMatrix(1, 1) = -2.0f / (float)height;
	_projectionMatrix(1, 2) =  0.0f;
	_projectionMatrix(1, 3) =  0.0f;

	_projectionMatrix(2, 0) =  0.0f;
	_projectionMatrix(2, 1) =  0.0f;
	_projectionMatrix(2, 2) =  0.0f;
	_projectionMatrix(2, 3) =  0.0f;

	_projectionMatrix(3, 0) = -1.0f;
	_projectionMatrix(3, 1) =  1.0f;
	_projectionMatrix(3, 2) =  0.0f;
	_projectionMatrix(3, 3) =  1.0f;

	// Directly apply changes when we are active.
	if (isActive()) {
		applyViewport();
		applyProjectionMatrix();
	}
	return true;
}

} // End of namespace Metal

