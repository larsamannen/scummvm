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
	//	_renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
}

MetalRenderbufferTarget::~MetalRenderbufferTarget() {
	_metalLayer->release();
	//_renderPassDescriptor->release();

}

void MetalRenderbufferTarget::activateInternal() {
	_drawable = _metalLayer->nextDrawable();
	_targetTexture = _drawable->texture();
}

void MetalRenderbufferTarget::deactivateInternal() {
}

bool MetalRenderbufferTarget::setSize(uint width, uint height) {
	_width = width;
	_height = height;
	// Set viewport dimensions.
	_viewport->originX = 0;
	_viewport->originY = 0;
	_viewport->width = width;
	_viewport->height = height;

	// Directly apply changes when we are active.
	if (isActive()) {
		applyViewport();
		applyProjectionMatrix();
	}
	return true;
}

void MetalRenderbufferTarget::refreshScreen(MTL::CommandBuffer *commandBuffer) {
	commandBuffer->presentDrawable(_drawable);
	Framebuffer::refreshScreen(commandBuffer);
	//_renderCommandEncoder->release();
	//_commandBuffer->release();
	//_autoreleasePool->release();

}

} // End of namespace Metal

