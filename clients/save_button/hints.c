/*
 * Adapted for LASH by Robert Ham, 2002
 */

/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2001  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2001  Haavard Kvaalen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#define XA_WIN_SUPPORTING_WM_CHECK 	"_WIN_SUPPORTING_WM_CHECK"
#define XA_WIN_LAYER			"_WIN_LAYER"
#define XA_WIN_HINTS			"_WIN_HINTS"
#define XA_WIN_STATE			"_WIN_STATE"

/* flags for the window layer */
typedef enum {
	WIN_LAYER_DESKTOP = 0,
	WIN_LAYER_BELOW = 2,
	WIN_LAYER_NORMAL = 4,
	WIN_LAYER_ONTOP = 6,
	WIN_LAYER_DOCK = 8,
	WIN_LAYER_ABOVE_DOCK = 10
} WinLayer;

#define WIN_STATE_STICKY		(1<<0)

#define WIN_HINTS_SKIP_WINLIST		(1<<1)	/* not in win list */
#define WIN_HINTS_SKIP_TASKBAR		(1<<2)	/* not on taskbar */

#define XA_NET_SUPPORTING_WM_CHECK	"_NET_SUPPORTING_WM_CHECK"
#define XA_NET_WM_STATE			"_NET_WM_STATE"
#define XA_NET_STATE_STAYS_ON_TOP	"_NET_WM_STATE_STAYS_ON_TOP"
#define XA_NET_STATE_STICKY		"_NET_WM_STATE_STICKY"

#define _NET_WM_STATE_REMOVE   0
#define _NET_WM_STATE_ADD      1
#define _NET_WM_STATE_TOGGLE   2

void (*set_always_func) (GtkWidget *, gboolean);
void (*set_sticky_func) (GtkWidget *, gboolean);
void (*set_skip_winlist_func) (GtkWidget *);

gboolean
hint_always_on_top_available(void)
{
	return !!set_always_func;
}

gboolean
hint_sticky_available(void)
{
	return !!set_sticky_func;
}

gboolean
hint_skip_winlist_available(void)
{
	return !!set_skip_winlist_func;
}

static void
gnome_wm_set_skip_winlist(GtkWidget * widget)
{
	long data[1];
	Atom xa_win_hints = gdk_atom_intern(XA_WIN_HINTS, FALSE);

	data[0] = WIN_HINTS_SKIP_WINLIST | WIN_HINTS_SKIP_TASKBAR;
	XChangeProperty(GDK_DISPLAY(), GDK_WINDOW_XWINDOW(widget->window),
					xa_win_hints, XA_CARDINAL, 32,
					PropModeReplace, (unsigned char *)data, 1);
}

static void
gnome_wm_set_window_always(GtkWidget * window, gboolean always)
{
	XEvent xev;
	int prev_error;
	long layer = WIN_LAYER_ONTOP;
	Atom xa_win_layer = gdk_atom_intern(XA_WIN_LAYER, FALSE);

	if (always == FALSE)
		layer = WIN_LAYER_NORMAL;

	prev_error = gdk_error_warnings;
	gdk_error_warnings = 0;

	if (GTK_WIDGET_MAPPED(window)) {
		xev.type = ClientMessage;
		xev.xclient.type = ClientMessage;
		xev.xclient.window = GDK_WINDOW_XWINDOW(window->window);
		xev.xclient.message_type = xa_win_layer;
		xev.xclient.format = 32;
		xev.xclient.data.l[0] = layer;

		XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
				   SubstructureNotifyMask, (XEvent *) & xev);
	} else {
		long data[1];

		data[0] = layer;
		XChangeProperty(GDK_DISPLAY(),
						GDK_WINDOW_XWINDOW(window->window),
						xa_win_layer, XA_CARDINAL, 32, PropModeReplace,
						(unsigned char *)data, 1);
	}
	gdk_error_warnings = prev_error;

}

static void
gnome_wm_set_window_sticky(GtkWidget * window, gboolean sticky)
{
	XEvent xev;
	int prev_error;
	long state = 0;
	Atom xa_win_state = gdk_atom_intern(XA_WIN_STATE, FALSE);

	if (sticky)
		state = WIN_STATE_STICKY;

	prev_error = gdk_error_warnings;
	gdk_error_warnings = 0;

	if (GTK_WIDGET_MAPPED(window)) {
		xev.type = ClientMessage;
		xev.xclient.type = ClientMessage;
		xev.xclient.window = GDK_WINDOW_XWINDOW(window->window);
		xev.xclient.message_type = xa_win_state;
		xev.xclient.format = 32;
		xev.xclient.data.l[0] = WIN_STATE_STICKY;
		xev.xclient.data.l[1] = state;

		XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
				   SubstructureNotifyMask, (XEvent *) & xev);
	} else {
		long data[2];

		data[0] = state;
		XChangeProperty(GDK_DISPLAY(),
						GDK_WINDOW_XWINDOW(window->window),
						xa_win_state, XA_CARDINAL, 32, PropModeReplace,
						(unsigned char *)data, 1);
	}
	gdk_error_warnings = prev_error;

}

