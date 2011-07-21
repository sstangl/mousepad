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
	struct btnconfig padconfig; /* Stores configuration file options: left, up, right, down, etc. */
	
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


	/* Read in configuration file */
	configfile = config_open();
	
	if (configfile == NULL)
	{
		fprintf(stderr, " Couldn't find a pad configuration file.\n Run "PROGRAM_NAME"-config to build one.\n");
		return 1;
	}

	if (config_read(configfile, &padconfig) < 0) {
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

		/* Open Joystick device */
		// TODO: Make this blocking and have a separate thread for cursor.
		if ((joyFD = open(device, O_RDONLY | O_NONBLOCK)) < 0)
		{
			fprintf(stderr, " Could not open joystick device.\n");
			return -1;
		}
		
		/* Deallocate button array if it was previously allocated */
		if (numButtons > 0)
		{
			free(button);
		}

		/* Retrieve the number of buttons */
		ioctl(joyFD, JSIOCGBUTTONS, &numButtons);
		
		/* Allocate an array to store button information */
		button = (struct btn *) calloc(numButtons, sizeof( struct btn ));
		
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
					{
						button[jevent.number].pressedTimeBefore = button[jevent.number].pressedTime;
					}
					
					/* updated pressedTime with the clock */
					clock_gettime(CLOCK_REALTIME, &button[jevent.number].pressedTime);
					
					button[jevent.number].processed = 0;

					button[jevent.number].state = jevent.value;
				}

				/*
				 * Translate jevent.number to the button bitfield.
				 * This is a temporary hack because I feel lazy.
				 */
				if (jevent.number == padconfig.left)
					changed = BUTTON_LEFT;
				else if (jevent.number == padconfig.upleft)
					changed = BUTTON_UPLEFT;
				else if (jevent.number == padconfig.up)
					changed = BUTTON_UP;
				else if (jevent.number == padconfig.upright)
					changed = BUTTON_UPRIGHT;
				else if (jevent.number == padconfig.right)
					changed = BUTTON_RIGHT;
				else if (jevent.number == padconfig.downright)
					changed = BUTTON_DOWNRIGHT;
				else if (jevent.number == padconfig.down)
					changed = BUTTON_DOWN;
				else if (jevent.number == padconfig.downleft)
					changed = BUTTON_DOWNLEFT;
				else if (jevent.number == padconfig.start)
					changed = BUTTON_START;
				else if (jevent.number == padconfig.back)
					changed = BUTTON_BACK;
			}

			/* 
			 * Translate this stupid state to buttons/changed.
			 * This is a temporary hack because I feel lazy.
			 */
			buttonstate_t buttons = 0;

			buttons |= button[padconfig.left].state ? BUTTON_LEFT : 0;
			buttons |= button[padconfig.upleft].state ? BUTTON_UPLEFT : 0;
			buttons |= button[padconfig.up].state ? BUTTON_UP : 0;
			buttons |= button[padconfig.upright].state ? BUTTON_UPRIGHT : 0;
			buttons |= button[padconfig.right].state ? BUTTON_RIGHT : 0;
			buttons |= button[padconfig.downright].state ? BUTTON_DOWNRIGHT : 0;
			buttons |= button[padconfig.down].state ? BUTTON_DOWN : 0;
			buttons |= button[padconfig.downleft].state ? BUTTON_DOWNLEFT : 0;
			buttons |= button[padconfig.start].state ? BUTTON_START : 0;
			buttons |= button[padconfig.back].state ? BUTTON_BACK : 0;


			/* Process Events */

			/* Mouse Mode */
			if (mode == 0)
			{
				if (changed)
					mouse_event(buttons, changed);
				mouse_tick();
			}
