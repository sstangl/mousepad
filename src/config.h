/*
 * config.h
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

#ifndef __mousepad_config_h__

#include <stdio.h>

#define CONFIG_FILENAME "mousepad.conf"

/* Joystick configuration mapping from JEvent to button. */
struct btnconfig
{
	unsigned char left, upleft, up, upright, right, downright,
	              down, downleft, start, back;
};

FILE *config_open();
int config_read(FILE *f, int n, int *joymap);
int config_close();

#endif /* __mousepad_config_h__ */