static gboolean
gnome_wm_found(void)
{
	Atom r_type, support_check;
	int r_format, prev_error, p;
	unsigned long count, bytes_remain;
	unsigned char *prop = NULL, *prop2 = NULL;
	gboolean ret = FALSE;

	prev_error = gdk_error_warnings;
	gdk_error_warnings = 0;
	support_check = gdk_atom_intern(XA_WIN_SUPPORTING_WM_CHECK, FALSE);

	p = XGetWindowProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(), support_check,
						   0, 1, False, XA_CARDINAL, &r_type, &r_format,
						   &count, &bytes_remain, &prop);

	if (p == Success && prop && r_type == XA_CARDINAL &&
		r_format == 32 && count == 1) {
		Window n = *(long *)prop;

		p = XGetWindowProperty(GDK_DISPLAY(), n, support_check, 0, 1,
							   False, XA_CARDINAL, &r_type, &r_format,
							   &count, &bytes_remain, &prop2);

		if (p == Success && prop2 && r_type == XA_CARDINAL &&
			r_format == 32 && count == 1)
			ret = TRUE;
	}

	if (prop)
		XFree(prop);
	if (prop2)
		XFree(prop2);
	gdk_error_warnings = prev_error;
	return ret;
}

static gboolean
net_wm_found(void)
{
	Atom r_type, support_check;
	int r_format, prev_error, p;
	unsigned long count, bytes_remain;
	unsigned char *prop = NULL, *prop2 = NULL;
	gboolean ret = FALSE;

	prev_error = gdk_error_warnings;
	gdk_error_warnings = 0;
	support_check = gdk_atom_intern(XA_NET_SUPPORTING_WM_CHECK, FALSE);

	p = XGetWindowProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(), support_check,
						   0, 1, False, XA_WINDOW, &r_type, &r_format,
						   &count, &bytes_remain, &prop);

	if (p == Success && prop && r_type == XA_WINDOW &&
		r_format == 32 && count == 1) {
		Window n = *(Window *) prop;

		p = XGetWindowProperty(GDK_DISPLAY(), n, support_check, 0, 1,
							   False, XA_WINDOW, &r_type, &r_format,
							   &count, &bytes_remain, &prop2);

		if (p == Success && prop2 && *prop2 == *prop &&
			r_type == XA_WINDOW && r_format == 32 && count == 1)
			ret = TRUE;
	}

	if (prop)
		XFree(prop);
	if (prop2)
		XFree(prop2);
	gdk_error_warnings = prev_error;
	return ret;
}

static void
net_wm_set_property(GtkWidget * window, char *atom, gboolean state)
{
	XEvent xev;
	gint prev_error;
	int set = _NET_WM_STATE_ADD;
	Atom type, property;

	if (state == FALSE)
		set = _NET_WM_STATE_REMOVE;

	type = gdk_atom_intern(XA_NET_WM_STATE, FALSE);
	property = gdk_atom_intern(atom, FALSE);

	prev_error = gdk_error_warnings;
	gdk_error_warnings = 0;

	xev.type = ClientMessage;
	xev.xclient.type = ClientMessage;
	xev.xclient.window = GDK_WINDOW_XWINDOW(window->window);
	xev.xclient.message_type = type;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = set;
	xev.xclient.data.l[1] = property;
	xev.xclient.data.l[2] = 0;

	XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
			   SubstructureNotifyMask, (XEvent *) & xev);

	gdk_error_warnings = prev_error;

}

static void
net_wm_set_window_always(GtkWidget * window, gboolean always)
{
	net_wm_set_property(window, XA_NET_STATE_STAYS_ON_TOP, always);
}

static void
net_wm_set_window_sticky(GtkWidget * window, gboolean sticky)
{
	net_wm_set_property(window, XA_NET_STATE_STICKY, sticky);
}

void
check_wm_hints(void)
{
	if (gnome_wm_found()) {
		set_sticky_func = gnome_wm_set_window_sticky;
		set_always_func = gnome_wm_set_window_always;
		set_skip_winlist_func = gnome_wm_set_skip_winlist;
	} else if (net_wm_found()) {
		set_sticky_func = net_wm_set_window_sticky;
		set_always_func = net_wm_set_window_always;
	}
}
