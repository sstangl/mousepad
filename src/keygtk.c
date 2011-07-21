/*
 * keygtk.c
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

#include <gtk/gtk.h>

#include <X11/X.h>
#include <X11/Xlib.h>

#define CALCULATED_WIDTH (screenWidth/5)
#define DISTANCE_FROM_CORNER 20

Display *display;

int keygtk_init(Display *d)
{
	if (d == NULL) return -1;
	display = d;
	return 0;
}

/* Thread for the GTK-based keyboard indicator */
void gtkThreadMain()
{
#if 0
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
	const int screenHeight = XDisplayHeight(display, DefaultScreen(display));
	const int screenWidth = XDisplayWidth(display, DefaultScreen(display));
	
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

#endif
}
