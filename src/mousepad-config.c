/*
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
 * along with Mousepad.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <glade/glade.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>

#define PROGRAM_NAME "mousepad-config"
#define CONFIG_FILENAME "mousepad.conf"

#define LEFT 1
#define UPLEFT 2
#define UP 3
#define UPRIGHT 4
#define RIGHT 5
#define DOWNRIGHT 6
#define DOWN 7
#define DOWNLEFT 8
#define BACK 9
#define START 10

/******************************************/
           /*Global Variables*/

int joyFD;
int numButtons = 0;
char *configpath;

/* GTK stuff */
GtkLabel *lblBtnLeft, *lblBtnUpLeft, *lblBtnUp,
	*lblBtnUpRight, *lblBtnRight, *lblBtnDownRight,
	*lblBtnDown, *lblBtnDownLeft, *lblBtnBack,
	*lblBtnStart;
GtkStatusbar *statusbar;
GtkWindow *mousepadWindow;

/* Stores the joystick configuration */
int padconfig[11];

struct btn
{
	char state; /* False, True for Not pressed, Pressed */
} *button; /* Array of all buttons and their properties */

/******************************************/

int get_joystick_input()
{
	struct js_event jevent;
	
	gtk_statusbar_pop(statusbar, 0);
	gtk_statusbar_push(statusbar, 0, "Press a button on the pad to set it.");
	while (gtk_events_pending())
		gtk_main_iteration();
	
	/* Flush the event stack */
	while (read(joyFD, &jevent, sizeof(struct js_event)))
	{
		if (errno == EAGAIN)
			break;
	}
	
	while (1)
	{
		read(joyFD, &jevent, sizeof(struct js_event));
		
		if ((jevent.type & ~JS_EVENT_INIT) == JS_EVENT_BUTTON && jevent.value == 1)
		{
			gtk_statusbar_pop(statusbar, 0);
			return jevent.number;
		}
	}
}

/* Ensures that only the `check` button controls a key */
void ensure_button_set (int buttonNum, int check)
{
	int i, oldButton = -1;
	
	for (i = 0; i < numButtons; i++)
	{
		if (padconfig[i] == check && i != buttonNum)
		{
			oldButton = i;
		}
	}
	
	if (oldButton != -1)
	{
		padconfig[oldButton] = 0;

		switch (oldButton)
		{
			case LEFT:
				gtk_label_set_label(lblBtnLeft, "Left");
				break;
			case UPLEFT:
				gtk_label_set_label(lblBtnUpLeft, "Up Left");
				break;
			case UP:
				gtk_label_set_label(lblBtnUp, "Up");
				break;
			case UPRIGHT:
				gtk_label_set_label(lblBtnUpRight, "Up Right");
				break;
			case RIGHT:
				gtk_label_set_label(lblBtnRight, "Right");
				break;
			case DOWNRIGHT:
				gtk_label_set_label(lblBtnDownRight, "Down Right");
				break;
			case DOWN:
				gtk_label_set_label(lblBtnDown, "Down");
				break;
			case DOWNLEFT:
				gtk_label_set_label(lblBtnDownLeft, "Down Left");
				break;
			case BACK:
				gtk_label_set_label(lblBtnBack, "Back");
				break;
			case START:
				gtk_label_set_label(lblBtnStart, "Start");
				break;
		}
	}

}


void on_mousepadWindow_destroy (GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
}

void on_quit_activate (GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
}

int on_save_activate (GtkWidget *widget, gpointer data)
{
	int buttonMap[numButtons + 1];
	FILE *configfile;
	int i;
	
	for (i = 0; i < numButtons + 1; i++)
	{
		buttonMap[i] = 0;
	}
	
	for (i = 1; i < 11; i++)
	{
		buttonMap[padconfig[i]] = i;
	}
	
	configfile = fopen(configpath, "w");
	
	for (i = 0; i < numButtons + 1; i++)
	{
		//printf("%i, %i\n", i, buttonMap[i]);
		switch (buttonMap[i])
		{
			case 0:
				fwrite(" ", sizeof(char), 1, configfile);
				break;
			case LEFT:
				fwrite("a", sizeof(char), 1, configfile);
				break;
			case UPLEFT:
				fwrite("q", sizeof(char), 1, configfile);
				break;
			case UP:
				fwrite("w", sizeof(char), 1, configfile);
				break;
			case UPRIGHT:
				fwrite("e", sizeof(char), 1, configfile);
				break;
			case RIGHT:
				fwrite("d", sizeof(char), 1, configfile);
				break;
			case DOWNRIGHT:
				fwrite("c", sizeof(char), 1, configfile);
				break;
			case DOWN:
				fwrite("x", sizeof(char), 1, configfile);
				break;
			case DOWNLEFT:
				fwrite("z", sizeof(char), 1, configfile);
				break;
			case BACK:
				fwrite("1", sizeof(char), 1, configfile);
				break;
			case START:
				fwrite("3", sizeof(char), 1, configfile);
				break;
		}
		
	}
	fclose(configfile);
	
	gtk_statusbar_pop(statusbar, 0);
	gtk_statusbar_push(statusbar, 0, strcat(configpath," saved."));
	while (gtk_events_pending())
		gtk_main_iteration();
	
	return 0;
}

