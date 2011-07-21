/*
 * mouse.h
 * Copyright Sean Stangl <sean.stangl@gmail.com> 2005-2011
 *
 * This file is part of Mousepad.
 *
 * Mousepad is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mousepad is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mousepad.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __mousepad_mousepad_h__
#define __mousepad_mousepad_h__

/* 
 * Button defines.
 * Button state is transmitted as an integer, of which 10 bits are used,
 *  each corresponding to a button state.
 */
#define BUTTON_LEFT      (0x1)
#define BUTTON_UPLEFT    (0x1 << 1)
#define BUTTON_UP        (0x1 << 2)
#define BUTTON_UPRIGHT   (0x1 << 3)
#define BUTTON_RIGHT     (0x1 << 4)
#define BUTTON_DOWNRIGHT (0x1 << 5)
#define BUTTON_DOWN      (0x1 << 6)
#define BUTTON_DOWNLEFT  (0x1 << 7)
#define BUTTON_START     (0x1 << 8)
#define BUTTON_BACK      (0x1 << 9)

/* Bitfield of pad button toggles, using the above defines. */
typedef int buttonstate_t;

/* Bitfield of pad button toggles, but only one bit is set. */
typedef int button_t;

#endif /* __mousepad_mousepad_h__ */

