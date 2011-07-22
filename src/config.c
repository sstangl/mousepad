/*
 * config.c
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

#include "config.h"
#include "mousepad.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Look for the configuration file, first in ~/.CONFIG_FILENAME,
 *  then in /etc/CONFIG_FILENAME.
 * Returns NULL if neither file exists or if fopen() fails.
 */
FILE *config_open()
{
	char *home = getenv("HOME");
	if (home != NULL) {
		FILE *f;
		char *path = alloca(strlen(home) + strlen("/."CONFIG_FILENAME) + 1);
		strcpy(path, home);
		strcat(path, "/."CONFIG_FILENAME);
		
		if ((f = fopen(path, "r")) != NULL)
			return f;
	}

	return fopen("/etc/"CONFIG_FILENAME, "r");
}

/* Close the configuration file. */
int config_close(FILE *f)
{
	if (f == NULL)
		return -1;
	return fclose(f);
}

/* 
 * Create a map from jevent.number to button bitfield.
 * n is the length of joymap.
 */
int config_read(FILE *f, int n, int *joymap)
{
	for (int i = 0; !feof(f) && i < n; i++) {
		char c;
		fread(&c, sizeof(char), 1, f);

		/*
		 * These values correspond to the position of the DDR keys,
		 * if you imagine them on the left-hand side of the QWERTY keyboard.
		 */
		switch (c) {
			case 'a': joymap[i] = BUTTON_LEFT; break;
			case 'q': joymap[i] = BUTTON_UPLEFT; break;
			case 'w': joymap[i] = BUTTON_UP; break;
			case 'e': joymap[i] = BUTTON_UPRIGHT; break;
			case 'd': joymap[i] = BUTTON_RIGHT; break;
			case 'c': joymap[i] = BUTTON_DOWNRIGHT; break;
			case 'x': joymap[i] = BUTTON_DOWN; break;
			case 'z': joymap[i] = BUTTON_DOWNLEFT; break;
			case '1': joymap[i] = BUTTON_BACK; break;
			case '3': joymap[i] = BUTTON_START; break;
			case ' ':  /* fall through */
			case '\r': /* fall through */
			case '\n': joymap[i] = 0x0; break;
			default:
				return -1;
		}
	}
	
	return 0;
}

