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

#ifndef BACKENDS_GRAPHICS_METAL_PIPELINES_PIPELINE_H
#define BACKENDS_GRAPHICS_METAL_PIPELINES_PIPELINE_H

#include "backends/graphics/metal/framebuffer.h"
#include "backends/graphics/metal/texture.h"

#include "math/matrix4.h"

namespace MTL {
class Buffer;
class CommandBuffer;
class RenderPipelineDescriptor;
class RenderPipelineState;
class Texture;
class Viewport;
}

namespace Metal {

class Framebuffer;

/**
 * Interface for Metal pipeline functionality.
 */
class Pipeline {
public:
	Pipeline();
	virtual ~Pipeline() { if (isActive()) deactivate(); }
	
	/**
	 * Activate the pipeline.
	 *
	 * This sets the Metal state to make use of drawing with the given
	 * Metal pipeline.
	 */
	void activate(MTL::CommandBuffer *commandBuffer);
	
	/**
	 * Deactivate the pipeline.
	 */
	void deactivate();
	
	/**
	 * Set framebuffer to render to.
	 *
	 * Client is responsible for any memory management related to framebuffer.
	 *
	 * @param framebuffer Framebuffer to activate.
	 * @return Formerly active framebuffer.
	 */
	Framebuffer *setFramebuffer(Framebuffer *framebuffer);
	
	/**
	 * Set modulation color.
	 *
	 * @param r Red component in [0,1].
	 * @param g Green component in [0,1].
	 * @param b Blue component in [0,1].
	 * @param a Alpha component in [0,1].
	 */
	virtual void setColor(float r, float g, float b, float a) = 0;
	
	/**
	 * Draw a texture rectangle to the currently active framebuffer.
	 *
	 * @param texture     Texture to use for drawing.
	 * @param coordinates x1, y1, x2, y2 coordinates where to draw the texture.
	 */
	inline void drawTexture(const MetalTexture &texture, const float *coordinates, const float *texcoords) {
		drawTextureInternal(texture, coordinates, texcoords);
	}

	inline void drawTexture(const MetalTexture &texture, const float *coordinates) {
		drawTextureInternal(texture, coordinates, texture.getTexCoords());
	}

	inline void drawTexture(const MetalTexture &texture, float x, float y, float w, float h) {
		const float coordinates[4*2] = {
			x,     y + h, // Left Bottom point
			x + w, y + h, // Right Bottom point
			x + w, y,     // Right Top point
			x,     y      // Left Top point
		};

		drawTextureInternal(texture, coordinates, texture.getTexCoords());
	}
	
	/**
	 * Set the projection matrix.
	 *
	 * This is intended to be only ever be used by Framebuffer subclasses.
	 */
	virtual void setProjectionMatrix(const Math::Matrix4 &projectionMatrix) = 0;
	
	void setViewport(int x, int y, int w, int h);
	
	void setLoadAction(int action) { _loadAction = action; }
	
	void disableBlendMode();
	
	void setBlendModeOpaque();
	
	void setBlendModeTraditionalTransparency();
	
	void setBlendModeMaskAlphaAndInvertByColor();
	
protected:
	/**
	 * Activate the pipeline.
	 *
	 * This sets the OpenGL state to make use of drawing with the given
	 * OpenGL pipeline.
	 */
	virtual void activateInternal();

	/**
	 * Deactivate the pipeline.
	 */
	virtual void deactivateInternal();

	virtual void drawTextureInternal(const MetalTexture &texture, const float *coordinates, const float *texcoords) = 0;

	bool isActive() const { return activePipeline == this; }
	

	Framebuffer *_activeFramebuffer;
	MTL::RenderPipelineDescriptor *_pipelineDescriptor;
	MTL::RenderPipelineState *_pipeLineState;
	MTL::CommandBuffer *_commandBuffer;
	MTL::Viewport *_viewport;
	int _loadAction;
	const MetalTexture *_paletteTexture;
	float _colorAttributes[4*4];


private:
	/** Currently active rendering pipeline. */
	static Pipeline *activePipeline;
};

} // End of namespace Metal

#endif