void reset_data ()
{
	int i;
	for (i = 1; i < 11; i++)
	{
		padconfig[i] = -1;
	}
	
	gtk_label_set_label(lblBtnLeft, "Left");
	gtk_label_set_label(lblBtnUpLeft, "Up Left");
	gtk_label_set_label(lblBtnUp, "Up");
	gtk_label_set_label(lblBtnUpRight, "Up Right");
	gtk_label_set_label(lblBtnRight, "Right");
	gtk_label_set_label(lblBtnDownRight, "Down Right");
	gtk_label_set_label(lblBtnDown, "Down");
	gtk_label_set_label(lblBtnDownLeft, "Down Left");
	gtk_label_set_label(lblBtnBack, "Back");
	gtk_label_set_label(lblBtnStart, "Start");
}

void on_undo_settings_activate (GtkWidget *widget, gpointer data)
{
	reset_data();
	gtk_statusbar_pop(statusbar, 0);
	gtk_statusbar_push(statusbar, 0, "All buttons reset.");
	while (gtk_events_pending())
		gtk_main_iteration();
}

void on_btnLeft_pressed (GtkWidget *widget, gpointer data)
{
	char tmpchar[30];
	padconfig[LEFT] = get_joystick_input();
	snprintf(tmpchar, 30, "Left\n[Button %d]", padconfig[LEFT]);
	gtk_label_set_label(lblBtnLeft, tmpchar);
	ensure_button_set(LEFT, padconfig[LEFT]);
}

void on_btnUpLeft_pressed (GtkWidget *widget, gpointer data)
{
	char tmpchar[30];
	padconfig[UPLEFT] = get_joystick_input();
	snprintf(tmpchar, 30, "Up Left\n[Button %d]", padconfig[UPLEFT]);
	gtk_label_set_label(lblBtnUpLeft, tmpchar);
	ensure_button_set(UPLEFT, padconfig[UPLEFT]);
}

void on_btnUp_pressed (GtkWidget *widget, gpointer data)
{
	char tmpchar[30];
	padconfig[UP] = get_joystick_input();
	snprintf(tmpchar, 30, "Up\n[Button %d]", padconfig[UP]);
	gtk_label_set_label(lblBtnUp, tmpchar);
	ensure_button_set(UP, padconfig[UP]);
}

void on_btnUpRight_pressed (GtkWidget *widget, gpointer data)
{
	char tmpchar[30];
	padconfig[UPRIGHT] = get_joystick_input();
	snprintf(tmpchar, 30, "Up Right\n[Button %d]", padconfig[UPRIGHT]);
	gtk_label_set_label(lblBtnUpRight, tmpchar);
	ensure_button_set(UPRIGHT, padconfig[UPRIGHT]);
}

void on_btnRight_pressed (GtkWidget *widget, gpointer data)
{
	char tmpchar[30];
	padconfig[RIGHT] = get_joystick_input();
	snprintf(tmpchar, 30, "Right\n[Button %d]", padconfig[RIGHT]);
	gtk_label_set_label(lblBtnRight, tmpchar);
	ensure_button_set(RIGHT, padconfig[RIGHT]);
}

void on_btnDownRight_pressed (GtkWidget *widget, gpointer data)
{
	char tmpchar[30];
	padconfig[DOWNRIGHT] = get_joystick_input();
	snprintf(tmpchar, 30, "Down Right\n[Button %d]", padconfig[DOWNRIGHT]);
	gtk_label_set_label(lblBtnDownRight, tmpchar);
	ensure_button_set(DOWNRIGHT, padconfig[DOWNRIGHT]);
}

void on_btnDown_pressed (GtkWidget *widget, gpointer data)
{
	char tmpchar[30];
	padconfig[DOWN] = get_joystick_input();
	snprintf(tmpchar, 30, "Down\n[Button %d]", padconfig[DOWN]);
	gtk_label_set_label(lblBtnDown, tmpchar);
	ensure_button_set(DOWN, padconfig[DOWN]);
}

