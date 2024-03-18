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

	_metalTexture->replaceRegion(MTL::Region(dirtyArea.left, dirtyArea.top, 0, _textureData.w - dirtyArea.left, dirtyArea.height(), 1), 0, _textureData.getBasePtr(dirtyArea.left, dirtyArea.top), _textureData.pitch);

	// We should have handled everything, thus not dirty anymore.
	clearDirty();
}

// _clut8Texture needs 8 bits internal precision, otherwise graphics glitches
// can occur.
// However, in practice (according to fuzzie) it's 8bit. If we run into
// problems, we need to switch to GL_R8 and GL_RED, but that is only supported
// for ARB_texture_rg and GLES3+ (EXT_rexture_rg does not support GL_R8).
TextureCLUT8GPU::TextureCLUT8GPU(MTL::Device *device) :
	_device(device),
	_clut8Texture(nullptr),
	_paletteTexture(nullptr),
	//_target(new TextureTarget()), _clut8Pipeline(new CLUT8LookUpPipeline()),
	_clut8Vertices(), _clut8Data(), _userPixelData(), _palette(),
	_paletteDirty(false) {
	// Allocate space for 256 colors.
	MTL::TextureDescriptor *d = MTL::TextureDescriptor::alloc()->init();
	d->setWidth(256);
	d->setHeight(1);
	_paletteTexture = _device->newTexture(d);
	d->release();

	// Setup pipeline.
	//_clut8Pipeline->setFramebuffer(_target);
	//_clut8Pipeline->setPaletteTexture(&_paletteTexture);
	//_clut8Pipeline->setColor(1.0f, 1.0f, 1.0f, 1.0f);
}

TextureCLUT8GPU::~TextureCLUT8GPU() {
	//delete _clut8Pipeline;
	//delete _target;
	_clut8Data.free();
	if (_clut8Texture)
		_clut8Texture->release();
}

void TextureCLUT8GPU::destroy() {
	_clut8Texture->release();
	//_clut8Texture.destroy();
	//_paletteTexture.destroy();
	//_target->destroy();
	//delete _clut8Pipeline;
	//_clut8Pipeline = nullptr;
}

void TextureCLUT8GPU::recreate() {
	//_clut8Texture.create();
	//_paletteTexture.create();
	//_target->create();

	// In case image date exists assure it will be completely refreshed next
	// time.
	if (_clut8Data.getPixels()) {
		flagDirty();
		_paletteDirty = true;
	}

	//if (_clut8Pipeline == nullptr) {
	//	_clut8Pipeline = new CLUT8LookUpPipeline();
		// Setup pipeline.
	//	_clut8Pipeline->setFramebuffer(_target);
	//	_clut8Pipeline->setPaletteTexture(&_paletteTexture);
	//	_clut8Pipeline->setColor(1.0f, 1.0f, 1.0f, 1.0f);
	//}
}

void TextureCLUT8GPU::enableLinearFiltering(bool enable) {
	//_target->getTexture()->enableLinearFiltering(enable);
}

void TextureCLUT8GPU::allocate(uint width, uint height) {
	// Assure the texture can contain our user data.
	//_clut8Texture.setSize(width, height);
	//_target->setSize(width, height);
	MTL::TextureDescriptor *d = MTL::TextureDescriptor::alloc()->init();
	d->setWidth(width);
	d->setHeight(height);
	_clut8Texture = _device->newTexture(d);
	d->release();

	// In case the needed texture dimension changed we will reinitialize the
	// texture data buffer.
	if (_clut8Texture->width() != (uint)_clut8Data.w || _clut8Texture->height() != (uint)_clut8Data.h) {
		// Create a buffer for the texture data.
		_clut8Data.create(_clut8Texture->width(), _clut8Texture->height(), Graphics::PixelFormat::createFormatCLUT8());
	}

	// Create a sub-buffer for raw access.
	_userPixelData = _clut8Data.getSubArea(Common::Rect(width, height));

	// Setup structures for internal rendering to metal texture.
	_clut8Vertices[0] = 0;
	_clut8Vertices[1] = 0;

	_clut8Vertices[2] = width;
	_clut8Vertices[3] = 0;

	_clut8Vertices[4] = 0;
	_clut8Vertices[5] = height;

	_clut8Vertices[6] = width;
	_clut8Vertices[7] = height;

	// The whole texture is dirty after we changed the size. This fixes
	// multiple texture size changes without any actual update in between.
	// Without this we might try to write a too big texture into the GL
	// texture.
	flagDirty();
}

Graphics::PixelFormat TextureCLUT8GPU::getFormat() const {
	return Graphics::PixelFormat::createFormatCLUT8();
}

void TextureCLUT8GPU::setColorKey(uint colorKey) {
	// The key color is set to black so the color value is pre-multiplied with the alpha value
	// to avoid color fringes due to filtering.
	// Erasing the color data is not a problem as the palette is always fully re-initialized
	// before setting the key color.
	_palette[colorKey * 4    ] = 0x00;
	_palette[colorKey * 4 + 1] = 0x00;
	_palette[colorKey * 4 + 2] = 0x00;
	_palette[colorKey * 4 + 3] = 0x00;

	_paletteDirty = true;
}

void TextureCLUT8GPU::setPalette(uint start, uint colors, const byte *palData) {
	byte *dst = _palette + start * 4;

	while (colors-- > 0) {
		memcpy(dst, palData, 3);
		dst[3] = 0xFF;

		dst += 4;
		palData += 3;
	}

	_paletteDirty = true;
}

const MTL::Texture *TextureCLUT8GPU::getMetalTexture() const {
	return _clut8Texture;
}

void TextureCLUT8GPU::updateMetalTexture() {
	const bool needLookUp = Surface::isDirty() || _paletteDirty;

	// Update CLUT8 texture if necessary.
	if (Surface::isDirty()) {
		Common::Rect dirtyArea = getDirtyArea();
		_clut8Texture->replaceRegion(MTL::Region(dirtyArea.left, dirtyArea.top, 0, _clut8Data.w - dirtyArea.left, dirtyArea.height(), 1), 0, _clut8Data.getBasePtr(dirtyArea.left, dirtyArea.top), _clut8Data.pitch); //updateArea(getDirtyArea(), _clut8Data);
		clearDirty();
	}

	// Update palette if necessary.
	if (_paletteDirty) {
		Graphics::Surface palSurface;
		palSurface.init(256, 1, 256, _palette,
#ifdef SCUMM_LITTLE_ENDIAN
						Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24) // ABGR8888
#else
						Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0) // RGBA8888
#endif
					   );

		//_paletteTexture.updateArea(Common::Rect(256, 1), palSurface);
		_paletteTexture->replaceRegion(MTL::Region(0, 0, 0, 256, 1, 1), 0, palSurface.getBasePtr(0, 0), palSurface.pitch);
		_paletteDirty = false;
	}

	// In case any data changed, do color look up and store result in _target.
	if (needLookUp) {
		lookUpColors();
	}
}

void TextureCLUT8GPU::lookUpColors() {
	// Setup pipeline to do color look up.
	//_clut8Pipeline->activate();

	// Do color look up.
	//_clut8Pipeline->drawTexture(_clut8Texture, _clut8Vertices);

	//_clut8Pipeline->deactivate();
}

} // end namespace Metal
