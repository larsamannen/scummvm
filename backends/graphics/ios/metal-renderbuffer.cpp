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

#include "backends/graphics/ios/metal-renderbuffer.h"
#include <QuartzCore/QuartzCore.hpp>
#include <Metal/Metal.hpp>

namespace Metal {

//
// Render to backbuffer target implementation
//
MetalRenderbufferTarget::MetalRenderbufferTarget(CA::MetalLayer *metalLayer)
	: Framebuffer(metalLayer->device()),  _metalLayer(metalLayer) {
		_renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
}

MetalRenderbufferTarget::~MetalRenderbufferTarget() {
	_metalLayer->release();
	_renderPassDescriptor->release();
}

void MetalRenderbufferTarget::activateInternal() {
	_drawable = _metalLayer->nextDrawable();
	// draw to drawable
	auto *attachment = _renderPassDescriptor->colorAttachments()->object(0);
	attachment->setClearColor(MTL::ClearColor(_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]));
	attachment->setLoadAction(MTL::LoadActionClear);
	attachment->setStoreAction(MTL::StoreActionStore);
	attachment->setTexture(_drawable->texture());
	_renderCommandEncoder = _commandBuffer->renderCommandEncoder(_renderPassDescriptor);
	// Allocate framebuffer object if necessary.
	//if (!_glFBO) {
	//	GL_CALL(glGenFramebuffers(1, &_glFBO));
	//	needUpdate = true;
	//}

	// Attach FBO to rendering context.
	//GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, _glFBO));

	// Attach render buffer to newly created FBO.
	//if (needUpdate) {
	//	GL_CALL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _glRBO));
	//}
}

void MetalRenderbufferTarget::deactivateInternal() {
	_drawable->texture()->release();
	_drawable->release();
	_renderCommandEncoder->release();
}

bool MetalRenderbufferTarget::setSize(uint width, uint height) {
	// Set viewport dimensions.
	_viewport->originX = 0;
	_viewport->originY = 0;
	_viewport->width = width;
	_viewport->height = height;

	// Setup orthogonal projection matrix.
//	_projectionMatrix(0, 0) =  2.0f / width;
//	_projectionMatrix(0, 1) =  0.0f;
//	_projectionMatrix(0, 2) =  0.0f;
//	_projectionMatrix(0, 3) =  0.0f;

//	_projectionMatrix(1, 0) =  0.0f;
//	_projectionMatrix(1, 1) = -2.0f / height;
//	_projectionMatrix(1, 2) =  0.0f;
//	_projectionMatrix(1, 3) =  0.0f;

//	_projectionMatrix(2, 0) =  0.0f;
//	_projectionMatrix(2, 1) =  0.0f;
//	_projectionMatrix(2, 2) =  0.0f;
//	_projectionMatrix(2, 3) =  0.0f;

//	_projectionMatrix(3, 0) = -1.0f;
//	_projectionMatrix(3, 1) =  1.0f;
//	_projectionMatrix(3, 2) =  0.0f;
//	_projectionMatrix(3, 3) =  1.0f;

	// Directly apply changes when we are active.
	if (isActive()) {
		applyViewport();
		applyProjectionMatrix();
	}
	return true;
}

void MetalRenderbufferTarget::refreshScreen() {
	_renderCommandEncoder->endEncoding();
	_commandBuffer->presentDrawable(_drawable);
	Framebuffer::refreshScreen();
}

} // End of namespace Metal

