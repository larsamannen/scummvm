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

#include "backends/graphics/metal/pipelines/clut8.h"
#include "backends/graphics/metal/shader.h"
#include "backends/graphics/metal/framebuffer.h"
#include <Metal/Metal.hpp>

namespace Metal {

CLUT8LookUpPipeline::CLUT8LookUpPipeline(MTL::Device *metalDevice)
	: ShaderPipeline(metalDevice, ShaderMan.query(ShaderManager::kCLUT8LookUpFragmentShader)) {
}

void CLUT8LookUpPipeline::drawTextureInternal(const MetalTexture &texture, const float *coordinates, const float *texcoords) {
	assert(isActive());
	
	//MTL::RenderCommandEncoder *encoder = _activeFramebuffer->getRenderCommandEncoder();
	// Set the palette texture.
	if (_paletteTexture) {
		// This texture can now be referred to by index with the attribute [[texture(1)]] in a shader function’s parameter list.
		//encoder->setFragmentTexture(_paletteTexture, 1);
	}
	//encoder->release();

	ShaderPipeline::drawTextureInternal(texture, coordinates, texcoords);
}

} // End of namespace OpenGL
