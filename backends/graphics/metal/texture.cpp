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

#include "backends/graphics/metal/texture.h"
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

namespace Metal {

Surface::Surface()
	: _allDirty(false), _dirtyArea() {
}

void Surface::copyRectToTexture(uint x, uint y, uint w, uint h, const void *srcPtr, uint srcPitch) {
	Graphics::Surface *dstSurf = getSurface();
	assert(x + w <= (uint)dstSurf->w);
	assert(y + h <= (uint)dstSurf->h);

	addDirtyArea(Common::Rect(x, y, x + w, y + h));

	const byte *src = (const byte *)srcPtr;
	byte *dst = (byte *)dstSurf->getBasePtr(x, y);
	const uint pitch = dstSurf->pitch;
	const uint bytesPerPixel = dstSurf->format.bytesPerPixel;

	if (srcPitch == pitch && x == 0 && w == (uint)dstSurf->w) {
		memcpy(dst, src, h * pitch);
	} else {
		while (h-- > 0) {
			memcpy(dst, src, w * bytesPerPixel);
			dst += pitch;
			src += srcPitch;
		}
	}
}

void Surface::fill(uint32 color) {
	Graphics::Surface *dst = getSurface();
	dst->fillRect(Common::Rect(dst->w, dst->h), color);

	flagDirty();
}

void Surface::fill(const Common::Rect &r, uint32 color) {
	Graphics::Surface *dst = getSurface();
	dst->fillRect(r, color);

	addDirtyArea(r);
}

void Surface::addDirtyArea(const Common::Rect &r) {
	// *sigh* Common::Rect::extend behaves unexpected whenever one of the two
	// parameters is an empty rect. Thus, we check whether the current dirty
	// area is valid. In case it is not we simply use the parameters as new
	// dirty area. Otherwise, we simply call extend.
	if (_dirtyArea.isEmpty()) {
		_dirtyArea = r;
	} else {
		_dirtyArea.extend(r);
	}
}

Common::Rect Surface::getDirtyArea() const {
	if (_allDirty) {
		return Common::Rect(getWidth(), getHeight());
	} else {
		return _dirtyArea;
	}
}

//
// Surface implementations
//

Texture::Texture(/*GLenum glIntFormat, GLenum glFormat, GLenum glType,*/ MTL::Device *device, const Graphics::PixelFormat &format)
	: Surface(), _format(format), _device(device),/*_glTexture(glIntFormat, glFormat, glType),*/
	  _textureData(), _userPixelData() {
}

Texture::~Texture() {
	_textureData.free();
	_metalTexture->release();
}

void Texture::destroy() {
//	_metalTexture->release();
	//_glTexture.destroy();
}

void Texture::recreate() {
	//_glTexture.create();

	// In case image date exists assure it will be completely refreshed next
	// time.
	if (_textureData.getPixels()) {
		flagDirty();
	}
}

void Texture::enableLinearFiltering(bool enable) {
	//_glTexture.enableLinearFiltering(enable);
}

void Texture::allocate(uint width, uint height) {
	// Assure the texture can contain our user data.
	//_metalTexture.setSize(width, height);
	MTL::TextureDescriptor *d = MTL::TextureDescriptor::alloc()->init();
	d->setWidth(width);
	d->setHeight(height);
	d->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
	_metalTexture = _device->newTexture(d);
	d->release();

	// In case the needed texture dimension changed we will reinitialize the
	// texture data buffer.
	if (_metalTexture->width() != (uint)_textureData.w || _metalTexture->height() != (uint)_textureData.h) {
		// Create a buffer for the texture data.
		_textureData.create(_metalTexture->width(), _metalTexture->height(), _format);
	}

	// Create a sub-buffer for raw access.
	_userPixelData = _textureData.getSubArea(Common::Rect(width, height));

	// The whole texture is dirty after we changed the size. This fixes
	// multiple texture size changes without any actual update in between.
	// Without this we might try to write a too big texture into the GL
	// texture.
	flagDirty();
}

void Texture::updateMetalTexture() {
	if (!isDirty()) {
		return;
	}

	Common::Rect dirtyArea = getDirtyArea();

	updateMetalTexture(dirtyArea);
}

void Texture::updateMetalTexture(Common::Rect &dirtyArea) {
	// In case we use linear filtering we might need to duplicate the last
	// pixel row/column to avoid glitches with filtering.
//	if (_glTexture.isLinearFilteringEnabled()) {
	if (dirtyArea.right == _userPixelData.w && _userPixelData.w != _textureData.w) {
		uint height = dirtyArea.height();

		const byte *src = (const byte *)_textureData.getBasePtr(_userPixelData.w - 1, dirtyArea.top);
		byte *dst = (byte *)_textureData.getBasePtr(_userPixelData.w, dirtyArea.top);

		while (height-- > 0) {
			memcpy(dst, src, _textureData.format.bytesPerPixel);
			dst += _textureData.pitch;
			src += _textureData.pitch;
		}

		// Extend the dirty area.
		++dirtyArea.right;
	}

	if (dirtyArea.bottom == _userPixelData.h && _userPixelData.h != _textureData.h) {
		const byte *src = (const byte *)_textureData.getBasePtr(dirtyArea.left, _userPixelData.h - 1);
		byte *dst = (byte *)_textureData.getBasePtr(dirtyArea.left, _userPixelData.h);
		memcpy(dst, src, dirtyArea.width() * _textureData.format.bytesPerPixel);

		// Extend the dirty area.
		++dirtyArea.bottom;
	}

	_metalTexture->replaceRegion(MTL::Region(dirtyArea.left, dirtyArea.top, 0, dirtyArea.right /*_textureData.w*/, dirtyArea.height(), 1), 0, _textureData.getBasePtr(0, dirtyArea.top), _textureData.pitch);

	// We should have handled everything, thus not dirty anymore.
	clearDirty();
}

} // end namespace Metal
