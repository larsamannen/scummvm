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

#ifndef BACKENDS_GRAPHICS_IOS_IOS_GRAPHICS_H
#define BACKENDS_GRAPHICS_IOS_IOS_GRAPHICS_H

#include "common/scummsys.h"
#include "backends/graphics/opengl/opengl-graphics.h"

#include <OpenGLES/EAGL.h>
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

class IOSGraphics: public GraphicsManager {
public:
	virtual ~IOSGraphics() {}

	virtual void initSurface() = 0;
	virtual void deinitSurface() = 0;

	virtual Common::Point getMousePosition() = 0;
	virtual bool notifyMousePosition(Common::Point &mouse) = 0;

	/**
	 * A (subset) of the graphic manager's state. This is used when switching
	 * between different Android graphic managers at runtime.
	 */
	struct State {
		int screenWidth, screenHeight;
		bool aspectRatio;
		bool fullscreen;
		bool cursorPalette;

#ifdef USE_RGB_COLOR
		Graphics::PixelFormat pixelFormat;
#endif
	};

	/**
	 * Gets the current state of the graphics manager.
	 */
	virtual State getState() const = 0;

	/**
	 * Sets up a basic state of the graphics manager.
	 */
	virtual bool setState(const State &state) = 0;
};

class IOSGraphicsManager :
	public OpenGL::OpenGLGraphicsManager, public IOSGraphics {
public:
		IOSGraphicsManager();
	virtual ~IOSGraphicsManager();

	virtual void initSurface() override;
	virtual void deinitSurface() override;

	virtual IOSGraphics::State getState() const override;
	virtual bool setState(const IOSGraphics::State &state) override;

	void updateScreen() override;

	void displayMessageOnOSD(const Common::U32String &msg) override;

	virtual bool notifyMousePosition(Common::Point &mouse) override;
	virtual Common::Point getMousePosition() override { return Common::Point(_cursorX, _cursorY); }

	float getHiDPIScreenFactor() const override;

protected:
	void setSystemMousePosition(const int x, const int y) override {}

	void showOverlay(bool inGUI) override;
	void hideOverlay() override;


	bool loadVideoMode(uint requestedWidth, uint requestedHeight, const Graphics::PixelFormat &format) override;

	void refreshScreen() override;
		
private:
	EAGLContext *_context;
};

#endif
