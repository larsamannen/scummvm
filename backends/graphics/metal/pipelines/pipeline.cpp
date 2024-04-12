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

#include "backends/graphics/metal/pipelines/pipeline.h"
#include "backends/graphics/metal/framebuffer.h"

#include <Metal/Metal.hpp>

namespace Metal {

Pipeline *Pipeline::activePipeline = nullptr;

Pipeline::Pipeline()
	: _activeFramebuffer(nullptr), _viewport(nullptr) {
}

void Pipeline::activate(MTL::CommandBuffer *commandBuffer) {
	_commandBuffer = commandBuffer;

	//if (activePipeline == this) {
	//	return;
	//}

//	if (activePipeline) {
//		activePipeline->deactivate();
//	}

	activePipeline = this;

	activateInternal();
}

void Pipeline::activateInternal() {
	if (_activeFramebuffer) {
		_activeFramebuffer->activate(this);
	}
}

void Pipeline::deactivate() {
	assert(isActive());

	deactivateInternal();

	activePipeline = nullptr;
	//delete _viewport;
}

void Pipeline::deactivateInternal() {
	if (_activeFramebuffer) {
		_activeFramebuffer->deactivate();
	}
}

void Pipeline::disableBlendMode() {
	MTL::RenderPipelineColorAttachmentDescriptor *renderbufferAttachment = _pipelineDescriptor->colorAttachments()->object(0);
	renderbufferAttachment->setBlendingEnabled(false);
}

void Pipeline::setBlendModeOpaque() {
	_colorAttributes[0] = 1.0f;
	_colorAttributes[1] = 1.0f;
	_colorAttributes[2] = 1.0f;
	_colorAttributes[3] = 0.0f;
	MTL::RenderPipelineColorAttachmentDescriptor *renderbufferAttachment = _pipelineDescriptor->colorAttachments()->object(0);
	renderbufferAttachment->setBlendingEnabled(true);
	renderbufferAttachment->setRgbBlendOperation(MTL::BlendOperationAdd);
	renderbufferAttachment->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
	renderbufferAttachment->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
	//renderbufferAttachment->setSourceRGBBlendFactor(MTL::BlendFactorSourceColor);
	//renderbufferAttachment->setSourceAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);

	//renderbufferAttachment->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceColor);
}

void Pipeline::setBlendModeTraditionalTransparency() {
	MTL::RenderPipelineColorAttachmentDescriptor *renderbufferAttachment = _pipelineDescriptor->colorAttachments()->object(0);
	renderbufferAttachment->setBlendingEnabled(true);
	renderbufferAttachment->setRgbBlendOperation(MTL::BlendOperationAdd);
	renderbufferAttachment->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
	renderbufferAttachment->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
}

void Pipeline::setBlendModeMaskAlphaAndInvertByColor() {
	MTL::RenderPipelineColorAttachmentDescriptor *renderbufferAttachment = _pipelineDescriptor->colorAttachments()->object(0);
	renderbufferAttachment->setBlendingEnabled(true);
	renderbufferAttachment->setRgbBlendOperation(MTL::BlendOperationAdd);
	renderbufferAttachment->setAlphaBlendOperation(MTL::BlendOperationAdd);
	renderbufferAttachment->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
	renderbufferAttachment->setSourceAlphaBlendFactor(MTL::BlendFactorSourceAlpha);
	renderbufferAttachment->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
	renderbufferAttachment->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
}

void Pipeline::setViewport(int x, int y, int w, int h) {
	_viewport = new MTL::Viewport();
	_viewport->originX = x;
	_viewport->originY = y;
	_viewport->width = w;
	_viewport->height = h;
}

Framebuffer *Pipeline::setFramebuffer(Framebuffer *framebuffer) {
	Framebuffer *oldFramebuffer = _activeFramebuffer;
	if (isActive() && oldFramebuffer) {
		oldFramebuffer->deactivate();
	}

	_activeFramebuffer = framebuffer;
	if (isActive() && _activeFramebuffer) {
		_activeFramebuffer->activate(this);
	}

	return oldFramebuffer;
}

} // End of namespace Metal

