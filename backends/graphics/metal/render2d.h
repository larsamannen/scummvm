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

#ifndef BACKENDS_GRAPHICS_METAL_RENDER2D_H
#define BACKENDS_GRAPHICS_METAL_RENDER2D_H

//#include "backends/graphics/metal/texture.h"

namespace CA {
class MetalDrawable;
}

namespace MTL {
class Buffer;
class CommandBuffer;
class CommandQueue;
class Device;
class RenderPipelineState;
class Texture;
class Viewport;
}

#include <simd/simd.h>

namespace Metal {

class MetalTexture;

class Render2d
{
	
	struct Vertex {
		// Positions in pixel space. A value of 100 indicates 100 pixels from the origin/center.
		simd_float2 position;
		// 2D texture coordinate
		simd_float2 texCoord;
	};
	
public:
	Render2d(MTL::Device* device);
	~Render2d();
	void buildShaders();
	void buildBuffers();
	void draw2dTexture(const MetalTexture &texture, const float *coordinates, const float *texcoords);
	
private:
	MTL::Device *_device;
	MTL::CommandBuffer *_commandBuffer;
	MTL::CommandQueue *_commandQueue;
	MTL::RenderPipelineState *_pipeLineState;
	MTL::RenderPipelineState *_clut8PipeLineState;
	MTL::Buffer* _vertexPositionsBuffer;
	MTL::Buffer* _indexBuffer;
	MTL::Viewport *_cursorViewPort;
};

} // end namespace Metal
#endif
