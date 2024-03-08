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

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include "backends/graphics/metal/metal-graphics.h"
#include "common/translation.h"
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

enum {
	GFX_METAL = 0
};

static float quadVertexData[] =
{
	0.5, -0.5, 0.0, 1.0,
	-0.5, -0.5, 0.0, 1.0,
	-0.5,  0.5, 0.0, 1.0,

	0.5,  0.5, 0.0, 1.0,
	0.5, -0.5, 0.0, 1.0,
	-0.5,  0.5, 0.0, 1.0
};

MetalGraphicsManager::MetalGraphicsManager()
{
	_cursorX = 0;
	_cursorY = 0;
	_forceRedraw = false;
	_windowWidth = 0;
	_windowHeight = 0;
}

MetalGraphicsManager::~MetalGraphicsManager()
{
	_drawable->release();
	delete _renderer;
}

void MetalGraphicsManager::notifyContextCreate(MTL::Device *device,
											   CA::MetalDrawable *drawable,
											   const Graphics::PixelFormat &defaultFormat,
											   const Graphics::PixelFormat &defaultFormatAlpha) {
	// Set up the target: backbuffer usually
	_device = device;
	_drawable = drawable;
	_drawable->retain();
	_overlayFormat = defaultFormat;
	_renderer = new Renderer(_device);
}

// Windowed
bool MetalGraphicsManager::gameNeedsAspectRatioCorrection() const {
	// TODO
	return false;
}

void MetalGraphicsManager::handleResizeImpl(const int width, const int height) {
	if (_overlayScreen)
		_overlayScreen->release();

	MTL::TextureDescriptor *d = MTL::TextureDescriptor::alloc()->init();
	d->setWidth(width);
	d->setHeight(height);
	d->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
	d->setTextureType(MTL::TextureType2D);
	d->setStorageMode(MTL::StorageModeManaged);
	d->setUsage(MTL::ResourceUsageSample | MTL::ResourceUsageRead);
	
	_overlayScreen = _device->newTexture(d);
	_overlayScreen->retain();
}

// GraphicsManager
bool MetalGraphicsManager::hasFeature(OSystem::Feature f) const {
	// TODO
	switch (f) {
	default:
		return false;
	}
}

void MetalGraphicsManager::setFeatureState(OSystem::Feature f, bool enable) {
	// TODO
	switch (f) {
	default:
		break;
	}
}

bool MetalGraphicsManager::getFeatureState(OSystem::Feature f) const {
	switch (f) {
	default:
		return false;
	}
}

const OSystem::GraphicsMode metalGraphicsModes[] = {
	{ "metal",  _s("Metal"), GFX_METAL },
	{ nullptr, nullptr, 0 }
};

const OSystem::GraphicsMode *MetalGraphicsManager::getSupportedGraphicsModes() const {
	return metalGraphicsModes;
}

#ifdef USE_RGB_COLOR
Graphics::PixelFormat MetalGraphicsManager::getScreenFormat() const {
	// TODO
	return _overlayFormat;
}

Common::List<Graphics::PixelFormat> MetalGraphicsManager::getSupportedFormats() const {
	Common::List<Graphics::PixelFormat> formats;

	// Our default mode is (memory layout wise) RGBA8888 which is a different
	// logical layout depending on the endianness. We chose this mode because
	// it is the only 32bit color mode we can safely assume to be present in
	// OpenGL and OpenGL ES implementations. Thus, we need to supply different
	// logical formats based on endianness.
#ifdef SCUMM_LITTLE_ENDIAN
	// ABGR8888
	formats.push_back(Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));
#else
	// RGBA8888
	formats.push_back(Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0));
#endif
	// RGB565
	formats.push_back(Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0));
	// RGBA5551
	formats.push_back(Graphics::PixelFormat(2, 5, 5, 5, 1, 11, 6, 1, 0));
	// RGBA4444
	formats.push_back(Graphics::PixelFormat(2, 4, 4, 4, 4, 12, 8, 4, 0));

	// These formats are not natively supported by OpenGL ES implementations,
	// we convert the pixel format internally.
#ifdef SCUMM_LITTLE_ENDIAN
	// RGBA8888
	formats.push_back(Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0));
