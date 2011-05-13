/*
 * Mousepad
 * Copyright Sean Stangl <sean.stangl@gmail.com> 2005-2010
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

#define CONFIG_FILENAME "mousepad.conf"

#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>

#include <gtk/gtk.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>

/******************************************************/

/* Motion, Button Constants */
#define MOTION_DAMP 1.0
#define POINTER_MOVEMENT_DELAY_MILLISECONDS 10
#define INPUT_DELAY_MILLISECONDS 20
#define JUMP_DELAY_MILLISECONDS 100
#define DOUBLE_TAP_TIME_MILLISECONDS 160
#define VELOCITY_MAX 30.0
#define INITIAL_VELOCITY 2

/* Keyboard states */
#define KEYBOARD_STATE_NONE 0
#define KEYBOARD_STATE_LEFT 1
#define KEYBOARD_STATE_UP 2
#define KEYBOARD_STATE_RIGHT 3
#define KEYBOARD_STATE_DOWN 4
#define KEYBOARD_STATE_UPLEFT 5
#define KEYBOARD_STATE_UPRIGHT 6
#define KEYBOARD_STATE_DOWNRIGHT 7
#define KEYBOARD_STATE_DOWNLEFT 8

/* Keyboard Indicator Constants */
#define CALCULATED_WIDTH screenWidth/5    /* screenWidth, screenHeight valid ints */
#define DISTANCE_FROM_CORNER 20

/******************************************************/

/* The pointer (mouse cursor) */
struct pntr
{
	signed int x,y; /* respective coordinates. */
	float xVel, yVel;
	float xAccel, yAccel;
} pointer; /* global pointer */

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

/* Stores the joystick configuration */
struct btnconfig
{
	/* using "unsigned" suppresses gcc warnings */
	unsigned char left, down, up, right, downleft, downright,
		upleft, upright, start, back;
};

/* State of the shift mechanism, global. */
char shiftPressed = False;

/* State of the keyboard mechanism, global. */
char keyboardState = KEYBOARD_STATE_NONE;
static pthread_mutex_t stateMutex = PTHREAD_MUTEX_INITIALIZER;

char mode = 0; /* Boolean mode. 0 if mouse mode, 1 if keyboard mode. Toggled by Start. */
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/******************************************************/

/* Moves the pointer relatively */
void movePointer (signed int xdist, signed int ydist)
{

	Display *display = XOpenDisplay(NULL);

	/* Garbage variables */
	Window tmpwindow;
	unsigned int tmp;
	signed int tmp2;

	/* Obtain the pointer's position. */
	XQueryPointer(display, RootWindow(display, DefaultScreen(display)), &tmpwindow, &tmpwindow, &pointer.x,
		&pointer.y, &tmp2, &tmp2, &tmp);

	/* Move the pointer. */
	XWarpPointer(display, None, RootWindow(display, DefaultScreen(display)), 0, 0, 0, 0, pointer.x+xdist, pointer.y+ydist);

	XFlush(display);
	XCloseDisplay(display);

}


/* Causes a mouse click, woo!
   Thanks to daniels in #xorg for recommending XTest
   instead of the hundred-line monstrosity I had before. */
void clickPointer (unsigned int button)
{
	Display *display = XOpenDisplay(NULL);

	XTestFakeButtonEvent(display, button, 1, 0);
	XTestFakeButtonEvent(display, button, 0, 0);

	XFlush(display);
	XCloseDisplay(display);
}

/* Closes the Focused Window */
void closeFocusedWindow ()
{
	Display *display = XOpenDisplay(NULL);
	Window focused;
	int revert;

	XGetInputFocus(display, &focused, &revert);

	if(focused != None)
	{
		XDestroyWindow(display, focused);
	}

	XCloseDisplay(display);
}


/* Sends a key press event to the current window */
void pressKey (unsigned int key)
{
	Display *display = XOpenDisplay(NULL);
	
	if (shiftPressed == True)
	{
		XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Shift_R), True, CurrentTime);
	}
	
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, key), True, CurrentTime);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, key), False, CurrentTime);
	
	if (shiftPressed == True)
	{
		XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Shift_R), False, CurrentTime);
		shiftPressed = False;
	}
	
	/* Set the time that this key was sent */
	clock_gettime(CLOCK_REALTIME, &keySentTime);
	
	XCloseDisplay(display);
}

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

