/*
 * keyboard.c
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

#include "keyboard.h"

#define KEYBOARD_STATE_NONE 0
#define KEYBOARD_STATE_LEFT 1
#define KEYBOARD_STATE_UP 2
#define KEYBOARD_STATE_RIGHT 3
#define KEYBOARD_STATE_DOWN 4
#define KEYBOARD_STATE_UPLEFT 5
#define KEYBOARD_STATE_UPRIGHT 6
#define KEYBOARD_STATE_DOWNRIGHT 7
#define KEYBOARD_STATE_DOWNLEFT 8

Display *display;
int shift = 0; // State of the shift toggle: nonzero if active.

/* Keyboard initialization. */
int keyboard_init(Display *d)
{
	if (d == NULL) return -1;

	display = d;
	return 0;
}

/* Internal wrapper for XTestFakeKeyEvent. */
static inline int keyboard_keyevent(unsigned key, int pushed)
{
	return XTestFakeKeyEvent(display, XKeysymToKeycode(display, key),
	                         pushed, CurrentTime);
}

/* Send a key press event to the current window. */
void keyboard_press(unsigned key)
{
	if (shift)
		keyboard_keyevent(XK_Shift_R, True);

	keyboard_keyevent(key, True);  // press key
	keyboard_keyevent(key, False); // release key

	if (shift)
		keyboard_keyevent(XK_Shift_R, False);
}

void keyboard_event(buttonstate_t buttons, button_t changed)
{
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
}

