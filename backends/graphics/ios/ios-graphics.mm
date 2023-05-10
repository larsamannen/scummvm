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

#include "backends/graphics/opengl/pipelines/pipeline.h"
#include "backends/graphics/ios/ios-graphics.h"

#include <Foundation/Foundation.h>


IOSGraphicsManager::IOSGraphicsManager() {

	// Initialize our OpenGL ES context.
	initSurface();
}

IOSGraphicsManager::~IOSGraphicsManager() {
	deinitSurface();
}

void IOSGraphicsManager::initSurface() {

	_context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

	// In case creating the OpenGL ES context failed, we will error out here.
	if (_context == nil) {
		//fprintf(stderr, "%s\nCould not create OpenGL ES context.");
		//abort();
	}

	if ([EAGLContext setCurrentContext:_context]) {
		// glEnableClientState(GL_TEXTURE_COORD_ARRAY); printOpenGLError();
		// glEnableClientState(GL_VERTEX_ARRAY); printOpenGLError();
		//[self setupOpenGL];
	}


//	if (JNI::egl_bits_per_pixel == 16) {
		// We default to RGB565 and RGBA5551 which is closest to what we setup in Java side
	notifyContextCreate(OpenGL::kContextGLES2,
			Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0),
			Graphics::PixelFormat(2, 5, 5, 5, 1, 11, 6, 1, 0));
//	} else {
		// If not 16, this must be 24 or 32 bpp so make use of them
//		notifyContextCreate(OpenGL::kContextGLES2,
//#ifdef SCUMM_BIG_ENDIAN
//				Graphics::PixelFormat(3, 8, 8, 8, 0, 16, 8, 0, 0),
//				Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0)
//#else
//				Graphics::PixelFormat(3, 8, 8, 8, 0, 0, 8, 16, 0),
//				Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24)
//#endif
//		);
//	}

	//handleResize(JNI::egl_surface_width, JNI::egl_surface_height);
}

void IOSGraphicsManager::deinitSurface() {
	if (!_context)
		return;

	//LOGD("deinitializing 2D surface");

	notifyContextDestroy();

	//JNI::deinitSurface();
}

void IOSGraphicsManager::updateScreen() {
	//ENTER();

	if (!_context)
		return;

	OpenGLGraphicsManager::updateScreen();
}

void IOSGraphicsManager::displayMessageOnOSD(const Common::U32String &msg) {
	//ENTER("%s", msg.encode().c_str());

	//JNI::displayMessageOnOSD(msg);
}

void IOSGraphicsManager::showOverlay(bool inGUI) {
	if (_overlayVisible && inGUI == _overlayInGUI)
		return;

	OpenGL::OpenGLGraphicsManager::showOverlay(inGUI);
}

void IOSGraphicsManager::hideOverlay() {
	if (!_overlayVisible)
		return;

	OpenGL::OpenGLGraphicsManager::hideOverlay();
}

float IOSGraphicsManager::getHiDPIScreenFactor() const {
	// TODO: Use JNI to get DisplayMetrics.density, which according to the documentation
	// seems to be what we want.
	// "On a medium-density screen, DisplayMetrics.density equals 1.0; on a high-density
	//  screen it equals 1.5; on an extra-high-density screen, it equals 2.0; and on a
	//  low-density screen, it equals 0.75. This figure is the factor by which you should
	//  multiply the dp units in order to get the actual pixel count for the current screen."

	return 2.f;
}

bool IOSGraphicsManager::loadVideoMode(uint requestedWidth, uint requestedHeight, const Graphics::PixelFormat &format) {
	//ENTER("%d, %d, %s", requestedWidth, requestedHeight, format.toString().c_str());

	// We get this whenever a new resolution is requested. Since Android is
	// using a fixed output size we do nothing like that here.
	// TODO: Support screen rotation
	return true;
}

void IOSGraphicsManager::refreshScreen() {
	//ENTER();

	// Last minute draw of touch controls
	//dynamic_cast<OSystem_Android *>(g_system)->getTouchControls().draw();

	//JNI::swapBuffers();
}


bool IOSGraphicsManager::notifyMousePosition(Common::Point &mouse) {
	mouse.x = CLIP<int16>(mouse.x, _activeArea.drawRect.left, _activeArea.drawRect.right);
	mouse.y = CLIP<int16>(mouse.y, _activeArea.drawRect.top, _activeArea.drawRect.bottom);

	setMousePosition(mouse.x, mouse.y);
	mouse = convertWindowToVirtual(mouse.x, mouse.y);

	return true;
}

IOSGraphics::State IOSGraphicsManager::getState() const {
	State state;

	state.screenWidth   = getWidth();
	state.screenHeight  = getHeight();
	state.aspectRatio   = getFeatureState(OSystem::kFeatureAspectRatioCorrection);
	state.fullscreen    = getFeatureState(OSystem::kFeatureFullscreenMode);
	state.cursorPalette = getFeatureState(OSystem::kFeatureCursorPalette);
#ifdef USE_RGB_COLOR
	state.pixelFormat   = getScreenFormat();
#endif
	return state;
}

bool IOSGraphicsManager::setState(const IOSGraphics::State &state) {
	beginGFXTransaction();

#ifdef USE_RGB_COLOR
		initSize(state.screenWidth, state.screenHeight, &state.pixelFormat);
#else
		initSize(state.screenWidth, state.screenHeight, nullptr);
#endif
		setFeatureState(OSystem::kFeatureAspectRatioCorrection, state.aspectRatio);
		setFeatureState(OSystem::kFeatureFullscreenMode, state.fullscreen);
		setFeatureState(OSystem::kFeatureCursorPalette, state.cursorPalette);

	return endGFXTransaction() == OSystem::kTransactionSuccess;
}
