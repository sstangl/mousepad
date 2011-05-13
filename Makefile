default: mousepad mousepad-config

mousepad: src/mousepad.c
	gcc -std=gnu99 -Wall -o mousepad src/mousepad.c -pthread -lX11 -lXtst -lrt -Wl,--as-needed,--sort-common `pkg-config gtk+-2.0 --libs --cflags`
	strip mousepad

mousepad-config: src/mousepad-config.c
	gcc -Wall -o mousepad-config src/mousepad-config.c `pkg-config libglade-2.0 --cflags --libs` -Wl,-export-dynamic
	strip mousepad-config

clean:
	rm -f mousepad mousepad-config