void on_btnDownLeft_pressed (GtkWidget *widget, gpointer data)
{
	char tmpchar[30];
	padconfig[DOWNLEFT] = get_joystick_input();
	snprintf(tmpchar, 30, "Down Left\n[Button %d]", padconfig[DOWNLEFT]);
	gtk_label_set_label(lblBtnDownLeft, tmpchar);
	ensure_button_set(DOWNLEFT, padconfig[DOWNLEFT]);
}

void on_btnBack_pressed (GtkWidget *widget, gpointer data)
{
	char tmpchar[30];
	padconfig[BACK] = get_joystick_input();
	snprintf(tmpchar, 30, "Back\n[Button %d]", padconfig[BACK]);
	gtk_label_set_label(lblBtnBack, tmpchar);
	ensure_button_set(BACK, padconfig[BACK]);
}

void on_btnStart_pressed (GtkWidget *widget, gpointer data)
{
	char tmpchar[30];
	padconfig[START] = get_joystick_input();
	snprintf(tmpchar, 30, "Start\n[Button %d]", padconfig[START]);
	gtk_label_set_label(lblBtnStart, tmpchar);
	ensure_button_set(START, padconfig[START]);
	
}

int main (int argc, char *argv[])
{
	GladeXML *xml;
	char *device = "/dev/input/js0";
	GtkMessageDialog *dialog;
	
	gtk_init(&argc, &argv);
	
	/* Set config path (we want to die on this here if not) */
	if ((configpath = getenv("HOME")) == NULL)
	{
		fprintf(stderr, " Environment variable HOME not set.\n");
		return 1;
	}
	configpath = strcat(configpath, "/."CONFIG_FILENAME);
	
	xml = glade_xml_new("mousepad-config.glade", NULL, NULL);
	
	
	/* Reveal necessary widgets */
	mousepadWindow = (GtkWindow*) glade_xml_get_widget(xml, "mousepadWindow");
	lblBtnLeft = (GtkLabel*) glade_xml_get_widget(xml, "lblBtnLeft");
	lblBtnUpLeft = (GtkLabel*) glade_xml_get_widget(xml, "lblBtnUpLeft");
	lblBtnUp = (GtkLabel*) glade_xml_get_widget(xml, "lblBtnUp");
	lblBtnUpRight = (GtkLabel*) glade_xml_get_widget(xml, "lblBtnUpRight");
	lblBtnRight = (GtkLabel*) glade_xml_get_widget(xml, "lblBtnRight");
	lblBtnDownRight = (GtkLabel*) glade_xml_get_widget(xml, "lblBtnDownRight");
	lblBtnDown = (GtkLabel*) glade_xml_get_widget(xml, "lblBtnDown");
	lblBtnDownLeft = (GtkLabel*)glade_xml_get_widget(xml, "lblBtnDownLeft");
	lblBtnBack = (GtkLabel*) glade_xml_get_widget(xml, "lblBtnBack");
	lblBtnStart = (GtkLabel*) glade_xml_get_widget(xml, "lblBtnStart");
	statusbar = (GtkStatusbar*) glade_xml_get_widget(xml, "statusbar");
	
	/* Initialize Data */
	reset_data();
	
	/* Handle arguments manually without getopt(). */
	if (argc > 2) /* zu viele! */
	{
		fprintf(stderr, PROGRAM_NAME": Too many arguments.\n"); 
		return 1;
	}
	else if (argc == 2)
	{
		if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
		{
			fprintf(stdout, "Usage: %s [Joystick Device]\n", argv[0]);
			return 0;
		} else {
			device = argv[1];
		}
	}

	/* Open Joystick device */
	if ((joyFD = open(device, O_RDONLY | O_NONBLOCK)) == -1)
	{
		/* Error opening device; display error dialog and return */
		dialog = gtk_message_dialog_new (mousepadWindow,
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_CLOSE,
			"Couldn't open device %s.\n\nMake sure that the device is plugged in, or specify another device as the first argument to "PROGRAM_NAME".", device);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return 1;
	}
	
	/* Retrieve the number of buttons */
	ioctl(joyFD, JSIOCGBUTTONS, &numButtons);
	
	/* Allocate an array to store button information */
	button = (struct btn *) calloc(1, sizeof( struct btn ));

	glade_xml_signal_autoconnect(xml);

	gtk_main();

	return 0;
}
