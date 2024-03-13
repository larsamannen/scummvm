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

#ifndef BACKENDS_GRAPHICS_METAL_RENDERER_H
#define BACKENDS_GRAPHICS_METAL_RENDERER_H

namespace Graphics {
class Surface;
}

namespace CA {
class MetalDrawable;
}

namespace MTL {
class Device;
class Texture;
class CommandQueue;
class RenderPipelineState;
class Buffer;
}
#include <simd/simd.h>
class Renderer
{
public:
	Renderer(MTL::Device* device);
	~Renderer();
	void buildShaders();
	void buildBuffers();
	void draw(CA::MetalDrawable* drawable, MTL::Texture *texture);

private:
	MTL::Device *_device;
	MTL::CommandQueue *_commandQueue;
	MTL::RenderPipelineState *_pipeLineState;
	MTL::RenderPipelineState *_renderToTextureRenderPipeline;
	MTL::Buffer* _vertexPositionsBuffer;
	MTL::Buffer* _vertexColorsBuffer;
};

#endif
