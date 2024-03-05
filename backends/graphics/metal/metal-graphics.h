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

#ifndef BACKENDS_GRAPHICS_METAL_METAL_GRAPHICS_H
#define BACKENDS_GRAPHICS_METAL_METAL_GRAPHICS_H

#include "backends/graphics/windowed.h"
#include <Metal/Metal.h>

namespace Metal {

class MetalGraphicsManager : virtual public WindowedGraphicsManager {
public:
	MetalGraphicsManager();
	virtual ~MetalGraphicsManager();

protected:
	// Windowed
	bool gameNeedsAspectRatioCorrection() const override;
	void handleResizeImpl(const int width, const int height) override;

	// GraphicsManager
	bool hasFeature(OSystem::Feature f) const override;
	void setFeatureState(OSystem::Feature f, bool enable) override;
	bool getFeatureState(OSystem::Feature f) const override;
	
	const OSystem::GraphicsMode *getSupportedGraphicsModes() const override;
	
#ifdef USE_RGB_COLOR
	Graphics::PixelFormat getScreenFormat() const override;
	Common::List<Graphics::PixelFormat> getSupportedFormats() const override;
#endif
	
	// PaletteManager
	void setPalette(const byte *colors, uint start, uint num) override;
	void grabPalette(byte *colors, uint start, uint num) const override;
	
private:
	Graphics::PixelFormat _currentFormat;
};

} // End of namespace Metal

#endif
