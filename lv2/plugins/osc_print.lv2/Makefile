all:
	gcc -fPIC -shared -std=c99 -g -O0 -Wall `pkg-config --cflags glib-2.0` -o osc_print.so osc_print.c ../../extensions/osc/lv2_osc.c
