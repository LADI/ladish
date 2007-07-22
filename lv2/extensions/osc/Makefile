SOURCES = lv2_osc.c lv2_osc_print.c lv2_osc_test.c

all:
	gcc -std=c99 -g -O0 -Wall `pkg-config --cflags glib-2.0` -llo -o lv2_osc_test $(SOURCES)
