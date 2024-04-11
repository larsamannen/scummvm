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

#ifndef BACKENDS_GRAPHICS_METAL_FRAMEBUFFER_H
#define BACKENDS_GRAPHICS_METAL_FRAMEBUFFER_H

#include <simd/simd.h>
#include "math/matrix4.h"

namespace MTL {
class CommandBuffer;
class CommandQueue;
class Device;
class RenderCommandEncoder;
class RenderPassColorAttachmentDescriptor;
class RenderPassDescriptor;
class Texture;
class Viewport;
}

namespace Metal {

class Pipeline;

struct Vertex {
	// Positions in pixel space. A value of 100 indicates 100 pixels from the origin/center.
	simd_float2 position;
	// 2D texture coordinate
	simd_float2 texCoord;
};

/**
 * Object describing a framebuffer Metal can render to.
 */
class Framebuffer {
public:
	Framebuffer(MTL::Device *device);
	virtual ~Framebuffer() {};
	
public:
	enum BlendMode {
		/**
		 * Newly drawn pixels overwrite the existing contents of the framebuffer
		 * without mixing with them.
		 */
		kBlendModeDisabled,
		
		/**
		 * Newly drawn pixels overwrite the existing contents of the framebuffer
		 * without mixing with them. Alpha channel is discarded.
		 */
		kBlendModeOpaque,
		
		/**
		 * Newly drawn pixels mix with the framebuffer based on their alpha value
		 * for transparency.
		 */
		kBlendModeTraditionalTransparency,
		
		/**
		 * Newly drawn pixels mix with the framebuffer based on their alpha value
		 * for transparency.
		 *
		 * Requires the image data being drawn to have its color values pre-multipled
		 * with the alpha value.
		 */
		kBlendModePremultipliedTransparency,
		
		/**
		 * Newly drawn pixels add to the destination value.
		 */
		kBlendModeAdditive,
		
		/**
		 * Newly drawn pixels mask out existing pixels based on the alpha value and
		 * add inversions of the pixels based on the color.
		 */
		kBlendModeMaskAlphaAndInvertByColor,
	};
	
	/**
	 * Set the clear color of the framebuffer.
	 */
	void setClearColor(float r, float g, float b, float a);
	
	/**
	 * Enable/disable GL_BLEND.
	 */
	void enableBlend(BlendMode mode);
	
	/**
	 * Enable/disable GL_SCISSOR_TEST.
	 */
	void enableScissorTest(bool enable);
	
	/**
	 * Set scissor box dimensions.
	 */
	void setScissorBox(int x, int y, int w, int h);
	
	void setViewport(int x, int y, int w, int h);

	/**
	 * Obtain projection matrix of the framebuffer.
	 */
	const Math::Matrix4 &getProjectionMatrix() const { return _projectionMatrix; }
	
	enum CopyMask {
		kCopyMaskClearColor   = (1 << 0),
		kCopyMaskBlendState   = (1 << 1),
		kCopyMaskScissorState = (1 << 2),
		kCopyMaskScissorBox   = (1 << 4),
		
		kCopyMaskAll          = kCopyMaskClearColor | kCopyMaskBlendState |
		kCopyMaskScissorState | kCopyMaskScissorBox,
	};
	
	/**
	 * Copy rendering state from another framebuffer
	 */
	void copyRenderStateFrom(const Framebuffer &other, uint copyMask);
	
protected:
	bool isActive() const { return _pipeline != nullptr; }
	
	MTL::Viewport *_viewport;
	void applyViewport();
	
	Math::Matrix4 _projectionMatrix;
	void applyProjectionMatrix();
	
	/**
	 * Activate framebuffer.
	 *
	 * This is supposed to set all state associated with the framebuffer.
	 */
	virtual void activateInternal() = 0;
	
	/**
	 * Deactivate framebuffer.
	 *
	 * This is supposed to make any cleanup required when unbinding the
	 * framebuffer.
	 */
	virtual void deactivateInternal() {}
	
public:
	/**
	 * Set the size of the target buffer.
	 */
	virtual bool setSize(uint width, uint height) = 0;
	
	/**
	 * Accessor to activate framebuffer for pipeline.
	 */
	void activate(Pipeline *pipeline);
	
	/**
	 * Accessor to deactivate framebuffer from pipeline.
	 */
	void deactivate();
	
	virtual void refreshScreen(MTL::CommandBuffer *commandBuffer);
	
	MTL::CommandQueue *getCommandQueue() { return _commandQueue; }
	MTL::Texture *getTargetTexture() { return _targetTexture; }

protected:
	MTL::Device *_metalDevice;
	MTL::CommandQueue *_commandQueue;
	MTL::RenderPassDescriptor *_renderPassDescriptor;
	MTL::Texture *_targetTexture;

	
	float _clearColor[4];

private:
	Pipeline *_pipeline;

	void applyClearColor();

	BlendMode _blendState;
	void applyBlendState();

	bool _scissorTestState;
	void applyScissorTestState();

	int _scissorBox[4];
	void applyScissorBox();
};

class MetalTexture;

/**
 * Render to texture framebuffer implementation.
 *
 * This target allows to render to a texture, which can then be used for
 * further rendering.
 */
class TextureTarget : public Framebuffer {
public:
	TextureTarget(MTL::Device *device);
	~TextureTarget() override;

	/**
	 * Notify that the GL context is about to be destroyed.
	 */
	void destroy();

	/**
	 * Notify that the GL context has been created.
	 */
	void create();

	/**
	 * Set size of the texture target.
	 */
	bool setSize(uint width, uint height) override;

	/**
	 * Query pointer to underlying Metal texture.
	 */
	MetalTexture *getTexture() const { return _texture; }

protected:
	void activateInternal() override;

private:
	MetalTexture *_texture;
	MTL::RenderPassDescriptor *_renderPassDescriptor;
	//GLuint _glFBO;
	bool _needUpdate;
};

} // End of namespace Metal

#endif