#if 0
			/* Keyboard Mode */
			else
			{
				/* Individual Button Pushes */
				for (i = 0; i < numButtons; i++)
				{
					/* Force a delay */
					clock_gettime(CLOCK_REALTIME, &currentTime);
					if (diffMilliseconds(currentTime, button[i].pressedTime) <= INPUT_DELAY_MILLISECONDS || button[i].processed == 1)
					{
						continue;
					}
					
					/* Handle the different keys */
					if (i == padconfig.left && buttonCombo == 0)
					{
						if (button[i].state == 1
							&& diffMilliseconds(button[i].pressedTime, button[i].pressedTimeBefore) <= DOUBLE_TAP_TIME_MILLISECONDS
							&& keyboardState == KEYBOARD_STATE_NONE)
						{
							if ((getMilliseconds(button[i].pressedTimeBefore) > getMilliseconds(keySentTime)))
							{
								keyboard_press(XK_Left);
							}
						}
						else if (button[i].state == 1)
						{
							switch (keyboardState)
							{
								case KEYBOARD_STATE_NONE:
									keyboardState = KEYBOARD_STATE_LEFT;
									break;
								case KEYBOARD_STATE_UP:
									keyboard_press(XK_m);
									break;
								case KEYBOARD_STATE_RIGHT:
									keyboard_press(XK_r);
									break;
								case KEYBOARD_STATE_DOWN:
									keyboard_press(XK_w);
									break;
								case KEYBOARD_STATE_UPLEFT:
									keyboard_press(XK_6);
									break;
								case KEYBOARD_STATE_UPRIGHT:
									shiftPressed = True;
									keyboard_press(XK_question);
									break;
							}
						}
						else if (keyboardState == KEYBOARD_STATE_LEFT)
						{	
							keyboardState = KEYBOARD_STATE_NONE;
						}
					}
					else if (i == padconfig.upleft && buttonCombo == 0)
					{
						if (button[i].state == 1)
						{
							switch (keyboardState)
							{
								case KEYBOARD_STATE_NONE:
									keyboardState = KEYBOARD_STATE_UPLEFT;
									break;
								case KEYBOARD_STATE_LEFT:
									keyboard_press(XK_a);
									break;
								case KEYBOARD_STATE_UP:
									keyboard_press(XK_n);
									break;
								case KEYBOARD_STATE_RIGHT:
									keyboard_press(XK_s);
									break;
								case KEYBOARD_STATE_DOWN:
									keyboard_press(XK_x);
									break;
								case KEYBOARD_STATE_UPRIGHT:
									keyboard_press(XK_comma);
									break;
							}
						}
						else if (keyboardState == KEYBOARD_STATE_UPLEFT)
						{
							keyboardState = KEYBOARD_STATE_NONE;
						}
					}
					else if (i == padconfig.up && buttonCombo == 0)
					{
						if (button[i].state == 1
							&& diffMilliseconds(button[i].pressedTime, button[i].pressedTimeBefore) <= DOUBLE_TAP_TIME_MILLISECONDS
							&& keyboardState == KEYBOARD_STATE_NONE)
						{
							if ((getMilliseconds(button[i].pressedTimeBefore) > getMilliseconds(keySentTime)))
							{
								keyboard_press(XK_Up);
							}
						}
						else if (button[i].state == 1)
						{
							switch (keyboardState)
							{
								case KEYBOARD_STATE_NONE:
									keyboardState = KEYBOARD_STATE_UP;
									break;
								case KEYBOARD_STATE_LEFT:
									keyboard_press(XK_b);
									break;
								case KEYBOARD_STATE_RIGHT:
									keyboard_press(XK_t);
									break;
								case KEYBOARD_STATE_DOWN:
									keyboard_press(XK_y);
									break;
								case KEYBOARD_STATE_UPLEFT:
									keyboard_press(XK_0);
									break;
								case KEYBOARD_STATE_UPRIGHT:
									keyboard_press(XK_period);
									break;
							}
						}
						else if (keyboardState == KEYBOARD_STATE_UP)
						{
							keyboardState = KEYBOARD_STATE_NONE;
						}
					}
					else if (i == padconfig.upright && buttonCombo == 0)
					{
						if (button[i].state == 1)
						{
							switch (keyboardState)
							{
								case KEYBOARD_STATE_NONE:
									keyboardState = KEYBOARD_STATE_UPRIGHT;
									break;
								case KEYBOARD_STATE_LEFT:
									keyboard_press(XK_c);
									break;
								case KEYBOARD_STATE_UP:
									keyboard_press(XK_h);
									break;
								case KEYBOARD_STATE_RIGHT:
									keyboard_press(XK_u);
									break;
								case KEYBOARD_STATE_DOWN:
									keyboard_press(XK_z);
									break;
								case KEYBOARD_STATE_UPLEFT:
									keyboard_press(XK_1);
									break;
							}
						}
						else if (keyboardState == KEYBOARD_STATE_UPRIGHT)
						{
							keyboardState = KEYBOARD_STATE_NONE;
						}
					}
					else if (i == padconfig.right && buttonCombo == 0)
					{
						if (button[i].state == 1
							&& diffMilliseconds(button[i].pressedTime, button[i].pressedTimeBefore) <= DOUBLE_TAP_TIME_MILLISECONDS
							&& keyboardState == KEYBOARD_STATE_NONE)
						{
							if ((getMilliseconds(button[i].pressedTimeBefore) > getMilliseconds(keySentTime)))
							{
								keyboard_press(XK_Right);
							}
						}
						else if (button[i].state == 1)
						{
							switch (keyboardState)
							{
								case KEYBOARD_STATE_NONE:
									keyboardState = KEYBOARD_STATE_RIGHT;
									break;
								case KEYBOARD_STATE_LEFT:
									keyboard_press(XK_d);
									break;
								case KEYBOARD_STATE_UP:
									keyboard_press(XK_i);
									break;
								case KEYBOARD_STATE_DOWN:
									keyboard_press(XK_BackSpace);
									break;
								case KEYBOARD_STATE_UPLEFT:
									keyboard_press(XK_2);
									break;
								case KEYBOARD_STATE_UPRIGHT:
									keyboard_press(XK_7);
									break;
							}
						}
						else if (keyboardState == KEYBOARD_STATE_RIGHT)
						{
							keyboardState = KEYBOARD_STATE_NONE;
						}
					}
					else if (i == padconfig.downright && buttonCombo == 0)
					{
						if (button[i].state == 1)
						{
							switch (keyboardState)
							{
								case KEYBOARD_STATE_LEFT:
									keyboard_press(XK_e);
									break;
								case KEYBOARD_STATE_UP:
									keyboard_press(XK_j);
									break;
								case KEYBOARD_STATE_RIGHT:
									keyboard_press(XK_o);
									break;
								case KEYBOARD_STATE_DOWN:
									shiftPressed = False;
									keyboard_press(XK_slash);
									break;
								case KEYBOARD_STATE_UPLEFT:
									keyboard_press(XK_3);
									break;
								case KEYBOARD_STATE_UPRIGHT:
									keyboard_press(XK_8);
									break;
							}
						}
					}
					else if (i == padconfig.down && buttonCombo == 0)
					{
						if (button[i].state == 1
							&& diffMilliseconds(button[i].pressedTime, button[i].pressedTimeBefore) <= DOUBLE_TAP_TIME_MILLISECONDS
							&& keyboardState == KEYBOARD_STATE_NONE)
						{
							if ((getMilliseconds(button[i].pressedTimeBefore) > getMilliseconds(keySentTime)))
							{
								keyboard_press(XK_Down);
							}
						}
						else if (button[i].state == 1)
						{
							switch (keyboardState)
							{
								case KEYBOARD_STATE_NONE:
									keyboardState = KEYBOARD_STATE_DOWN;
									break;
								case KEYBOARD_STATE_LEFT:
									keyboard_press(XK_f);
									break;
								case KEYBOARD_STATE_UP:
									keyboard_press(XK_k);
									break;
								case KEYBOARD_STATE_RIGHT:
									keyboard_press(XK_p);
									break;
								case KEYBOARD_STATE_UPLEFT:
									keyboard_press(XK_4);
									break;
								case KEYBOARD_STATE_UPRIGHT:
									keyboard_press(XK_9);
									break;
							}
						}
						else if (keyboardState == KEYBOARD_STATE_DOWN)
						{
							keyboardState = KEYBOARD_STATE_NONE;
						}
					}
					else if (i == padconfig.downleft && buttonCombo == 0)
					{
						if (button[i].state == 1)
						{
							switch (keyboardState)
							{
								case KEYBOARD_STATE_LEFT:
									keyboard_press(XK_g);
									break;
								case KEYBOARD_STATE_UP:
									keyboard_press(XK_l);
									break;
								case KEYBOARD_STATE_RIGHT:
									keyboard_press(XK_q);
									break;
								case KEYBOARD_STATE_DOWN:
									keyboard_press(XK_v);
									break;
								case KEYBOARD_STATE_UPLEFT:
									keyboard_press(XK_5);
									break;
								case KEYBOARD_STATE_UPRIGHT:
									keyboard_press(XK_apostrophe);
									break;
							}
						}
					}
					else if (i == padconfig.start && button[i].state == 1)
					{
						mode = 0;
						mouse_begin();
						keyboardState = 0;
						shiftPressed = False;
					}
					else if (i == padconfig.back && button[i].state == 1)
					{
						/* Toggle shift */
						shiftPressed = shiftPressed ^ 1;
					}

					clock_gettime(CLOCK_REALTIME, &startTime);
					button[i].processed = 1;
					numButtonsPushed = 0;

					/* A button has just been unpressed, and the combo broken */
					if (button[i].state == 0) {
						buttonCombo = 0;
					}
				}
				
				/* Button Combinations */
				if (button[padconfig.left].state  == 1 && button[padconfig.right].state  == 1 &&
					diffMilliseconds(button[padconfig.left].pressedTime, button[padconfig.right].pressedTime) <= JUMP_DELAY_MILLISECONDS &&
					buttonCombo == 0)
				{
					keyboard_press(XK_space);
					buttonCombo = 1;
				}
		
				if (button[padconfig.up].state  == 1 && button[padconfig.down].state == 1 &&
					diffMilliseconds(button[padconfig.up].pressedTime, button[padconfig.down].pressedTime) <= JUMP_DELAY_MILLISECONDS &&
					buttonCombo == 0)
				{
					keyboard_press(XK_Return);
					buttonCombo = 1;
				}
				
			}
#endif

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

