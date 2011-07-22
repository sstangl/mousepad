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

#include "mousepad.h"

#include <gtk/gtk.h>

#include <X11/X.h>
#include <X11/Xlib.h>

#define DISTANCE_FROM_CORNER 20

Display *display;
GtkWindow *window;
GtkImage *image;
int window_shown = 0;

struct
{
	GdkPixbuf *left, *left_shift,
	          *upleft, *upleft_shift,
	          *up, *up_shift,
	          *upright, *upright_shift,
	          *right, *right_shift,
	          //*downright, *downright_shift,
	          *down, *down_shift,
	          //*downleft, *downleft_shift,
	          *none;
} pixbufs;

/* GTK Keyboard Indicator initialization. */
int keygtk_init(Display *d)
{
	if (d == NULL) return -1;
	display = d;

	/* Retrieve the resolution in pixels */
	const int screen_height = XDisplayHeight(d, DefaultScreen(d));
	const int screen_width = XDisplayWidth(d, DefaultScreen(d));
	const int window_width = screen_width / 5;

	// TODO: error handling
	window = (GtkWindow *)gtk_window_new(GTK_WINDOW_POPUP);
	gtk_window_stick(window);
	gtk_window_move(window,
	                screen_width  - window_width - DISTANCE_FROM_CORNER,
									screen_height - window_width - DISTANCE_FROM_CORNER);
	
	/* Load pixbufs. */
	pixbufs.left = gdk_pixbuf_new_from_file_at_size(
		"img/mousepad_left.png", window_width, window_width, NULL);
	pixbufs.left_shift = gdk_pixbuf_new_from_file_at_size(
		"img/mousepad_left_shift.png", window_width, window_width, NULL);

	pixbufs.upleft = gdk_pixbuf_new_from_file_at_size(
		"img/mousepad_upleft.png", window_width, window_width, NULL);
	pixbufs.upleft_shift = gdk_pixbuf_new_from_file_at_size(
		"img/mousepad_upleft_shift.png", window_width, window_width, NULL);
	
	pixbufs.up = gdk_pixbuf_new_from_file_at_size(
		"img/mousepad_up.png", window_width, window_width, NULL);
	pixbufs.up_shift = gdk_pixbuf_new_from_file_at_size(
		"img/mousepad_up_shift.png", window_width, window_width, NULL);
	
	pixbufs.upright = gdk_pixbuf_new_from_file_at_size(
		"img/mousepad_upright.png", window_width, window_width, NULL);
	pixbufs.upright_shift = gdk_pixbuf_new_from_file_at_size(
		"img/mousepad_upright_shift.png", window_width, window_width, NULL);
	
	pixbufs.right = gdk_pixbuf_new_from_file_at_size(
		"img/mousepad_right.png", window_width, window_width, NULL);
	pixbufs.right_shift = gdk_pixbuf_new_from_file_at_size(
		"img/mousepad_right_shift.png", window_width, window_width, NULL);
	
	pixbufs.down = gdk_pixbuf_new_from_file_at_size(
		"img/mousepad_down.png", window_width, window_width, NULL);
	pixbufs.down_shift = gdk_pixbuf_new_from_file_at_size(
		"img/mousepad_down_shift.png", window_width, window_width, NULL);
	
	pixbufs.none = gdk_pixbuf_new_from_file_at_size(
		"img/mousepad_none.png", window_width, window_width, NULL);

	//TODO: Check whether images failed to load.
	
	/* Create image widget. */
	image = (GtkImage *)gtk_image_new_from_pixbuf(pixbufs.none);
	
	gtk_container_add(GTK_CONTAINER(window), (GtkWidget *)image);
	gtk_widget_show((GtkWidget *)image);

	gtk_image_set_from_pixbuf(image, pixbufs.none); // TODO: Necessary?

	return 0;
}

void keygtk_window_show()
{
	if (window_shown) return;
	gtk_widget_show((GtkWidget *)window);
	window_shown = 1;

	while (gtk_events_pending())
		gtk_main_iteration();
}

void keygtk_window_hide()
{
	if (!window_shown) return;
	gtk_widget_hide((GtkWidget *)window);
	window_shown = 0;

	while (gtk_events_pending())
		gtk_main_iteration();
}

/*
 * Set the pad button -> key binding into a particular layout mode.
 * layout is one of the BUTTON defines, or 0x0 to clear the layout.
 */
int keygtk_set_layout(int layout)
{
	GdkPixbuf *pixbuf = NULL;

	// TODO: Handle the Shift key.
	switch (layout) {
		case 0x0:              pixbuf = pixbufs.none;      break;
		case BUTTON_LEFT:      pixbuf = pixbufs.left;      break;
		case BUTTON_UPLEFT:    pixbuf = pixbufs.upleft;    break;
		case BUTTON_UP:        pixbuf = pixbufs.up;        break;
		case BUTTON_UPRIGHT:   pixbuf = pixbufs.upright;   break;
		case BUTTON_RIGHT:     pixbuf = pixbufs.right;     break;
		case BUTTON_DOWNRIGHT: pixbuf = pixbufs.none;      break;
		case BUTTON_DOWN:      pixbuf = pixbufs.down;      break;
		case BUTTON_DOWNLEFT:  pixbuf = pixbufs.none;      break;
		default:
			return -1;
	}

	gtk_image_clear(image); //TODO: Necessary?
	gtk_image_set_from_pixbuf(image, pixbuf);

	while (gtk_events_pending())
		gtk_main_iteration();

	return 0;
}

