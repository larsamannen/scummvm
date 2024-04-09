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
 * but WITHOUT ANY WARR ANTY; without even the implied warranty of
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
#include "backends/graphics/metal/pipelines/pipeline.h"
#include "backends/graphics/metal/pipelines/shader.h"
#include "backends/graphics/metal/shader.h"
#include "common/translation.h"
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

namespace Metal {

MetalGraphicsManager::MetalGraphicsManager() :
	_pipeline(nullptr), _currentState(), _oldState(), _defaultFormat(), _defaultFormatAlpha(),
	_gameScreen(nullptr), _overlay(nullptr), _cursor(nullptr), _cursorMask(nullptr),
	_cursorHotspotX(0), _cursorHotspotY(0),
	_cursorHotspotXScaled(0), _cursorHotspotYScaled(0), _cursorWidthScaled(0), _cursorHeightScaled(0),
	_cursorKeyColor(0), _cursorUseKey(true), _cursorDontScale(false), _cursorPaletteEnabled(false),
	_screenChangeID(0)
{
	_cursorX = 0;
	_cursorY = 0;
	_forceRedraw = false;
	_windowWidth = 0;
	_windowHeight = 0;
	memset(_gamePalette, 0, sizeof(_gamePalette));
}

MetalGraphicsManager::~MetalGraphicsManager()
{
	delete _gameScreen;
	delete _overlay;
	delete _cursor;
	delete _cursorMask;
	delete _renderer;
	delete _pipeline;
}

void MetalGraphicsManager::notifyContextCreate(CA::MetalLayer *metalLayer,
											   Framebuffer *target,
											   const Graphics::PixelFormat &defaultFormat,
											   const Graphics::PixelFormat &defaultFormatAlpha) {
	// Set up the target: backbuffer usually
//	delete _targetBuffer;
	_targetBuffer = target;

	// Initialize pipeline.
	delete _pipeline;
	_pipeline = nullptr;

	_device = metalLayer->device();
	_commandQueue = _device->newCommandQueue();

	ShaderMan.notifyCreate(_device);
	_pipeline = new ShaderPipeline(_device, ShaderMan.query(ShaderManager::kDefaultFragmentShader));
	
	_pipeline->setColor(1.0f, 1.0f, 1.0f, 1.0f);

	// Setup backbuffer state.

	// Default to opaque black as clear color.
	_targetBuffer->setClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	
	_pipeline->setFramebuffer(_targetBuffer);
	
	_defaultFormat = defaultFormat;
	_defaultFormatAlpha = defaultFormatAlpha;

	// Refresh the output screen dimensions if some are set up.
	if (_windowWidth != 0 && _windowHeight != 0) {
		handleResize(_windowWidth, _windowHeight);
	}
	
	if (_gameScreen) {
		_gameScreen->recreate();
	}

	if (_overlay) {
		_overlay->recreate();
	}

	if (_cursor) {
		_cursor->recreate();
	}

	if (_cursorMask) {
		_cursorMask->recreate();
	}

	//_renderer = new Renderer(_device);
}

void MetalGraphicsManager::notifyContextDestroy() {
	if (_gameScreen) {
		_gameScreen->destroy();
	}

	if (_overlay) {
		_overlay->destroy();
	}

	if (_cursor) {
		_cursor->destroy();
	}

	if (_cursorMask) {
		_cursorMask->destroy();
	}

	// Destroy rendering pipeline.
	delete _pipeline;
	_pipeline = nullptr;

	// Destroy the target
	delete _targetBuffer;
	_targetBuffer = nullptr;

	_commandQueue->release();
}

// Windowed
bool MetalGraphicsManager::gameNeedsAspectRatioCorrection() const {
	// TODO
	return false;
}

void MetalGraphicsManager::handleResizeImpl(const int width, const int height) {
	uint overlayWidth = width;
	uint overlayHeight = height;
	
	// HACK: We limit the minimal overlay size to 256x200, which is the
	// minimum of the dimensions of the two resolutions 256x240 (NES) and
	// 320x200 (many DOS games use this). This hopefully assure that our
	// GUI has working layouts.
	overlayWidth = MAX<uint>(overlayWidth, 256);
	overlayHeight = MAX<uint>(overlayHeight, 200);
	
	if (!_overlay || _overlay->getFormat() != _defaultFormatAlpha) {
		delete _overlay;
		_overlay = nullptr;
		
		_overlay = createSurface(_defaultFormatAlpha);
		assert(_overlay);
	}

	_overlay->allocate(overlayWidth, overlayHeight);
	_overlay->fill(0);
	
	// Re-setup the scaling and filtering for the screen and cursor
	recalculateDisplayAreas();
	recalculateCursorScaling();
	//updateLinearFiltering();
	
	// Something changed, so update the screen change ID.
	++_screenChangeID;
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
	case OSystem::kFeatureCursorPalette:
		_cursorPaletteEnabled = enable;
		updateCursorPalette();
		break;
	default:
		break;
	}
}

