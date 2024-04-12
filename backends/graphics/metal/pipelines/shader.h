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

#ifndef BACKENDS_GRAPHICS_METAL_PIPELINES_SHADER_H
#define BACKENDS_GRAPHICS_METAL_PIPELINES_SHADER_H

#include "backends/graphics/metal/pipelines/pipeline.h"

namespace MTL {
class Device;
class Function;
class Viewport;
}

namespace Metal {

class ShaderPipeline : public Pipeline {
public:
	ShaderPipeline(MTL::Device *metalDevice, MTL::Function *shader);
	~ShaderPipeline() override;

	void setColor(float r, float g, float b, float a) override;

	void setProjectionMatrix(const Math::Matrix4 &projectionMatrix) override;

protected:
	void activateInternal() override;
	void deactivateInternal() override;
	void drawTextureInternal(const MetalTexture &texture, const float *coordinates, const float *texcoords) override;
	matrix_float4x4 matrix_ortho(float left, float right, float bottom, float top, float nearZ, float farZ);

	//GLuint _coordsVBO;
	//GLuint _texcoordsVBO;
	//GLuint _colorVBO;
	MTL::Function *const _activeShader;
	MTL::Device *_metalDevice;
};

} // End of namespace Metal

#endif