#else
	// ABGR8888
	formats.push_back(Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));
#endif
	// RGB555, this is used by SCUMM HE 16 bit games.
	formats.push_back(Graphics::PixelFormat(2, 5, 5, 5, 0, 10, 5, 0, 0));

	formats.push_back(Graphics::PixelFormat::createFormatCLUT8());

	return formats;
}
#endif

bool MetalGraphicsManager::setGraphicsMode(int mode, uint flags) {
	return true;
}

int MetalGraphicsManager::getGraphicsMode() const {
	return 0;
}

void MetalGraphicsManager::initSize(uint width, uint height, const Graphics::PixelFormat *format) {
	handleResize(width, height);
}

void MetalGraphicsManager::initSizeHint(const Graphics::ModeList &modes) {
	
}

int MetalGraphicsManager::getScreenChangeID() const {
	return 0;
}

void MetalGraphicsManager::beginGFXTransaction() {
	
}

OSystem::TransactionError MetalGraphicsManager::endGFXTransaction() {
	return OSystem::kTransactionSuccess;
}

int16 MetalGraphicsManager::getHeight() const {
	return _overlayScreen->height();
}

int16 MetalGraphicsManager::getWidth() const {
	return _overlayScreen->width();
}

void MetalGraphicsManager::copyRectToScreen(const void *buf, int pitch, int x, int y, int w, int h) {
	_overlayScreen->replaceRegion(MTL::Region(x, y, 0, w, h, 1), 0, buf, pitch);
}

Graphics::Surface *MetalGraphicsManager::lockScreen() {
	return nullptr;
}

void MetalGraphicsManager::unlockScreen() {
	
}

void MetalGraphicsManager::fillScreen(uint32 col) {
	
}

void MetalGraphicsManager::fillScreen(const Common::Rect &r, uint32 col) {
	
}

void MetalGraphicsManager::updateScreen() {
	_renderer->draw(_drawable, _overlayScreen);
}
void MetalGraphicsManager::setShakePos(int shakeXOffset, int shakeYOffset) {
	
}
void MetalGraphicsManager::setFocusRectangle(const Common::Rect& rect) {
	
}

void MetalGraphicsManager::clearFocusRectangle() {
	
}

void MetalGraphicsManager::showOverlay(bool inGUI) {
	
}
void MetalGraphicsManager::hideOverlay() {
	printf("HEJ");
}

bool MetalGraphicsManager::isOverlayVisible() const {
	return false;
}

Graphics::PixelFormat MetalGraphicsManager::getOverlayFormat() const {
	return _overlayFormat;
}

void MetalGraphicsManager::clearOverlay() {
	
}

void MetalGraphicsManager::grabOverlay(Graphics::Surface &surface) const {
	
}

void MetalGraphicsManager::copyRectToOverlay(const void *buf, int pitch, int x, int y, int w, int h) {
	_overlayScreen->replaceRegion(MTL::Region( x, y, 0, w, h, 1 ), 0, buf, pitch);
}

int16 MetalGraphicsManager::getOverlayHeight() const {
	if (_overlayScreen) {
		return _overlayScreen->height();
	}
	return 0;
}

int16 MetalGraphicsManager::getOverlayWidth() const {
	if (_overlayScreen) {
		return _overlayScreen->width();
	}
	return 0;
}

float MetalGraphicsManager::getHiDPIScreenFactor() const {
	return 1.0f;
}

bool MetalGraphicsManager::showMouse(bool visible) {
	return 0;
}

void MetalGraphicsManager::warpMouse(int x, int y) {
	
}

void MetalGraphicsManager::setMouseCursor(const void *buf, uint w, uint h, int hotspotX, int hotspotY, uint32 keycolor, bool dontScale, const Graphics::PixelFormat *format, const byte *mask) {
	
}

void MetalGraphicsManager::setCursorPalette(const byte *colors, uint start, uint num) {
	
}

// PaletteManager
void MetalGraphicsManager::setPalette(const byte *colors, uint start, uint num) {
	
}

void MetalGraphicsManager::grabPalette(byte *colors, uint start, uint num) const {
	
}

