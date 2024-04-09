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

#ifndef BACKENDS_GRAPHICS_METAL_TEXTURE_H
#define BACKENDS_GRAPHICS_METAL_TEXTURE_H

#include "graphics/pixelformat.h"
#include "graphics/surface.h"
#include "backends/graphics/metal/renderer.h"
#include "common/rect.h"

namespace MTL {
class Buffer;
class Device;
class Texture;
}

namespace Metal {

/**
 * Interface for Metal implementations of a 2D surface.
 */
class Surface {
public:
	Surface();
	virtual ~Surface() {}

	/**
	 * Destroy Metal description of surface.
	 */
	virtual void destroy() = 0;

	/**
	 * Recreate Metal description of surface.
	 */
	virtual void recreate() = 0;

	/**
	 * Enable or disable linear texture filtering.
	 *
	 * @param enable true to enable and false to disable.
	 */
	virtual void enableLinearFiltering(bool enable) = 0;

	/**
	 * Allocate storage for surface.
	 *
	 * @param width  The desired logical width.
	 * @param height The desired logical height.
	 */
	virtual void allocate(uint width, uint height) = 0;

	/**
	 * Assign a mask to the surface, where a byte value of 0 is black with 0 alpha and 1 is the normal color.
	 *
	 * @param mask   The mask data.
	 */
	virtual void setMask(const byte *mask) {}

	/**
	 * Copy image data to the surface.
	 *
	 * The format of the input data needs to match the format returned by
	 * getFormat.
	 *
	 * @param x        X coordinate of upper left corner to copy data to.
	 * @param y        Y coordinate of upper left corner to copy data to.
	 * @param w        Width of the image data to copy.
	 * @param h        Height of the image data to copy.
	 * @param src      Pointer to image data.
	 * @param srcPitch The number of bytes in a row of the image data.
	 */
	void copyRectToTexture(uint x, uint y, uint w, uint h, const void *src, uint srcPitch);

	/**
	 * Fill the surface with a fixed color.
	 *
	 * @param color Color value in format returned by getFormat.
	 */
	void fill(uint32 color);
	void fill(const Common::Rect &r, uint32 color);

	void flagDirty() { _allDirty = true; }
	virtual bool isDirty() const { return _allDirty || !_dirtyArea.isEmpty(); }

	virtual uint getWidth() const = 0;
	virtual uint getHeight() const = 0;

	/**
	 * @return The logical format of the texture data.
	 */
	virtual Graphics::PixelFormat getFormat() const = 0;

	virtual Graphics::Surface *getSurface() = 0;
	virtual const Graphics::Surface *getSurface() const = 0;

	/**
	 * @return Whether the surface is having a palette.
	 */
	virtual bool hasPalette() const { return false; }

	/**
	 * Set color key for paletted textures.
	 *
	 * This needs to be called after any palette update affecting the color
	 * key. Calling this multiple times will result in multiple color indices
	 * to be treated as color keys.
	 */
	virtual void setColorKey(uint colorKey) {}
	virtual void setPalette(uint start, uint colors, const byte *palData) {}

	virtual void setScaler(uint scalerIndex, int scaleFactor) {}

	/**
	 * Update underlying Metal texture to reflect current state.
	 */
	virtual void updateMetalTexture(MTL::CommandBuffer *commandBuffer) = 0;

	/**
	 * Obtain underlying Metal texture.
	 */
	virtual const MTL::Texture *getMetalTexture() const = 0;
	
	const MTL::Buffer *getVertexPositionsBuffer() const;
	const MTL::Buffer *getIndexBuffer() const;
protected:
	void clearDirty() { _allDirty = false; _dirtyArea = Common::Rect(); }

	void addDirtyArea(const Common::Rect &r);
	Common::Rect getDirtyArea() const;
	
	MTL::Buffer *_vertexPositionsBuffer;
	MTL::Buffer *_indexBuffer;

private:
	bool _allDirty;
	Common::Rect _dirtyArea;
};

/**
 * An Metal texture wrapper. It automatically takes care of all Metal
 * texture handling issues and also provides access to the texture data.
 */
class Texture : public Surface {
public:
	/**
	 * Create a new texture with the specific internal format.
	 *
	 * @param glIntFormat The internal format to use.
	 * @param glFormat    The input format.
	 * @param glType      The input type.
	 * @param format      The format used for the texture input.
	 */
	Texture(/*GLenum glIntFormat, GLenum glFormat, GLenum glType,*/ MTL::Device *device, const Graphics::PixelFormat &format);
	~Texture() override;

	void destroy() override;

	void recreate() override;

	void enableLinearFiltering(bool enable) override;

	void allocate(uint width, uint height) override;

	uint getWidth() const override { return _userPixelData.w; }
	uint getHeight() const override { return _userPixelData.h; }

	/**
	 * @return The logical format of the texture data.
	 */
	Graphics::PixelFormat getFormat() const override { return _format; }

	Graphics::Surface *getSurface() override { return &_userPixelData; }
	const Graphics::Surface *getSurface() const override { return &_userPixelData; }

	void updateMetalTexture(MTL::CommandBuffer *commandBuffer) override;
	const MTL::Texture *getMetalTexture() const override { return _metalTexture; }
protected:
	const Graphics::PixelFormat _format;

	void updateMetalTexture(Common::Rect &dirtyArea);

private:
	MTL::Texture *_metalTexture;
	MTL::Device *_device;

	Graphics::Surface _textureData;
	Graphics::Surface _userPixelData;
};

class TextureTarget;
class CLUT8LookUpPipeline;

class TextureCLUT8GPU : public Surface {
public:
	TextureCLUT8GPU(MTL::Device *device);
	~TextureCLUT8GPU() override;

	void destroy() override;

	void recreate() override;

	void enableLinearFiltering(bool enable) override;

	void allocate(uint width, uint height) override;

	bool isDirty() const override { return _paletteDirty || Surface::isDirty(); }

	uint getWidth() const override { return _userPixelData.w; }
	uint getHeight() const override { return _userPixelData.h; }

	Graphics::PixelFormat getFormat() const override;

	bool hasPalette() const override { return true; }

	void setColorKey(uint colorKey) override;
	void setPalette(uint start, uint colors, const byte *palData) override;

	Graphics::Surface *getSurface() override { return &_userPixelData; }
	const Graphics::Surface *getSurface() const override { return &_userPixelData; }

	void updateMetalTexture(MTL::CommandBuffer *commandBuffer) override;
	const MTL::Texture *getMetalTexture() const override;

	static bool isSupportedByContext() {
		return true;
	}
private:
	void lookUpColors(MTL::CommandBuffer *commandBuffer);

	MTL::Device *_device;
	MTL::Texture *_clut8Texture;
	MTL::Texture *_paletteTexture;
	TextureTarget *_target;
	CLUT8LookUpPipeline *_clut8Pipeline;

	Graphics::Surface _clut8Data;
	Graphics::Surface _userPixelData;

	byte _palette[4 * 256];
	bool _paletteDirty;
};
} // end namespace Metal

#endif // BACKENDS_GRAPHICS_METAL_TEXTURE_H
