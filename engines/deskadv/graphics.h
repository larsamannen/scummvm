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

#ifndef DESKADV_GRAPHICS_H
#define DESKADV_GRAPHICS_H

#include "graphics/surface.h"
#include "graphics/font.h"
#include "graphics/wincursor.h"
#include "common/formats/winexe_ne.h"
#include "common/formats/winexe_pe.h"
#include "common/rect.h"

namespace Deskadv {

class Gfx {
public:
	Gfx(DeskadvEngine *vm);
	virtual ~Gfx(void);

	void updateScreen(void);
	void drawTile(uint32 ref, uint8 x, uint8 y);
	void loadCursors(const char *filename);
	void setDefaultCursor(void);
	void changeCursor(uint id);
	void loadBMP(const char *filename, uint x, uint y);

	void drawScreenOutline(void);
	void drawStartup(void);
	void drawWeapon(uint32 ref);
	void drawWeaponPower(uint8 level);
	void eraseInventoryItem(uint slot);
	void drawInventoryItem(uint slot, uint32 iconRef, const char *name);
	const Common::Rect *getInvScrUp(void);
	const Common::Rect *getInvScrDown(void);
	Common::Rect *getInvScrThumb(void) { return InvScrThumb; };
	void drawDirectionArrows(bool left, bool up, bool right, bool down);
	void drawHealthMeter(uint level);

	// Debug Routines
	void viewPalette(void);

private:
	DeskadvEngine *_vm;

	Graphics::Surface *_screen;
	Common::NEResources _ne;
	Common::PEResources _pe;
	Common::Array<Common::WinResourceID> _cursor;
	const Graphics::Font *_font;

	// Inventory Scroll Bar
	Common::Rect *InvScrThumb;

	void drawTileInt(uint32 ref, uint x, uint y, byte transparentColor);
	void drawShadowFrame(const Common::Rect *rect, bool recessed, bool firstInverse, uint thickness);
	void drawFrameCircle(Graphics::Surface *target, const Common::Point centre, uint radius, uint color);
	void drawFilledCircle(Graphics::Surface *target, const Common::Point centre, uint radius, uint color);
};

} // End of namespace Deskadv

#endif