/* Thread for the GTK-based keyboard indicator */
void *gtkThreadMain()
{
	
	GtkWidget *keyboardWindow;
	GtkWidget *image;
	GdkPixbuf *pixbuf_left, *pixbuf_left_shift,
			*pixbuf_upleft, *pixbuf_upleft_shift,
			*pixbuf_up, *pixbuf_up_shift,
			*pixbuf_upright, *pixbuf_upright_shift,
			*pixbuf_right, *pixbuf_right_shift,
			*pixbuf_down, *pixbuf_down_shift,
			*pixbuf_none;
	int windowIsShown = 0; /* Reduces processing burden */
	int imageShown = 0; /* Reduces processing burden */
	int internalShiftState = 0; /* Shift state displayed */
	
	/* Retrieve the resolution in pixels */
	Display *display = XOpenDisplay(NULL);
	const int screenHeight = XDisplayHeight(display, DefaultScreen(display));
	const int screenWidth = XDisplayWidth(display, DefaultScreen(display));
	XCloseDisplay(display);
	
	keyboardWindow = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_window_stick ((GtkWindow *)keyboardWindow);
	gtk_window_move ((GtkWindow *)keyboardWindow, screenWidth - CALCULATED_WIDTH - DISTANCE_FROM_CORNER, screenHeight - CALCULATED_WIDTH - DISTANCE_FROM_CORNER);
	
	/* Load images into pixbufs */
	pixbuf_left = gdk_pixbuf_new_from_file_at_size ("img/mousepad_left.png", CALCULATED_WIDTH, CALCULATED_WIDTH, NULL);
	pixbuf_left_shift = gdk_pixbuf_new_from_file_at_size ("img/mousepad_left_shift.png", CALCULATED_WIDTH, CALCULATED_WIDTH, NULL);
	
	pixbuf_upleft = gdk_pixbuf_new_from_file_at_size ("img/mousepad_upleft.png", CALCULATED_WIDTH, CALCULATED_WIDTH, NULL);
	pixbuf_upleft_shift = gdk_pixbuf_new_from_file_at_size ("img/mousepad_upleft_shift.png", CALCULATED_WIDTH, CALCULATED_WIDTH, NULL);
	
	pixbuf_up = gdk_pixbuf_new_from_file_at_size ("img/mousepad_up.png", CALCULATED_WIDTH, CALCULATED_WIDTH, NULL);
	pixbuf_up_shift = gdk_pixbuf_new_from_file_at_size ("img/mousepad_up_shift.png", CALCULATED_WIDTH, CALCULATED_WIDTH, NULL);
	
	pixbuf_upright = gdk_pixbuf_new_from_file_at_size ("img/mousepad_upright.png", CALCULATED_WIDTH, CALCULATED_WIDTH, NULL);
	pixbuf_upright_shift = gdk_pixbuf_new_from_file_at_size ("img/mousepad_upright_shift.png", CALCULATED_WIDTH, CALCULATED_WIDTH, NULL);
	
	pixbuf_right = gdk_pixbuf_new_from_file_at_size ("img/mousepad_right.png", CALCULATED_WIDTH, CALCULATED_WIDTH, NULL);
	pixbuf_right_shift = gdk_pixbuf_new_from_file_at_size ("img/mousepad_right_shift.png", CALCULATED_WIDTH, CALCULATED_WIDTH, NULL);
	
	pixbuf_down = gdk_pixbuf_new_from_file_at_size ("img/mousepad_down.png", CALCULATED_WIDTH, CALCULATED_WIDTH, NULL);
	pixbuf_down_shift = gdk_pixbuf_new_from_file_at_size ("img/mousepad_down_shift.png", CALCULATED_WIDTH, CALCULATED_WIDTH, NULL);
	
	pixbuf_none = gdk_pixbuf_new_from_file_at_size ("img/mousepad_none.png", CALCULATED_WIDTH, CALCULATED_WIDTH, NULL);
	
	/* Set default image */
	image = gtk_image_new_from_pixbuf (pixbuf_none);
	
	gtk_container_add (GTK_CONTAINER (keyboardWindow), image);
	gtk_widget_show (image);
	
	gtk_image_set_from_pixbuf ((GtkImage *)image, pixbuf_none);
	
	/* Broadcast end of thread initialization */
	pthread_cond_broadcast(&cond);
	
	/* Main loop */
	while (1)
	{
		
		/* If there is a set keyboard state and the window is not shown */
		if (mode == 1 && !windowIsShown)
		{
			gtk_widget_show (keyboardWindow);
			windowIsShown = 1;
		}
		else if (mode == 0 && windowIsShown)
		{
			gtk_widget_hide (keyboardWindow);
			windowIsShown = 0;
		}
		
		if (keyboardState != imageShown || shiftPressed != internalShiftState)
		{
		
			imageShown = keyboardState;
			internalShiftState = shiftPressed;
			
			gtk_image_clear((GtkImage *)image);
		
			switch (imageShown)
			{
				case KEYBOARD_STATE_NONE:
					gtk_image_set_from_pixbuf ((GtkImage *)image, pixbuf_none);
					break;
				case KEYBOARD_STATE_LEFT:
					if (internalShiftState == 0)
						gtk_image_set_from_pixbuf ((GtkImage *)image, pixbuf_left);
					else
						gtk_image_set_from_pixbuf ((GtkImage *)image, pixbuf_left_shift);
					break;
				case KEYBOARD_STATE_UPLEFT:
					if (internalShiftState == 0)
						gtk_image_set_from_pixbuf ((GtkImage *)image, pixbuf_upleft);
					else
						gtk_image_set_from_pixbuf ((GtkImage *)image, pixbuf_upleft_shift);
					break;
				case KEYBOARD_STATE_UP:
					if (internalShiftState == 0)
						gtk_image_set_from_pixbuf ((GtkImage *)image, pixbuf_up);
					else
						gtk_image_set_from_pixbuf ((GtkImage *)image, pixbuf_up_shift);
					break;
				case KEYBOARD_STATE_UPRIGHT:
					if (internalShiftState == 0)
						gtk_image_set_from_pixbuf ((GtkImage *)image, pixbuf_upright);
					else
						gtk_image_set_from_pixbuf ((GtkImage *)image, pixbuf_upright_shift);
					break;
				case KEYBOARD_STATE_RIGHT:
					if (internalShiftState == 0)
						gtk_image_set_from_pixbuf ((GtkImage *)image, pixbuf_right);
					else
						gtk_image_set_from_pixbuf ((GtkImage *)image, pixbuf_right_shift);
					break;
				case KEYBOARD_STATE_DOWN:
					if (internalShiftState == 0)
						gtk_image_set_from_pixbuf ((GtkImage *)image, pixbuf_down);
					else
						gtk_image_set_from_pixbuf ((GtkImage *)image, pixbuf_down_shift);
					break;
			}
			
		}
		
		while (gtk_events_pending())
			gtk_main_iteration();
		
		usleep(10000);
	}
	
	pthread_exit(NULL);
}


