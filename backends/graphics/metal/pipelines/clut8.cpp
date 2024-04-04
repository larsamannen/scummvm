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

#include "backends/graphics/metal/pipelines/clut8.h"
#include "backends/graphics/metal/shader.h"
#include "backends/graphics/metal/framebuffer.h"

namespace Metal {

CLUT8LookUpPipeline::CLUT8LookUpPipeline()
	: ShaderPipeline(ShaderMan.query(ShaderManager::kCLUT8LookUp)), _paletteTexture(nullptr) {
}

void CLUT8LookUpPipeline::drawTextureInternal(const MTL::Texture &texture, const MTL::Buffer *vertexPositionsBuffer, const MTL::Buffer *indexBuffer) {
	assert(isActive());

	// Set the palette texture.
	///GL_CALL(glActiveTexture(GL_TEXTURE1));
	if (_paletteTexture) {
		// This texture can now be referred to by index with the attribute [[texture(0)]] in a shader function’s parameter list.
		//pEnc->setFragmentTexture(&texture, 0);
	}

	//GL_CALL(glActiveTexture(GL_TEXTURE0));
	ShaderPipeline::drawTextureInternal(texture, vertexPositionsBuffer, indexBuffer);
}

} // End of namespace OpenGL
