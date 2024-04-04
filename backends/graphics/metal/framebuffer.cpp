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

Framebuffer::Framebuffer()
	: _metalDevice(nullptr), _viewport(new MTL::Viewport()), _projectionMatrix(),
	  _pipeline(nullptr), _clearColor(),
	  _blendState(kBlendModeDisabled), _scissorTestState(false), _scissorBox() {
}

void Framebuffer::activate(Pipeline *pipeline) {
	assert(pipeline);
	_pipeline = pipeline;
	_commandQueue = _metalDevice->newCommandQueue();

	MTL::RenderPassDescriptor *renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
	_rendererPassColorAttachmentDescriptor = renderPassDescriptor->colorAttachments()->object(0);
	
	// Move these?
	_rendererPassColorAttachmentDescriptor->setLoadAction(MTL::LoadActionClear);
	_rendererPassColorAttachmentDescriptor->setStoreAction(MTL::StoreActionStore);
	_rendererPassColorAttachmentDescriptor->setTexture(_texture);
	

	applyViewport();
	applyProjectionMatrix();
	applyClearColor();
	applyBlendState();
	applyScissorTestState();
	applyScissorBox();

	activateInternal();
	
	//_rendererCommandEncoder = _commandQueue->renderCommandEncoder(renderPassDescriptor);
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

void Framebuffer::applyViewport() {
	assert(_rendererCommandEncoder);
	_rendererCommandEncoder->setViewport(*_viewport);
}

void Framebuffer::applyProjectionMatrix() {
	assert(_pipeline);
	_pipeline->setProjectionMatrix(_projectionMatrix);
}

void Framebuffer::applyClearColor() {
	assert(_rendererPassColorAttachmentDescriptor);
	_rendererPassColorAttachmentDescriptor->setClearColor(MTL::ClearColor(_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]));
}

void Framebuffer::applyBlendState() {
	switch (_blendState) {
		case kBlendModeDisabled:
			//GL_CALL(glDisable(GL_BLEND));
			break;
		case kBlendModeOpaque:
			//GL_CALL(glEnable(GL_BLEND));
			//GL_CALL(glBlendColor(1.f, 1.f, 1.f, 0.f));
			//GL_CALL(glBlendFunc(GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR));
			break;
		case kBlendModeTraditionalTransparency:
			//GL_CALL(glEnable(GL_BLEND));
			//GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
			break;
		case kBlendModePremultipliedTransparency:
			//GL_CALL(glEnable(GL_BLEND));
			//GL_CALL(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
			break;
		case kBlendModeAdditive:
			//GL_CALL(glEnable(GL_BLEND));
			//GL_CALL(glBlendFunc(GL_ONE, GL_ONE));
			break;
		case kBlendModeMaskAlphaAndInvertByColor:
			//GL_CALL(glEnable(GL_BLEND));
			//GL_CALL(glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA));
			break;
		default:
			break;
	}
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
// Backbuffer implementation
//

void Backbuffer::activateInternal() {
//	if (OpenGLContext.framebufferObjectSupported) {
//		GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
//	}
}

bool Backbuffer::setSize(uint width, uint height) {
	// Set viewport dimensions.
	_viewport->originX = 0;
	_viewport->originY = 0;
	_viewport->width = width;
	_viewport->height = height;

#if 0
	// Setup orthogonal projection matrix.
	_projectionMatrix(0, 0) =  2.0f / width;
	_projectionMatrix(0, 1) =  0.0f;
	_projectionMatrix(0, 2) =  0.0f;
	_projectionMatrix(0, 3) =  0.0f;

	_projectionMatrix(1, 0) =  0.0f;
	_projectionMatrix(1, 1) = -2.0f / height;
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
#endif
	// Directly apply changes when we are active.
	if (isActive()) {
		applyViewport();
		applyProjectionMatrix();
	}
	return true;
}

//
// Render to texture target implementation
//

TextureTarget::TextureTarget() : _texture(nullptr), _needUpdate(true) {
}

TextureTarget::~TextureTarget() {
	//delete _texture;
	//GL_CALL_SAFE(glDeleteFramebuffers, (1, &_glFBO));
}

void TextureTarget::activateInternal() {
	// Allocate framebuffer object if necessary.
	//if (!_glFBO) {
	//	GL_CALL(glGenFramebuffers(1, &_glFBO));
		_needUpdate = true;
	//}

	// Attach destination texture to FBO.
	//GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, _glFBO));

	// If required attach texture to FBO.
	if (_needUpdate) {
		//GL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texture->getGLTexture(), 0));
		_needUpdate = false;
	}
}

void TextureTarget::destroy() {
	//GL_CALL(glDeleteFramebuffers(1, &_glFBO));
	//_glFBO = 0;

	//_texture->destroy();
}

void TextureTarget::create() {
	_needUpdate = true;
}

bool TextureTarget::setSize(uint width, uint height) {
	MTL::TextureDescriptor *d = MTL::TextureDescriptor::alloc()->init();
	d->setWidth(width);
	d->setHeight(height);
	d->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
	d->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
	_texture = _metalDevice->newTexture(d);
	d->release();

	const uint texWidth  = _texture->width();
	const uint texHeight = _texture->height();

	// Set viewport dimensions.
	_viewport->originX = 0;
	_viewport->originY = 0;
	_viewport->width = texWidth;
	_viewport->height = texHeight;

#if 0
	// Setup orthogonal projection matrix.
	_projectionMatrix(0, 0) =  2.0f / texWidth;
	_projectionMatrix(0, 1) =  0.0f;
	_projectionMatrix(0, 2) =  0.0f;
	_projectionMatrix(0, 3) =  0.0f;

	_projectionMatrix(1, 0) =  0.0f;
	_projectionMatrix(1, 1) =  2.0f / texHeight;
	_projectionMatrix(1, 2) =  0.0f;
	_projectionMatrix(1, 3) =  0.0f;

	_projectionMatrix(2, 0) =  0.0f;
	_projectionMatrix(2, 1) =  0.0f;
	_projectionMatrix(2, 2) =  0.0f;
	_projectionMatrix(2, 3) =  0.0f;

	_projectionMatrix(3, 0) = -1.0f;
	_projectionMatrix(3, 1) = -1.0f;
	_projectionMatrix(3, 2) =  0.0f;
	_projectionMatrix(3, 3) =  1.0f;
#endif
	// Directly apply changes when we are active.
	if (isActive()) {
		applyViewport();
		//applyProjectionMatrix();
	}
	return true;
}

} // End of namespace Metal

