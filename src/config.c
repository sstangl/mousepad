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
		char *path = alloca(strlen(home) + strlen(CONFIG_FILENAME) + 1);
		strcpy(path, home);
		strcat(path, CONFIG_FILENAME);
		
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

/* Fill in the padconfig structure by reading in the configuration file. */
int config_read(FILE *f, struct btnconfig *padconfig)
{
	memset(padconfig, 0x0, sizeof(struct btnconfig));

	for (int i = 0; !feof(f); i++) {
		char c;
		fread(&c, sizeof(char), 1, f);

		/*
		 * These values correspond to the position of the DDR keys,
		 * if you imagine them on the left-hand side of the QWERTY keyboard.
		 */
		switch (c) {
			case 'a': padconfig->left      = i; break;
			case 'q': padconfig->upleft    = i; break;
			case 'w': padconfig->up        = i; break;
			case 'e': padconfig->upright   = i; break;
			case 'd': padconfig->right     = i; break;
			case 'c': padconfig->downright = i; break;
			case 'x': padconfig->down      = i; break;
			case 'z': padconfig->downleft  = i; break;
			case '1': padconfig->back      = i; break;
			case '3': padconfig->start     = i; break;
			default: 
				return -1;
		}
	}
	
	return 0;
}
