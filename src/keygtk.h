/*
 * keygtk.h
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

#ifndef __mousepad_keygtk_h__

#include <X11/X.h>
#include <X11/Xlib.h>

int keygtk_init(Display *d);
void keygtk_window_show();
void keygtk_window_hide();
int keygtk_set_layout(int layout);

#endif /* __mousepad_keygtk_h__ */