int main (int argc, char *argv[])
{
	char *device = "/dev/input/js0"; /* Joystick device, the only argument taken. */
	int joyFD; /* Handle for the Joystick */
	struct js_event jevent; /* Joystick event */
	int numButtons = 0; /* Number of buttons on the joystick */
	struct btn *button = NULL; /* Stores the properties of each button */
	int i, n; /* Standard iterators for C88 */
	struct timespec startTime; /* The time at which the motion started (used to calculate delta) */
	struct timespec lastTime; /* The time of the last mouse movement, used to maintain the rate of change */
	struct timespec currentTime; /* Temporary holder for the current time */
	int numButtonsPushed = 0; /* Iterator for individual button press logic */
	int buttonCombo = 0; /* Whether a combination was pressed or not. Used to prevent duplicate events. */
	FILE *configfile;
	char *configpath;
	struct btnconfig padconfig; /* Stores configuration file options: left, up, right, down, etc. */
	char tmpchar;
	pthread_t keyboardThread;
	
	gtk_init(&argc, &argv);
	
	/* Handle arguments manually without getopt(). */
	if (argc > 2) /* zu viele! */
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
	
	/* First, checks ~/.mousepad.conf
	   Second, checks /etc/mousepad.conf */
	for (i = 0; i < 2; i++)
	{
		if (i == 0)
		{
			if ((configpath = getenv("HOME")) == NULL)
			{
				fprintf(stderr, " Environment variable HOME not set.\n");
				return 1;
			}
			configpath = strcat(configpath, "/."CONFIG_FILENAME);
		}
		else
		{
			configpath = "/etc/"CONFIG_FILENAME;
		}

		if ((configfile = fopen(configpath, "r")) != NULL)
		{
			break;
		}
	}
	
	/* Die if no config file */
	if (configfile == NULL)
	{
		fprintf(stderr, " Couldn't find a pad configuration file.\n Run "PROGRAM_NAME"-config to build one.\n");
		return 1;
	}
	
	/* Zero the memory */
	memset (&padconfig, '\0', sizeof( struct btnconfig ));

	/* Set padconfig from config file */
	for (i = 0; feof(configfile) == 0; i++)
	{
		fread(&tmpchar, sizeof(char), 1, configfile);
		switch (tmpchar)
		{
			case '1':
				padconfig.back = i;
				break;
			case '3':
				padconfig.start = i;
				break;
			case 'a':
				padconfig.left = i;
				break;
			case 'c':
				padconfig.downright = i;
				break;
			case 'd':
				padconfig.right = i;
				break;
			case 'e':
				padconfig.upright = i;
				break;
			case 'q':
				padconfig.upleft = i;
				break;
			case 'w':
				padconfig.up = i;
				break;
			case 'x':
				padconfig.down = i;
				break;
			case 'z':
				padconfig.downleft = i;
				break;
		}
	}

	fclose(configfile);
	
	/* Create the Keyboard Indicator thread */
	pthread_create(&keyboardThread, NULL, gtkThreadMain, NULL);
	
	/* Wait for the GTK thread to initialize before continuing */
	pthread_mutex_lock(&stateMutex);
	pthread_cond_wait(&cond, &stateMutex);
	
	/* Loop until an available joystick is found */
	while (1)
	{

		/* Open Joystick device */
		while ((joyFD = open(device, O_RDONLY | O_NONBLOCK)) == -1)
		{
			usleep(10000);
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
			}
			
			
			/* Process Events */

			/* The choice was made to separate keyboard and mouse modes
			   to maintain the general organized nature of the code.
			   This leads to some redundancy, but improves maintenance. */

			/* Mouse Mode */
			if (mode == 0)
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
					if (i == padconfig.left)
					{
						if (button[i].state == 1 && button[padconfig.right].state == 0)
						{
							pointer.xAccel = -1;
							pointer.xVel = -INITIAL_VELOCITY;
						} else if (button[i].state == 0 && button[padconfig.right].state  == 1) {
							pointer.xAccel = 1;
							pointer.xVel = INITIAL_VELOCITY;
						} else {
							pointer.xAccel = 0;
							pointer.xVel = 0;
						}
					}
					else if (i == padconfig.upleft)
					{
						if (button[i].state == 1)
						{
							/* This can be the only button pressed. */
							for (n = 0; n < numButtons; n++)
							{
								if (button[n].state == 1)
								{
									numButtonsPushed += 1;
								}
							}
							
							if (numButtonsPushed == 1)
							{
								closeFocusedWindow();
							}
						}
					}
					else if (i == padconfig.up)
					{
						if (button[i].state == 1 && button[padconfig.down].state == 0)
						{
							pointer.yAccel = -1;
							pointer.yVel = -INITIAL_VELOCITY;
						} else if (button[i].state == 0 && button[padconfig.down].state == 1) {
							pointer.yAccel = 1;
							pointer.yVel = INITIAL_VELOCITY;
						} else {
							pointer.yAccel = 0;
							pointer.yVel = 0;
						}
					}
					else if (i == padconfig.upright)
					{
						if (button[i].state == 1)
						{
							/* This can be the only button pressed. */
							for (n = 0; n < numButtons; n++)
							{
								if (button[n].state == 1)
								{
									numButtonsPushed += 1;
								}
							}
							
							if (numButtonsPushed == 1)
							{
								pressKey(XK_Page_Up);
							}
						}
					}
					else if (i == padconfig.right)
					{
						if (button[i].state == 1 && button[padconfig.left].state  == 0)
						{
							pointer.xAccel = 1;
							pointer.xVel = INITIAL_VELOCITY;
						} else if (button[i].state == 0 && button[padconfig.left].state  == 1) {
							pointer.xAccel = -1;
							pointer.xVel = -INITIAL_VELOCITY;
						} else {
							pointer.xAccel = 0;
							pointer.xVel = 0;
						}
					}
					else if (i == padconfig.downright)
					{
						if (button[i].state == 1)
						{
							/* This can be the only button pressed. */
							for (n = 0; n < numButtons; n++)
							{
								if (button[n].state == 1)
								{
									numButtonsPushed += 1;
								}
							}
							
							if (numButtonsPushed == 1)
							{
								pressKey(XK_Page_Down);
							}
						}
					}
					else if (i == padconfig.down)
					{
						if (button[i].state == 1 && button[padconfig.up].state  == 0)
						{
							pointer.yAccel = 1;
							pointer.yVel = INITIAL_VELOCITY;
						} else if (button[i].state == 0 && button[padconfig.up].state  == 1) {
							pointer.yAccel = -1;
							pointer.yVel = -INITIAL_VELOCITY;
						} else {
							pointer.yAccel = 0;
							pointer.yVel = 0;
						}
					}
					else if (i == padconfig.start)
					{
						if (button[i].state == 1)
						{
							/* Zero the mouse motion */
							pointer.xVel = 0;
							pointer.yVel = 0;
							pointer.xAccel = 0;
							pointer.yAccel = 0;
							
							/* Set keyboard mode */
							mode = 1;
						}
					}

					clock_gettime(CLOCK_REALTIME, &startTime);
					button[i].processed = 1;
					numButtonsPushed = 0;

					/* A button has just been depressed, and a jump combo broken */
					if (button[i].state == 0) {
						buttonCombo = 0;
					}
					
				}
		
				/* Button Combinations */
				if (button[padconfig.left].state  == 1 && button[padconfig.right].state  == 1 &&
					diffMilliseconds(button[padconfig.left].pressedTime, button[padconfig.right].pressedTime) <= JUMP_DELAY_MILLISECONDS &&
					buttonCombo == 0)
				{
					clickPointer(Button1);
					buttonCombo = 1;
				}
		
				if (button[padconfig.up].state  == 1 && button[padconfig.down].state == 1 &&
					diffMilliseconds(button[padconfig.up].pressedTime, button[padconfig.down].pressedTime) <= JUMP_DELAY_MILLISECONDS &&
					buttonCombo == 0)
				{
					clickPointer(3);
					buttonCombo = 1;
				}
				
				/* Movement Logic */
				if (pointer.xAccel != 0 || pointer.yAccel != 0)
				{
					clock_gettime(CLOCK_REALTIME, &currentTime);
					if (diffMilliseconds(currentTime, lastTime) >= POINTER_MOVEMENT_DELAY_MILLISECONDS)
					{
						if (abs((int)pointer.xVel) >= (int)VELOCITY_MAX)
						{
							pointer.xVel = VELOCITY_MAX*pointer.xAccel;
						}
						else if (pointer.xAccel != 0)
						{
							pointer.xVel += pointer.xAccel*((float)diffMilliseconds(currentTime, startTime)/1000.0*MOTION_DAMP);
						}
		
						if (abs((int)pointer.yVel) >= (int)VELOCITY_MAX)
						{
							pointer.yVel = VELOCITY_MAX*pointer.yAccel;
						}
						else if (pointer.yAccel != 0)
						{
							pointer.yVel += pointer.yAccel*((float)diffMilliseconds(currentTime, startTime)/1000.0*MOTION_DAMP);
						}
						
						//printf("C: %d , S: %d, D: %i, TD: %i xVel:%f\n", (int)((abs(currentTime.tv_sec)*1000)+abs(((currentTime.tv_nsec)/1000000))), (int)((abs(startTime.tv_sec)*1000)+abs(((startTime.tv_nsec)/1000000))), diffMilliseconds(currentTime, startTime), (int)((abs(currentTime.tv_sec)*1000)+abs(((currentTime.tv_nsec)/1000000))) - (int)((abs(startTime.tv_sec)*1000)+abs(((startTime.tv_nsec)/1000000))), pointer.xVel);
		
						movePointer((int)pointer.xVel, (int)pointer.yVel);
						clock_gettime(CLOCK_REALTIME, &lastTime);
					}
				}

			}

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
								pressKey(XK_Left);
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
									pressKey(XK_m);
									break;
								case KEYBOARD_STATE_RIGHT:
									pressKey(XK_r);
									break;
								case KEYBOARD_STATE_DOWN:
									pressKey(XK_w);
									break;
								case KEYBOARD_STATE_UPLEFT:
									pressKey(XK_6);
									break;
								case KEYBOARD_STATE_UPRIGHT:
									shiftPressed = True;
									pressKey(XK_question);
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
									pressKey(XK_a);
									break;
								case KEYBOARD_STATE_UP:
									pressKey(XK_n);
									break;
								case KEYBOARD_STATE_RIGHT:
									pressKey(XK_s);
									break;
								case KEYBOARD_STATE_DOWN:
									pressKey(XK_x);
									break;
								case KEYBOARD_STATE_UPRIGHT:
									pressKey(XK_comma);
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
								pressKey(XK_Up);
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
									pressKey(XK_b);
									break;
								case KEYBOARD_STATE_RIGHT:
									pressKey(XK_t);
									break;
								case KEYBOARD_STATE_DOWN:
									pressKey(XK_y);
									break;
								case KEYBOARD_STATE_UPLEFT:
									pressKey(XK_0);
									break;
								case KEYBOARD_STATE_UPRIGHT:
									pressKey(XK_period);
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
									pressKey(XK_c);
									break;
								case KEYBOARD_STATE_UP:
									pressKey(XK_h);
									break;
								case KEYBOARD_STATE_RIGHT:
									pressKey(XK_u);
									break;
								case KEYBOARD_STATE_DOWN:
									pressKey(XK_z);
									break;
								case KEYBOARD_STATE_UPLEFT:
									pressKey(XK_1);
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
								pressKey(XK_Right);
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
									pressKey(XK_d);
									break;
								case KEYBOARD_STATE_UP:
									pressKey(XK_i);
									break;
								case KEYBOARD_STATE_DOWN:
									pressKey(XK_BackSpace);
									break;
								case KEYBOARD_STATE_UPLEFT:
									pressKey(XK_2);
									break;
								case KEYBOARD_STATE_UPRIGHT:
									pressKey(XK_7);
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
									pressKey(XK_e);
									break;
								case KEYBOARD_STATE_UP:
									pressKey(XK_j);
									break;
								case KEYBOARD_STATE_RIGHT:
									pressKey(XK_o);
									break;
								case KEYBOARD_STATE_DOWN:
									shiftPressed = False;
									pressKey(XK_slash);
									break;
								case KEYBOARD_STATE_UPLEFT:
									pressKey(XK_3);
									break;
								case KEYBOARD_STATE_UPRIGHT:
									pressKey(XK_8);
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
								pressKey(XK_Down);
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
									pressKey(XK_f);
									break;
								case KEYBOARD_STATE_UP:
									pressKey(XK_k);
									break;
								case KEYBOARD_STATE_RIGHT:
									pressKey(XK_p);
									break;
								case KEYBOARD_STATE_UPLEFT:
									pressKey(XK_4);
									break;
								case KEYBOARD_STATE_UPRIGHT:
									pressKey(XK_9);
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
									pressKey(XK_g);
									break;
								case KEYBOARD_STATE_UP:
									pressKey(XK_l);
									break;
								case KEYBOARD_STATE_RIGHT:
									pressKey(XK_q);
									break;
								case KEYBOARD_STATE_DOWN:
									pressKey(XK_v);
									break;
								case KEYBOARD_STATE_UPLEFT:
									pressKey(XK_5);
									break;
								case KEYBOARD_STATE_UPRIGHT:
									pressKey(XK_apostrophe);
									break;
							}
						}
					}
					else if (i == padconfig.start && button[i].state == 1)
					{
						mode = 0;
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
					pressKey(XK_space);
					buttonCombo = 1;
				}
		
				if (button[padconfig.up].state  == 1 && button[padconfig.down].state == 1 &&
					diffMilliseconds(button[padconfig.up].pressedTime, button[padconfig.down].pressedTime) <= JUMP_DELAY_MILLISECONDS &&
					buttonCombo == 0)
				{
					pressKey(XK_Return);
					buttonCombo = 1;
				}
				
			}

			/* Powernap */
			usleep(5000);
		}
	
	/* Joystick disconnected */
	close(joyFD);
	
	/* Reset default modes */
	mode = 0;
	keyboardState = 0;
	
	}
	
	return 0;
}
