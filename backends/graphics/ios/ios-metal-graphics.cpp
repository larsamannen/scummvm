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

#include "backends/graphics/ios/ios-metal-graphics.h"
#include "backends/platform/ios7/ios7_osys_main.h"


iOSMetalGraphicsManager::iOSMetalGraphicsManager() {
	initSurface();
}

iOSMetalGraphicsManager::~iOSMetalGraphicsManager() {
	deinitSurface();
}

void iOSMetalGraphicsManager::initSurface() {
	OSystem_iOS7 *sys = dynamic_cast<OSystem_iOS7 *>(g_system);

	// Create Metal Device
	MTL::Device *device = MTL::CreateSystemDefaultDevice();
	// Assign the device to the Core Animation Layer to connect it to the screen
	sys->assignMetalDevice(device);
	
	notifyContextCreate(device,
	// Currently iOS runs the ARMs in little-endian mode but prepare if
	// that is changed in the future.
#ifdef SCUMM_LITTLE_ENDIAN
	Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24),
	Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));
#else
	Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0),
	Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0));
#endif
	handleResize(sys->getScreenWidth(), sys->getScreenHeight());

}

CA::MetalDrawable *iOSMetalGraphicsManager::getNextDrawable() {
	return dynamic_cast<OSystem_iOS7 *>(g_system)->nextDrawable();
}

void iOSMetalGraphicsManager::deinitSurface() {
}

void iOSMetalGraphicsManager::notifyResize(const int width, const int height) {
	handleResize(width, height);
}

iOSCommonGraphics::State iOSMetalGraphicsManager::getState() const {
	iOSCommonGraphics::State state;

	state.screenWidth   = 0;
	state.screenHeight  = 0;
	state.aspectRatio   = getFeatureState(OSystem::kFeatureAspectRatioCorrection);
	state.cursorPalette = getFeatureState(OSystem::kFeatureCursorPalette);
#ifdef USE_RGB_COLOR
	state.pixelFormat   = getScreenFormat();
#endif
	return state;}

bool iOSMetalGraphicsManager::setState(const iOSCommonGraphics::State &state) {
	return false;
}

bool iOSMetalGraphicsManager::notifyMousePosition(Common::Point &mouse) {

	return true;
}

float iOSMetalGraphicsManager::getHiDPIScreenFactor() const {
	return dynamic_cast<OSystem_iOS7 *>(g_system)->getSystemHiDPIScreenFactor();
}

void iOSMetalGraphicsManager::showOverlay(bool inGUI) {
	if (_overlayVisible && inGUI == _overlayInGUI)
		return;

	// Don't change touch mode when not changing mouse coordinates
	if (inGUI) {
		_old_touch_mode = dynamic_cast<OSystem_iOS7 *>(g_system)->getCurrentTouchMode();
		// not in 3D, in overlay
		dynamic_cast<OSystem_iOS7 *>(g_system)->applyTouchSettings(false, true);
	} else if (_overlayInGUI) {
		// Restore touch mode active before overlay was shown
		dynamic_cast<OSystem_iOS7 *>(g_system)->setCurrentTouchMode(static_cast<TouchMode>(_old_touch_mode));
	}

	MetalGraphicsManager::showOverlay(inGUI);
}

void iOSMetalGraphicsManager::hideOverlay() {
	if (_overlayInGUI) {
		// Restore touch mode active before overlay was shown
		dynamic_cast<OSystem_iOS7 *>(g_system)->setCurrentTouchMode(static_cast<TouchMode>(_old_touch_mode));
	}

	MetalGraphicsManager::hideOverlay();
}
