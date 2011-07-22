default: mousepad mousepad-config

mousepad: src/mousepad.c src/mouse.c src/config.c src/keyboard.c src/keygtk.c
	gcc -g -std=gnu99 -Wall -o mousepad src/config.c src/mousepad.c src/mouse.c src/keyboard.c src/keygtk.c -lX11 -lXtst -lrt -Wl,--as-needed,--sort-common `pkg-config gtk+-2.0 --libs --cflags`
#	strip mousepad

mousepad-config: src/mousepad-config.c
	gcc -g -Wall -o mousepad-config src/mousepad-config.c `pkg-config libglade-2.0 --cflags --libs` -Wl,-export-dynamic
#	strip mousepad-config

clean:
	rm -f mousepad mousepad-config
