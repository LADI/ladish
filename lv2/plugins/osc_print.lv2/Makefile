all:
	gcc -fPIC -shared -std=c99 -g -O0 -Wall `pkg-config --cflags glib-2.0` -o osc_print.so osc_print.c ../lv2_osc.c
