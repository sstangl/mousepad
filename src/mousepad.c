/*
 * mousepad.c
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

#define PROGRAM_NAME "mousepad"
#define VERSION_NUMBER "0.3"

#include "config.h"
#include "keyboard.h"
#include "mouse.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <gtk/gtk.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>

/******************************************************/

/* Motion, Button Constants */
#define INPUT_DELAY_MILLISECONDS 20
#define JUMP_DELAY_MILLISECONDS 100
#define DOUBLE_TAP_TIME_MILLISECONDS 160

/******************************************************/

/* Structure for each button */
struct btn
{
	char state; /* False, True for Not pressed, Pressed */
	char processed; /* Whether a state change has been processed or not */
	struct timespec pressedTime; /* Time when the button was pressed */
	struct timespec pressedTimeBefore; /* Previous time when the button was pressed */
}; /* Array of all buttons and their properties */

/* keySentTime is used to ensure that sending an arrowkey in keyboard mode
   does not occur unless the particular key is exclusively pressed. */
struct timespec keySentTime; /* The time when a keystroke (of any kind) was sent */

/* State of the shift mechanism, global. */
char shiftPressed = False;

char mode = 0; /* Boolean mode. 0 if mouse mode, 1 if keyboard mode. Toggled by Start. */

/******************************************************/

/* Returns the difference in milliseconds between two times */
int diffMilliseconds (struct timespec time1, struct timespec time2)
{
	return abs(1000*(time1.tv_sec-time2.tv_sec)+(time1.tv_nsec-time2.tv_nsec)/1000000);
}

/* Returns the number of milliseconds elapsed */
int getMilliseconds (struct timespec time)
{
	return (1000*time.tv_sec + time.tv_nsec/1000000);
}


int main (int argc, char *argv[])
{
	char *device = "/dev/input/js0"; /* Joystick device, the only argument taken. */
	int joyFD; /* Handle for the Joystick */
	struct js_event jevent; /* Joystick event */
	int numButtons = 0; /* Number of buttons on the joystick */
	struct btn *button = NULL; /* Stores the properties of each button */
	FILE *configfile;
	
	gtk_init(&argc, &argv);
	
	/* Handle arguments manually without getopt(). */
	if (argc > 2)
	{
		fprintf(stderr, PROGRAM_NAME": Too many arguments.\n"); 
		return 1;
	}
	else if (argc >= 2)
	{
		if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
		{
			fprintf(stdout, "Usage: %s [Joystick Device]\n", argv[0]);
			return 0;
		} else {
			device = argv[1];
		}
	}


	/* Initialize joystick. */
	// TODO: Make this blocking and have a separate thread for cursor.
	if ((joyFD = open(device, O_RDONLY | O_NONBLOCK)) < 0)
	{
		fprintf(stderr, " Could not open joystick device.\n");
		return -1;
	}
		
	/* Retrieve the number of buttons */
	ioctl(joyFD, JSIOCGBUTTONS, &numButtons);
	if (numButtons <= 0)
		return -1;

	/* Allocate an array to store button information */
	button = (struct btn *) calloc(numButtons, sizeof( struct btn ));

	/* Map from jevent.number to button bitfield. */
	int *joymap = malloc(numButtons * sizeof(int));


	/* Read in configuration file */
	configfile = config_open();
	
	if (configfile == NULL)
	{
		fprintf(stderr, " Couldn't find a pad configuration file.\n Run "PROGRAM_NAME"-config to build one.\n");
		return 1;
	}

	if (config_read(configfile, numButtons, joymap) < 0) {
		fprintf(stderr, " Error parsing configuration file.\n");
		return 1;
	}
	config_close(configfile);
	
	Display *display = XOpenDisplay(NULL);
	if (mouse_init(display) < 0) return -1;
	if (keyboard_init(display) < 0) return -1;
	
	while (1)
	{
		mouse_begin();
		int buttons = 0;

		/* Main loop */
		while (1)
		{
			/* Update button values */
			read(joyFD, &jevent, sizeof(struct js_event));
			
			if (errno == ENODEV)
			{
				break;
			}

			int changed = 0;

			if ((jevent.type & ~JS_EVENT_INIT) == JS_EVENT_BUTTON)
			{
				/* If change in state */
				if (button[jevent.number].state != jevent.value)
				{
					/* Unpressing pushes the previous pressedTime down one level,
					   pressing itself just overwrites. */
					if (button[jevent.number].state == 0)
						button[jevent.number].pressedTimeBefore = button[jevent.number].pressedTime;
					
					/* updated pressedTime with the clock */
					clock_gettime(CLOCK_REALTIME, &button[jevent.number].pressedTime);
					button[jevent.number].processed = 0;
					button[jevent.number].state = jevent.value;

					/* Update changed and buttonstate TODO */
					changed = joymap[jevent.number];
					if (jevent.value)
						buttons &= ~changed;
					else
						buttons |= changed;
				}
			}


			/* Process Events */
			if (mode == 0) {
				if (changed)
					mouse_event(buttons, changed);
				mouse_tick();
			} else {
				if (changed)
					keyboard_event(buttons, changed);
			}

			/* Powernap */
			usleep(5000);
		}
	
		/* Joystick disconnected */
		close(joyFD);
	
		/* Reset default modes */
		mouse_end();
		mode = 0;
		// TODO: keyboard_begin();
	}
	
	return 0;
}