bool MetalGraphicsManager::getFeatureState(OSystem::Feature f) const {
	switch (f) {
	case OSystem::kFeatureCursorPalette:
		return _cursorPaletteEnabled;
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
	return _currentState.gameFormat;
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
	_currentState.graphicsMode = mode;
	return true;
}

int MetalGraphicsManager::getGraphicsMode() const {
	return _currentState.graphicsMode;
}

void MetalGraphicsManager::initSize(uint width, uint height, const Graphics::PixelFormat *format) {
	Graphics::PixelFormat requestedFormat;
#ifdef USE_RGB_COLOR
	if (!format) {
		requestedFormat = Graphics::PixelFormat::createFormatCLUT8();
	} else {
		requestedFormat = *format;
	}
	_currentState.gameFormat = requestedFormat;
#endif

	_currentState.gameWidth = width;
	_currentState.gameHeight = height;
	_gameScreenShakeXOffset = 0;
	_gameScreenShakeYOffset = 0;
	handleResizeImpl(width, height);
}

int MetalGraphicsManager::getScreenChangeID() const {
	return _screenChangeID;
}

void MetalGraphicsManager::beginGFXTransaction() {
	// Start a transaction.
	_oldState = _currentState;
	
}

OSystem::TransactionError MetalGraphicsManager::endGFXTransaction() {
	
	bool setupNewGameScreen = false;
	if (   _oldState.gameWidth  != _currentState.gameWidth
		|| _oldState.gameHeight != _currentState.gameHeight) {
		setupNewGameScreen = true;
	}

#ifdef USE_RGB_COLOR
	if (_oldState.gameFormat != _currentState.gameFormat) {
		setupNewGameScreen = true;
	}

	// Check whether the requested format can actually be used.
	Common::List<Graphics::PixelFormat> supportedFormats = getSupportedFormats();
	// In case the requested format is not usable we will fall back to CLUT8.
	if (Common::find(supportedFormats.begin(), supportedFormats.end(), _currentState.gameFormat) == supportedFormats.end()) {
		_currentState.gameFormat = Graphics::PixelFormat::createFormatCLUT8();
		//transactionError |= OSystem::kTransactionFormatNotSupported;
	}
#endif
#if 0
#ifdef USE_SCALERS
	if (_oldState.scaleFactor != _currentState.scaleFactor ||
		_oldState.scalerIndex != _currentState.scalerIndex) {
		setupNewGameScreen = true;
	}
#endif

	do {
		const uint desiredAspect = getDesiredGameAspectRatio();
		const uint requestedWidth  = _currentState.gameWidth;
		const uint requestedHeight = intToFrac(requestedWidth) / desiredAspect;

		// Consider that shader is OK by default
		// If loadVideoMode fails, we won't consider that shader was the error
		bool shaderOK = true;

		if (!loadVideoMode(requestedWidth, requestedHeight,
#ifdef USE_RGB_COLOR
						   _currentState.gameFormat
#else
						   Graphics::PixelFormat::createFormatCLUT8()
#endif
						  )
			|| !(shaderOK = loadShader(_currentState.shader))
		   // HACK: This is really nasty but we don't have any guarantees of
		   // a context existing before, which means we don't know the maximum
		   // supported texture size before this. Thus, we check whether the
		   // requested game resolution is supported over here.
		   || (   _currentState.gameWidth  > (uint)OpenGLContext.maxTextureSize
			   || _currentState.gameHeight > (uint)OpenGLContext.maxTextureSize)) {
			if (_transactionMode == kTransactionActive) {
				// Try to setup the old state in case its valid and is
				// actually different from the new one.
				if (_oldState.valid && _oldState != _currentState) {
					// Give some hints on what failed to set up.
					if (   _oldState.gameWidth  != _currentState.gameWidth
						|| _oldState.gameHeight != _currentState.gameHeight) {
						transactionError |= OSystem::kTransactionSizeChangeFailed;
					}

#ifdef USE_RGB_COLOR
					if (_oldState.gameFormat != _currentState.gameFormat) {
						transactionError |= OSystem::kTransactionFormatNotSupported;
					}
#endif

					if (_oldState.aspectRatioCorrection != _currentState.aspectRatioCorrection) {
						transactionError |= OSystem::kTransactionAspectRatioFailed;
					}

					if (_oldState.graphicsMode != _currentState.graphicsMode) {
						transactionError |= OSystem::kTransactionModeSwitchFailed;
					}

					if (_oldState.filtering != _currentState.filtering) {
						transactionError |= OSystem::kTransactionFilteringFailed;
					}
#ifdef USE_SCALERS
					if (_oldState.scalerIndex != _currentState.scalerIndex) {
						transactionError |= OSystem::kTransactionModeSwitchFailed;
					}
#endif

#if !USE_FORCED_GLES
					if (_oldState.shader != _currentState.shader) {
						transactionError |= OSystem::kTransactionShaderChangeFailed;
					}
#endif
					// Roll back to the old state.
					_currentState = _oldState;
					_transactionMode = kTransactionRollback;

					// Try to set up the old state.
					continue;
				}
				// If the shader failed and we had not a valid old state, try to unset the shader and do it again
				if (!shaderOK && !_currentState.shader.empty()) {
					_currentState.shader.clear();
					_transactionMode = kTransactionRollback;
					continue;
				}
			}

			// DON'T use error(), as this tries to bring up the debug
			// console, which WON'T WORK now that we might no have a
			// proper screen.
			warning("OpenGLGraphicsManager::endGFXTransaction: Could not load any graphics mode!");
			g_system->quit();
		}

		// In case we reach this we have a valid state, yay.
		_transactionMode = kTransactionNone;
		_currentState.valid = true;
	} while (_transactionMode == kTransactionRollback);
#endif
	if (setupNewGameScreen) {
		delete _gameScreen;
		_gameScreen = nullptr;

		bool wantScaler = _currentState.scaleFactor > 1;

#ifdef USE_RGB_COLOR
		_gameScreen = createSurface(_currentState.gameFormat, false, wantScaler);
#else
		_gameScreen = createSurface(Graphics::PixelFormat::createFormatCLUT8(), false, wantScaler);
#endif
		assert(_gameScreen);
		if (_gameScreen->hasPalette()) {
			_gameScreen->setPalette(0, 256, _gamePalette);
		}

#ifdef USE_SCALERS
		if (wantScaler) {
			_gameScreen->setScaler(_currentState.scalerIndex, _currentState.scaleFactor);
		}
#endif

		_gameScreen->allocate(_currentState.gameWidth, _currentState.gameHeight);
		// We fill the screen to all black or index 0 for CLUT8.
#ifdef USE_RGB_COLOR
		if (_currentState.gameFormat.bytesPerPixel == 1) {
			_gameScreen->fill(0);
		} else {
			_gameScreen->fill(_gameScreen->getSurface()->format.RGBToColor(0, 0, 0));
		}
#else
		_gameScreen->fill(0);
#endif
	}
	// Update our display area and cursor scaling. This makes sure we pick up
	// aspect ratio correction and game screen changes correctly.
	recalculateDisplayAreas();
	recalculateCursorScaling();
	//updateLinearFiltering();

	// Something changed, so update the screen change ID.
	++_screenChangeID;

	// Since transactionError is a ORd list of TransactionErrors this is
	// clearly wrong. But our API is simply broken.
	//return (OSystem::TransactionError)transactionError;
#if 0
	_gameScreen = createSurface(Graphics::PixelFormat::createFormatCLUT8(), false, false);

	assert(_gameScreen);
	if (_gameScreen->hasPalette()) {
		_gameScreen->setPalette(0, 256, _gamePalette);
	}
	
	_gameScreen->allocate(_windowWidth, _windowHeight);
#endif
	return OSystem::kTransactionSuccess;
}

int16 MetalGraphicsManager::getHeight() const {
	return _currentState.gameHeight;
}

int16 MetalGraphicsManager::getWidth() const {
	return _currentState.gameWidth;
}

void MetalGraphicsManager::copyRectToScreen(const void *buf, int pitch, int x, int y, int w, int h) {
	_gameScreen->copyRectToTexture(x, y, w, h, buf, pitch);
}

Graphics::Surface *MetalGraphicsManager::lockScreen() {
	return nullptr;
}

void MetalGraphicsManager::unlockScreen() {
	
}

void MetalGraphicsManager::fillScreen(uint32 col) {
	_gameScreen->fill(col);
}

void MetalGraphicsManager::fillScreen(const Common::Rect &r, uint32 col) {
	_gameScreen->fill(r, col);
}

void MetalGraphicsManager::updateScreen() {
	NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

	MTL::CommandBuffer *commandBuffer = _commandQueue->commandBufferWithUnretainedReferences();

	// Update changes to textures.
#if 0
	_gameScreen->updateMetalTexture();
	if (_cursorVisible && _cursor) {
		_cursor->updateMetalTexture();
	}
	if (_cursorVisible && _cursorMask) {
		_cursorMask->updateMetalTexture();
	}
#endif
	_overlay->updateMetalTexture();
	
	_pipeline->activate(commandBuffer);
	
	// Clear the screen buffer.
	// TODO LARS, clear the screen?
	
	if (!_overlayVisible) {
		// The scissor test is enabled to:
		// - Clip the cursor to the game screen
		// - Clip the game screen when the shake offset is non-zero
		_targetBuffer->enableScissorTest(true);
	}

	// Don't draw cursor if it's not visible or there is none
	bool drawCursor = _cursorVisible && _cursor;

	// Alpha blending is disabled when drawing the screen
	_targetBuffer->enableBlend(Framebuffer::kBlendModeOpaque);

	// First step: Draw the (virtual) game screen.
	//_pipeline->drawTexture(*_gameScreen->getMetalTexture(), _gameScreen->getVertexPositionsBuffer(), _gameScreen->getIndexBuffer());
	
	// Third step: Draw the overlay if visible.
	if (_overlayVisible) {
		int dstX = (_windowWidth - _overlayDrawRect.width()) / 2;
		int dstY = (_windowHeight - _overlayDrawRect.height()) / 2;
		_targetBuffer->enableBlend(Framebuffer::kBlendModeTraditionalTransparency);
		_pipeline->drawTexture(*_overlay->getMetalTexture(), _overlay->getVertexPositionsBuffer(), _overlay->getIndexBuffer());
	}

	// Fourth step: Draw the cursor if we didn't before.
	//if (drawCursor)
	//	renderCursor();

	if (!_overlayVisible) {
		_targetBuffer->enableScissorTest(false);
	}
	
	_cursorNeedsRedraw = false;
	_forceRedraw = false;
	_targetBuffer->refreshScreen(commandBuffer);
	
	//_pipeline->deactivate();
	
	//CA::MetalDrawable *drawable = getNextDrawable();
	//_renderer->draw(drawable, _gameScreen->getMetalTexture(), _overlay->getMetalTexture(), drawCursor ? _cursor->getMetalTexture() : nullptr);
	//drawable->release();
	pPool->release();
}

void MetalGraphicsManager::setFocusRectangle(const Common::Rect& rect) {
	
}

void MetalGraphicsManager::clearFocusRectangle() {
	
}

void MetalGraphicsManager::showOverlay(bool inGUI) {
	if (_overlayVisible && _overlayInGUI == inGUI) {
		return;
	}

	WindowedGraphicsManager::showOverlay(inGUI);
	
}
void MetalGraphicsManager::hideOverlay() {
	printf("HEJ");
}

bool MetalGraphicsManager::isOverlayVisible() const {
	return false;
}

Graphics::PixelFormat MetalGraphicsManager::getOverlayFormat() const {
	return _overlay->getFormat();
}

void MetalGraphicsManager::clearOverlay() {
	_overlay->fill(0);
}

void MetalGraphicsManager::grabOverlay(Graphics::Surface &surface) const {
	const Graphics::Surface *overlayData = _overlay->getSurface();

	assert(surface.w >= overlayData->w);
	assert(surface.h >= overlayData->h);
	assert(surface.format.bytesPerPixel == overlayData->format.bytesPerPixel);

	const byte *src = (const byte *)overlayData->getPixels();
	byte *dst = (byte *)surface.getPixels();
	// LARs TODO
	//Graphics::copyBlit(dst, src, surface.pitch, overlayData->pitch, overlayData->w, overlayData->h, overlayData->format.bytesPerPixel);
}

void MetalGraphicsManager::copyRectToOverlay(const void *buf, int pitch, int x, int y, int w, int h) {
	_overlay->copyRectToTexture(x, y, w, h, buf, pitch);
}

int16 MetalGraphicsManager::getOverlayHeight() const {
	if (_overlay) {
		return _overlay->getHeight();
	}
	return 0;
}

int16 MetalGraphicsManager::getOverlayWidth() const {
	if (_overlay) {
		return _overlay->getWidth();
	}
	return 0;
}

bool MetalGraphicsManager::showMouse(bool visible) {
	return WindowedGraphicsManager::showMouse(visible);
}

void MetalGraphicsManager::warpMouse(int x, int y) {
	
}

namespace {
template<typename SrcColor, typename DstColor>
void multiplyColorWithAlpha(const byte *src, byte *dst, const uint w, const uint h,
							const Graphics::PixelFormat &srcFmt, const Graphics::PixelFormat &dstFmt,
							const uint srcPitch, const uint dstPitch, const SrcColor keyColor, bool useKeyColor) {
	for (uint y = 0; y < h; ++y) {
		for (uint x = 0; x < w; ++x) {
			const uint32 color = *(const SrcColor *)src;

			if (useKeyColor && color == keyColor) {
				*(DstColor *)dst = 0;
			} else {
				byte a, r, g, b;
				srcFmt.colorToARGB(color, a, r, g, b);

				if (a != 0xFF) {
					r = (int) r * a / 255;
					g = (int) g * a / 255;
					b = (int) b * a / 255;
				}

				*(DstColor *)dst = dstFmt.ARGBToColor(a, r, g, b);
			}

			src += sizeof(SrcColor);
			dst += sizeof(DstColor);
		}

		src += srcPitch - w * srcFmt.bytesPerPixel;
		dst += dstPitch - w * dstFmt.bytesPerPixel;
	}
}
} // End of anonymous namespace

void MetalGraphicsManager::setMouseCursor(const void *buf, uint w, uint h, int hotspotX, int hotspotY, uint32 keycolor, bool dontScale, const Graphics::PixelFormat *format, const byte *mask) {

	_cursorUseKey = (mask == nullptr);
	if (_cursorUseKey)
		_cursorKeyColor = keycolor;

	_cursorHotspotX = hotspotX;
	_cursorHotspotY = hotspotY;
	_cursorDontScale = dontScale;

	if (!w || !h) {
		delete _cursor;
		_cursor = nullptr;
		delete _cursorMask;
		_cursorMask = nullptr;
		return;
	}

	Graphics::PixelFormat inputFormat;
	Graphics::PixelFormat maskFormat;
#ifdef USE_RGB_COLOR
	if (format) {
		inputFormat = *format;
	} else {
		inputFormat = Graphics::PixelFormat::createFormatCLUT8();
	}
#else
	inputFormat = Graphics::PixelFormat::createFormatCLUT8();
#endif

//#ifdef USE_SCALERS
//	bool wantScaler = (_currentState.scaleFactor > 1) && !dontScale && _scalerPlugins[_currentState.scalerIndex]->get<ScalerPluginObject>().canDrawCursor();
//#else
	bool wantScaler = !dontScale;
//#endif

	bool wantMask = (mask != nullptr);
	bool haveMask = (_cursorMask != nullptr);

	// In case the color format has changed we will need to create the texture.
	if (!_cursor || _cursor->getFormat() != inputFormat || haveMask != wantMask) {
		delete _cursor;
		_cursor = nullptr;

		//GLenum glIntFormat, glFormat, glType;

		Graphics::PixelFormat textureFormat;
		if (inputFormat.bytesPerPixel == 1 || (inputFormat.aBits())) { //&& getGLPixelFormat(inputFormat, glIntFormat, glFormat, glType))) {
			// There is two cases when we can use the cursor format directly.
			// The first is when it's CLUT8, here color key handling can
			// always be applied because we use the alpha channel of
			// _defaultFormatAlpha for that.
			// The other is when the input format has alpha bits and
			// furthermore is directly supported.
			textureFormat = inputFormat;
		} else {
			textureFormat = _defaultFormatAlpha;
		}
		_cursor = createSurface(textureFormat, true, wantScaler, wantMask);
		assert(_cursor);

		//updateLinearFiltering();

//#ifdef USE_SCALERS
//		if (wantScaler) {
//			_cursor->setScaler(_currentState.scalerIndex, _currentState.scaleFactor);
//		}
//#endif
	}

	if (mask) {
		if (!_cursorMask) {
			maskFormat = _defaultFormatAlpha;
			_cursorMask = createSurface(maskFormat, true, wantScaler);
			assert(_cursorMask);

			//updateLinearFiltering();

//#ifdef USE_SCALERS
//			if (wantScaler) {
//				_cursorMask->setScaler(_currentState.scalerIndex, _currentState.scaleFactor);
//			}
//#endif
		}
	} else {
		delete _cursorMask;
		_cursorMask = nullptr;
	}

	Common::Point topLeftCoord(0, 0);
	Common::Point cursorSurfaceSize(w, h);

	// If the cursor is scalable, add a 1-texel transparent border.
	// This ensures that linear filtering falloff from the edge pixels has room to completely fade out instead of
	// being cut off at half-way.  Could use border clamp too, but GLES2 doesn't support that.
	if (!_cursorDontScale) {
		topLeftCoord = Common::Point(1, 1);
		cursorSurfaceSize += Common::Point(2, 2);
	}

	_cursor->allocate(cursorSurfaceSize.x, cursorSurfaceSize.y);
	if (_cursorMask)
		_cursorMask->allocate(cursorSurfaceSize.x, cursorSurfaceSize.y);

	_cursorHotspotX += topLeftCoord.x;
	_cursorHotspotY += topLeftCoord.y;

	if (inputFormat.bytesPerPixel == 1) {
		// For CLUT8 cursors we can simply copy the input data into the
		// texture.
		if (!_cursorDontScale)
			_cursor->fill(keycolor);
		
		_cursor->copyRectToTexture(topLeftCoord.x, topLeftCoord.y, w, h, buf, w * inputFormat.bytesPerPixel);

		if (mask) {
			// Construct a mask of opaque pixels
			Common::Array<byte> maskBytes;
			maskBytes.resize(cursorSurfaceSize.x * cursorSurfaceSize.y, 0);

			for (uint y = 0; y < h; y++) {
				for (uint x = 0; x < w; x++) {
					// The cursor pixels must be masked out for anything except opaque
					if (mask[y * w + x] == kCursorMaskOpaque)
						maskBytes[(y + topLeftCoord.y) * cursorSurfaceSize.x + topLeftCoord.x + x] = 1;
				}
			}

			_cursor->setMask(&maskBytes[0]);
		} else {
			_cursor->setMask(nullptr);
		}
	} else {
		// Otherwise it is a bit more ugly because we have to handle a key
		// color properly.

		Graphics::Surface *dst = _cursor->getSurface();
		const uint srcPitch = w * inputFormat.bytesPerPixel;

		// Copy the cursor data to the actual texture surface. This will make
		// sure that the data is also converted to the expected format.

		// Also multiply the color values with the alpha channel.
		// The pre-multiplication allows using a blend mode that prevents
		// color fringes due to filtering.

		if (!_cursorDontScale)
			_cursor->fill(0);

		byte *topLeftPixelPtr = static_cast<byte *>(dst->getBasePtr(topLeftCoord.x, topLeftCoord.y));

		if (dst->format.bytesPerPixel == 2) {
			if (inputFormat.bytesPerPixel == 2) {
				multiplyColorWithAlpha<uint16, uint16>((const byte *)buf, topLeftPixelPtr, w, h,
													   inputFormat, dst->format, srcPitch, dst->pitch, keycolor, _cursorUseKey);
			} else if (inputFormat.bytesPerPixel == 4) {
				multiplyColorWithAlpha<uint32, uint16>((const byte *)buf, topLeftPixelPtr, w, h,
													   inputFormat, dst->format, srcPitch, dst->pitch, keycolor, _cursorUseKey);
			}
		} else if (dst->format.bytesPerPixel == 4) {
			if (inputFormat.bytesPerPixel == 2) {
				multiplyColorWithAlpha<uint16, uint32>((const byte *)buf, topLeftPixelPtr, w, h,
													   inputFormat, dst->format, srcPitch, dst->pitch, keycolor, _cursorUseKey);
			} else if (inputFormat.bytesPerPixel == 4) {
				multiplyColorWithAlpha<uint32, uint32>((const byte *)buf, topLeftPixelPtr, w, h,
													   inputFormat, dst->format, srcPitch, dst->pitch, keycolor, _cursorUseKey);
			}
		}

		// Replace all non-opaque pixels with black pixels
		if (mask) {
			Graphics::Surface *cursorSurface = _cursor->getSurface();

			for (uint x = 0; x < w; x++) {
				for (uint y = 0; y < h; y++) {
					uint8 maskByte = mask[y * w + x];

					if (maskByte != kCursorMaskOpaque)
						cursorSurface->setPixel(x + topLeftCoord.x, y + topLeftCoord.y, 0);
				}
			}
		}

		// Flag the texture as dirty.
		_cursor->flagDirty();
	}

	if (_cursorMask && mask) {
		// Generate the multiply+invert texture.
		// We're generating this for a blend mode where source factor is ONE_MINUS_DST_COLOR and dest factor is ONE_MINUS_SRC_ALPHA
		// In other words, positive RGB channel values will add inverted destination pixels, positive alpha values will modulate
		// RGB+Alpha = Inverted   Alpha Only = Black   0 = No change

		Graphics::Surface *cursorSurface = _cursor->getSurface();
		Graphics::Surface *maskSurface = _cursorMask->getSurface();
		maskFormat = _cursorMask->getFormat();

		const Graphics::PixelFormat cursorFormat = cursorSurface->format;

		_cursorMask->fill(0);
		for (uint x = 0; x < w; x++) {
			for (uint y = 0; y < h; y++) {
				// See the description of renderCursor for an explanation of why this works the way it does.

				uint8 maskOpacity = 0xff;

				if (inputFormat.bytesPerPixel != 1) {
					uint32 cursorPixel = cursorSurface->getPixel(x + topLeftCoord.x, y + topLeftCoord.y);

					uint8 r, g, b;
					cursorFormat.colorToARGB(cursorPixel, maskOpacity, r, g, b);
				}

				uint8 maskInversionAdd = 0;

				uint8 maskByte = mask[y * w + x];
				if (maskByte == kCursorMaskTransparent)
					maskOpacity = 0;

				if (maskByte == kCursorMaskInvert) {
					maskOpacity = 0xff;
					maskInversionAdd = 0xff;
				}

				uint32 encodedMaskPixel = maskFormat.ARGBToColor(maskOpacity, maskInversionAdd, maskInversionAdd, maskInversionAdd);
				maskSurface->setPixel(x + topLeftCoord.x, y + topLeftCoord.y, encodedMaskPixel);
			}
		}

		_cursorMask->flagDirty();
	}

	// In case we actually use a palette set that up properly.
	if (inputFormat.bytesPerPixel == 1) {
		updateCursorPalette();
	}

	recalculateCursorScaling();
}

void MetalGraphicsManager::setCursorPalette(const byte *colors, uint start, uint num) {
	_cursorPaletteEnabled = true;

	memcpy(_cursorPalette + start * 3, colors, num * 3);
	updateCursorPalette();
}

// PaletteManager
void MetalGraphicsManager::setPalette(const byte *colors, uint start, uint num) {
	assert(_gameScreen->hasPalette());

	memcpy(_gamePalette + start * 3, colors, num * 3);
	_gameScreen->setPalette(start, num, colors);

	// We might need to update the cursor palette here.
	updateCursorPalette();
}

void MetalGraphicsManager::grabPalette(byte *colors, uint start, uint num) const {
	assert(_gameScreen->hasPalette());

	memcpy(colors, _gamePalette + start * 3, num * 3);
}
						   
Surface *MetalGraphicsManager::createSurface(const Graphics::PixelFormat &format, bool wantAlpha, bool wantScaler, bool wantMask) {
	if (format.bytesPerPixel == 1) {
		return new TextureCLUT8GPU(_device);
	}
	return new Texture(_device, format);
}

void MetalGraphicsManager::updateCursorPalette() {
	if (!_cursor || !_cursor->hasPalette()) {
		return;
	}

	if (_cursorPaletteEnabled) {
		_cursor->setPalette(0, 256, _cursorPalette);
	} else {
		_cursor->setPalette(0, 256, _gamePalette);
	}

	if (_cursorUseKey)
		_cursor->setColorKey(_cursorKeyColor);
}

void MetalGraphicsManager::recalculateCursorScaling() {
	if (!_cursor || !_gameScreen) {
		return;
	}

	uint cursorWidth = _cursor->getWidth();
	uint cursorHeight = _cursor->getHeight();

	// By default we use the unscaled versions.
	_cursorHotspotXScaled = _cursorHotspotX;
	_cursorHotspotYScaled = _cursorHotspotY;
	_cursorWidthScaled = cursorWidth;
	_cursorHeightScaled = cursorHeight;

	// In case scaling is actually enabled we will scale the cursor according
	// to the game screen.
	if (!_cursorDontScale) {
		const frac_t screenScaleFactorX = intToFrac(_gameDrawRect.width()) / _gameScreen->getWidth();
		const frac_t screenScaleFactorY = intToFrac(_gameDrawRect.height()) / _gameScreen->getHeight();

		_cursorHotspotXScaled = fracToInt(_cursorHotspotXScaled * screenScaleFactorX);
		_cursorWidthScaled    = fracToDouble(cursorWidth        * screenScaleFactorX);

		_cursorHotspotYScaled = fracToInt(_cursorHotspotYScaled * screenScaleFactorY);
		_cursorHeightScaled   = fracToDouble(cursorHeight       * screenScaleFactorY);
	}
}

} // end namespace Metal
