/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */
#include "common/scummsys.h"
 
#include "common/config-manager.h"
#include "common/events.h"
#include "common/file.h"
#include "common/random.h"
#include "common/fs.h"
#include "common/keyboard.h"

#include "graphics/cursorman.h"
#include "graphics/surface.h"
#include "graphics/pixelformat.h"

#include "engines/util.h"
#include "engines/advancedDetector.h"

#include "deskadv/console.h"
#include "deskadv/deskadv.h"

namespace Deskadv {

DeskadvEngine::DeskadvEngine(OSystem *syst, const DeskadvGameDescription *gameDesc) : Engine(syst), _gameDescription(gameDesc) {
#if 0
	DebugMan.addDebugChannel(kDebugResource, "Resource", "Resource Debug Flag");
	DebugMan.addDebugChannel(kDebugSaveLoad, "Saveload", "Saveload Debug Flag");
	DebugMan.addDebugChannel(kDebugScript, "Script", "Script Debug Flag");
	DebugMan.addDebugChannel(kDebugText, "Text", "Text Debug Flag");
	DebugMan.addDebugChannel(kDebugCollision, "Collision", "Collision Debug Flag");
	DebugMan.addDebugChannel(kDebugGraphics, "Graphics", "Graphics Debug Flag");
	DebugMan.addDebugChannel(kDebugSound, "Sound", "Sound Debug Flag");
#endif
	const Common::FSNode gameDataDir(ConfMan.get("path"));
	SearchMan.addSubDirectoryMatching(gameDataDir, "bitmaps");
	SearchMan.addSubDirectoryMatching(gameDataDir, "sfx");

	_rnd = new Common::RandomSource("deskadv");

	_console = 0;
	_gfx = 0;
	_snd = 0;
	_resource = 0;

	// TODO: Add Sound Mixer
}

DeskadvEngine::~DeskadvEngine() {

	delete _rnd;
	delete _console;

	delete _snd;
	delete _resource;
	delete _gfx;
}

Common::Error DeskadvEngine::run() {
	Common::Event event;

	_gfx = new Gfx(this);
	_snd = new Sound(this);
	_console = new DeskadvConsole(this);
	_resource = new Resource(this);

	Common::String resourceFilename;
	switch (getGameType()) {
	case GType_Indy:
		resourceFilename = "desktop.daw";
		break;
	case GType_Yoda:
		if (getFeatures() & ADGF_DEMO)
			resourceFilename = "yodademo.dta";
		else
			resourceFilename = "yodesk.dta";
		break;
	default:
		error("Unknown Game Type for Resource File...");
		break;
	}
	debug(1, "resourceFilename: \"%s\"", resourceFilename.c_str());
	if (!_resource->load(resourceFilename.c_str(), getGameType() == GType_Yoda))
		error("Loading from Resource File failed!");

	// Load Mouse Cursors
	switch (getGameType()) {
	case GType_Indy:
		_gfx->loadCursors("deskadv.exe");
		break;
	case GType_Yoda:
		if (getFeatures() & ADGF_DEMO)
			_gfx->loadCursors("yodademo.exe");
		else
			_gfx->loadCursors("yodesk.exe");
		break;
	default:
		error("Unknown Game Type for Executable File...");
		break;
	}
	_gfx->setDefaultCursor();
	CursorMan.showMouse(true);

	// TODO: Current BMPDecoder does not support this bitmap variant.
	//if (getGameType() == GType_Indy) {
	//	_gfx->loadBMP("wallppr.bmp", 0, 0);
	//	_gfx->updateScreen();
	//}
	_gfx->drawScreenOutline();
	_gfx->drawStartup();

	bool InvScrollGrabbed = false;
	while (!shouldQuit()) {
		//debug(1, "Main Loop Tick...");
		_gfx->updateScreen();

		while (g_system->getEventManager()->pollEvent(event)) {
			switch (event.type) {
			case Common::EVENT_LBUTTONDOWN:
				if (_gfx->getInvScrThumb()->contains(event.mouse)) {
					debug(1, "Inventory Scroll Thumb Clicked.");
					InvScrollGrabbed = true;
				}
				if (_gfx->getInvScrUp()->contains(event.mouse)) {
					debug(1, "Inventory Scroll Up Arrow Clicked.");
				}
				if (_gfx->getInvScrDown()->contains(event.mouse)) {
					debug(1, "Inventory Scroll Down Arrow Clicked.");
				}
				break;

			case Common::EVENT_LBUTTONUP:
				InvScrollGrabbed = false;
				break;

			case Common::EVENT_RBUTTONDOWN:
				break;

			case Common::EVENT_MOUSEMOVE:
				if (InvScrollGrabbed == true) {
					debug(1, "Moving Scroll Bar.");
					// FIXME: Move Position and redraw.
				}
				break;

			case Common::EVENT_KEYDOWN:
				switch (event.kbd.keycode) {
				case Common::KEYCODE_d:
					if (event.kbd.hasFlags(Common::KBD_CTRL)) {
						// Start the debugger
						getDebugger()->attach();
						getDebugger()->onFrame();
					}
					break;

				case Common::KEYCODE_ESCAPE:
					quitGame();
					break;

				default:
					break;
				}
				break;

			case Common::EVENT_QUIT:
			case Common::EVENT_RETURN_TO_LAUNCHER:
				return Common::kNoError;
				//quitGame();
				//break;

			default:
				break;
			}
		}

		_system->delayMillis(50);
	}

	return Common::kNoError;
}

} // End of namespace Deskadv
