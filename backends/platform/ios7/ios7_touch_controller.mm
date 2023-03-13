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

// Disable symbol overrides so that we can use system headers.
#define FORBIDDEN_SYMBOL_ALLOW_ALL

#include "backends/platform/ios7/ios7_touch_controller.h"
#include "backends/platform/ios7/ios7_video.h"

@implementation TouchController {
	UITouch *_firstTouch;
	UITouch *_secondTouch;
}

@dynamic view;
@dynamic isConnected;

- (id)initWithView:(iPhoneView *)view {
	self = [super initWithView:view];

	_firstTouch = NULL;
	_secondTouch = NULL;

	// Touches should always be present in iOS view
	[self setIsConnected:YES];

	return self;
}

- (UITouch *)secondTouchOtherTouchThan:(UITouch *)touch in:(NSSet *)set {
	NSArray *all = [set allObjects];
	for (UITouch *t in all) {
		if (t != touch) {
			return t;
		}
	}
	return nil;
}

- (void)handleTouch:(NSSet *)allTouches didBegan:(BOOL)began {
	if (allTouches.count) {
		int x, y;
		InputEvent inputEvent;
		_firstTouch = [allTouches anyObject];

		if (allTouches.count == 1) {
			if (!_firstTouch || _firstTouch.type == UITouchTypeIndirect) {
				return;
			}
#ifdef __IPHONE_13_4
			if (@available(iOS 13.4, *)) {
				// Touchpads are considered as UITouchTypeIndirectPointers. They are handled by the mouse controller class
				if (_firstTouch.type == UITouchTypeIndirectPointer) {
					return;
				}
			}
#endif
			inputEvent = (began ? kInputTouchFirstDown : kInputTouchFirstUp);
		} else /* count > 1 */ {
			_secondTouch = [self secondTouchOtherTouchThan:_firstTouch in:allTouches];
			if (!_secondTouch) {
				return;
			}
			inputEvent = (began ? kInputTouchSecondDown : kInputTouchSecondUp);
		}
		// Only set valid mouse coordinates in games
		if (![[self view] getMouseCoords:[_firstTouch locationInView:[self view]] eventX:&x eventY:&y]) {
			return;
		}
		[[self view] addEvent:InternalEvent(inputEvent, x, y)];
	}
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
	NSSet *allTouches = [event allTouches];
	[self handleTouch:allTouches didBegan:YES];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
	NSSet *allTouches = [event allTouches];
	for (UITouch *touch in allTouches) {
		if (touch == _firstTouch || touch == _secondTouch) {
			CGPoint touchLocation = [touch locationInView:[self view]];
			CGPoint previousTouchLocation = [touch previousLocationInView:[self view]];
			int previousX, previousY, currentX, currentY;
			if (![[self view] getMouseCoords:previousTouchLocation eventX:&previousX eventY:&previousY]) {
				return;
			}
			if (![[self view] getMouseCoords:touchLocation eventX:&currentX eventY:&currentY]) {
				return;
			}
			int deltaX = previousX - currentX;
			int deltaY = previousY - currentY;
			[[self view] addEvent:InternalEvent(kInputMouseDelta, (int)deltaX, (int)deltaY)];
		}
	}
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
	NSSet *allTouches = [event allTouches];
	[self handleTouch:allTouches didBegan:NO];
	_firstTouch = NULL;
	_secondTouch = NULL;
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
	_firstTouch = NULL;
	_secondTouch = NULL;
}

@end
