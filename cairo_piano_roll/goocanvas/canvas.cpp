/* Goocanvas experiments.   Lifted from Goocanvas demo.  GPL. */

#include <gtkmm.h>
#include <cairo.h>

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <goocanvas.h>
#include "goocanvasitemsimple.h"

GooCanvasItem *textitem, *ellipse1, *ellipse2, *rect1, *rect2, *rect3, *rect4;

gboolean dragging = FALSE;
double drag_x, drag_y;

void setup_canvas (GooCanvas *canvas);
GtkWidget *create_page (void);


/******************************** DEMO ITEM *******************************/

G_BEGIN_DECLS


#define GOO_TYPE_DEMO_ITEM            (goo_demo_item_get_type ())
#define GOO_DEMO_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_DEMO_ITEM, GooDemoItem))
#define GOO_DEMO_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_DEMO_ITEM, GooDemoItemClass))
#define GOO_IS_DEMO_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_DEMO_ITEM))
#define GOO_IS_DEMO_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_DEMO_ITEM))
#define GOO_DEMO_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GOO_TYPE_DEMO_ITEM, GooDemoItemClass))


typedef struct _GooDemoItem       GooDemoItem;
typedef struct _GooDemoItemClass  GooDemoItemClass;

struct _GooDemoItem
{
  GooCanvasItemSimple parent_object;

  gdouble x, y, width, height;
};

struct _GooDemoItemClass
{
  GooCanvasItemSimpleClass parent_class;
};


GType               goo_demo_item_get_type  (void) G_GNUC_CONST;

GooCanvasItem*      goo_demo_item_new       (GooCanvasItem      *parent,
					     gdouble             x,
					     gdouble             y,
					     gdouble             width,
					     gdouble             height,
					     ...);


G_END_DECLS


/* Use the GLib convenience macro to define the type. GooDemoItem is the
   class struct, goo_demo_item is the function prefix, and our class is a
   subclass of GOO_TYPE_CANVAS_ITEM_SIMPLE. */
G_DEFINE_TYPE (GooDemoItem, goo_demo_item, GOO_TYPE_CANVAS_ITEM_SIMPLE)


/* The standard object initialization function. */
void
goo_demo_item_init (GooDemoItem *demo_item)
{
  demo_item->x = 0.0;
  demo_item->y = 0.0;
  demo_item->width = 0.0;
  demo_item->height = 0.0;
}


/* The convenience function to create new items. This should start with a 
   parent argument and end with a variable list of object properties to fit
   in with the standard canvas items. */
GooCanvasItem*
goo_demo_item_new (GooCanvasItem      *parent,
		   gdouble             x,
		   gdouble             y,
		   gdouble             width,
		   gdouble             height,
		   ...)
{
  GooCanvasItem *item;
  GooDemoItem *demo_item;
  const char *first_property;
  va_list var_args;

  item = (GooCanvasItem*)g_object_new (GOO_TYPE_DEMO_ITEM, NULL);

  demo_item = (GooDemoItem*) item;
  demo_item->x = x;
  demo_item->y = y;
  demo_item->width = width;
  demo_item->height = height;

  va_start (var_args, height);
  first_property = va_arg (var_args, char*);
  if (first_property)
    g_object_set_valist ((GObject*) item, first_property, var_args);
  va_end (var_args);

  if (parent)
    {
      goo_canvas_item_add_child (parent, item, -1);
      g_object_unref (item);
    }

  return item;
}


/* The update method. This is called when the canvas is initially shown and
   also whenever the object is updated and needs to change its size and/or
   shape. It should calculate its new bounds in its own coordinate space,
   storing them in simple->bounds. */
void
goo_demo_item_update  (GooCanvasItemSimple *simple,
		       cairo_t             *cr)
{
  GooDemoItem *demo_item = (GooDemoItem*) simple;

  /* Compute the new bounds. */
  simple->bounds.x1 = demo_item->x;
  simple->bounds.y1 = demo_item->y;
  simple->bounds.x2 = demo_item->x + demo_item->width;
  simple->bounds.y2 = demo_item->y + demo_item->height;
}


/* The paint method. This should draw the item on the given cairo_t, using
   the item's own coordinate space. */
void
goo_demo_item_paint (GooCanvasItemSimple   *simple,
		     cairo_t               *cr,
		     const GooCanvasBounds *bounds)
{
  GooDemoItem *demo_item = (GooDemoItem*) simple;

  cairo_move_to (cr, demo_item->x, demo_item->y);
  cairo_line_to (cr, demo_item->x, demo_item->y + demo_item->height);
  cairo_line_to (cr, demo_item->x + demo_item->width,
		 demo_item->y + demo_item->height);
  cairo_line_to (cr, demo_item->x + demo_item->width, demo_item->y);
  cairo_close_path (cr);
  goo_canvas_style_set_fill_options (simple->simple_data->style, cr);
  cairo_fill (cr);
}


/* Hit detection. This should check if the given coordinate (in the item's
   coordinate space) is within the item. If it is it should return TRUE,
   otherwise it should return FALSE. */
gboolean
goo_demo_item_is_item_at (GooCanvasItemSimple *simple,
			  gdouble              x,
			  gdouble              y,
			  cairo_t             *cr,
			  gboolean             is_pointer_event)
{
  GooDemoItem *demo_item = (GooDemoItem*) simple;

  if (x < demo_item->x || (x > demo_item->x + demo_item->width)
      || y < demo_item->y || (y > demo_item->y + demo_item->height))
    return FALSE;

  return TRUE;
}


/* The class initialization function. Here we set the class' update(), paint()
   and is_item_at() methods. */
void
goo_demo_item_class_init (GooDemoItemClass *klass)
{
  GooCanvasItemSimpleClass *simple_class = (GooCanvasItemSimpleClass*) klass;

  simple_class->simple_update        = goo_demo_item_update;
  simple_class->simple_paint         = goo_demo_item_paint;
  simple_class->simple_is_item_at    = goo_demo_item_is_item_at;
}


/************************* END DEMO ITEM **********************************/

void
zoom_changed (GtkAdjustment *adj, GooCanvas *canvas)
{
  goo_canvas_set_scale (canvas, adj->value);
}


void
center_toggled (GtkToggleButton *button, gpointer data)
{
  /*foo_canvas_set_center_scroll_region (data,
                                   gtk_toggle_button_get_active (button));*/
}


void
anchor_toggled (GtkWidget *button, GooCanvas *canvas)
{
  GtkAnchorType anchor;

  anchor = (GtkAnchorType)GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "anchor"));

  if (GTK_TOGGLE_BUTTON (button)->active)
    g_object_set (canvas, "anchor", anchor, NULL);
}


void
scroll_to_50_50_clicked (GtkWidget *button, GooCanvas *canvas)
{
  goo_canvas_scroll_to (canvas, 50, 50);
}


void
scroll_to_500_500_clicked (GtkWidget *button, GooCanvas *canvas)
{
  goo_canvas_scroll_to (canvas, 500, 500);
}


void
scroll_to_250_250_clicked (GtkWidget *button, GooCanvas *canvas)
{
  goo_canvas_scroll_to (canvas, 250, 250);
}


void
animate_ellipse_clicked (GtkWidget *button, GooCanvas *canvas)
{
#if 1
  goo_canvas_item_animate (ellipse2, 100, 100, 1, 90, TRUE, 1000, 40,
			   GOO_CANVAS_ANIMATE_BOUNCE);
#endif
#if 0
  goo_canvas_item_animate (textitem, -300, -200, 1, 90, TRUE, 1000, 40,
			   GOO_CANVAS_ANIMATE_BOUNCE);
#endif
}


void
stop_animation_clicked (GtkWidget *button, GooCanvas *canvas)
{
  goo_canvas_item_stop_animation (ellipse2);
}


void
move_ellipse_clicked (GtkWidget *button, GooCanvas *canvas)
{
  static int last_state = 0;

#if 0
  g_print ("Moving ellipse\n");
#endif

  goo_canvas_item_set_transform (ellipse2, NULL);

  if (last_state == 0)
    {
      g_object_set (ellipse2,
		    "center-x", 300.0,
		    "center-y", 70.0,
		    "radius-x", 45.0,
		    "radius-y", 30.0,
		    "fill-color", "red",
		    "stroke-color", "midnightblue",
		    "line-width", 4.0,
		    "title", "A red ellipse",
		    NULL);
      last_state = 1;
    }
  else if (last_state == 1)
    {
      g_object_set (ellipse2,
		    "center-x", 390.0,
		    "center-y", 150.0,
		    "radius-x", 45.0,
		    "radius-y", 40.0,
		    "fill-pattern", NULL,
		    "stroke-color", "midnightblue",
		    "line-width", 4.0,
		    "title", "A brown ellipse",
		    NULL);
      last_state = 2;
    }
  else if (last_state == 2)
    {
      g_object_set (ellipse2,
		    "center-x", 0.0,
		    "center-y", 0.0,
		    "radius-y", 30.0,
		    NULL);
      goo_canvas_item_set_simple_transform (ellipse2, 100, 100, 1, 0);
      last_state = 3;
    }
  else if (last_state == 3)
    {
      goo_canvas_item_set_simple_transform (ellipse2, 200, 100, 2, 0);
      last_state = 4;
    }
  else if (last_state == 4)
    {
      goo_canvas_item_set_simple_transform (ellipse2, 200, 200, 1, 45);
      last_state = 5;
    }
  else if (last_state == 5)
    {
      goo_canvas_item_set_simple_transform (ellipse2, 50, 50, 0.2, 225);
      last_state = 6;
    }
  else
    {
      g_object_set (ellipse2,
		    "center-x", 335.0,
		    "center-y", 70.0,
		    "radius-x", 45.0,
		    "radius-y", 30.0,
		    "fill-color", "purple",
		    "stroke-color", "midnightblue",
		    "line-width", 4.0,
		    "title", "A purple ellipse",
		    NULL);
      last_state = 0;
    }
}


gboolean
on_motion_notify (GooCanvasItem *item,
		  GooCanvasItem *target,
		  GdkEventMotion *event,
		  gpointer data)
{
#if 0
  g_print ("received 'motion-notify' signal at %g, %g\n",
	   event->x, event->y);
#endif

  if (dragging && (event->state & GDK_BUTTON1_MASK))
    {
      double new_x = event->x;
      double new_y = event->y;

      goo_canvas_item_translate (item, new_x - drag_x, new_y - drag_y);
    }

  return TRUE;
}


void
output_items_in_area (GooCanvas *canvas,
		      gdouble x,
		      gdouble y)
{
  static gdouble last_x = 0.0, last_y = 0.0;
  GooCanvasBounds area;
  GList *items, *elem;

  /* Print out the items from the last point to this one. */
  area.x1 = MIN (x, last_x);
  area.x2 = MAX (x, last_x);
  area.y1 = MIN (y, last_y);
  area.y2 = MAX (y, last_y);
  items = goo_canvas_get_items_in_area (canvas, &area, TRUE, FALSE, FALSE);
  for (elem = items; elem; elem = elem->next)
    {
      g_print ("  found items in area (%g, %g - %g, %g): %p\n",
	       area.x1, area.y1, area.x2, area.y2, elem->data);
    }
  g_list_free (items);
  last_x = x;
  last_y = y;
}


gboolean
on_button_press (GooCanvasItem *item,
		 GooCanvasItem *target,
		 GdkEventButton *event,
		 gpointer data)
{
  GooCanvas *canvas;
  GdkCursor *fleur;
  GList *items, *elem;

  g_print ("%p received 'button-press' signal at %g, %g (root: %g, %g)\n",
	   item, event->x, event->y, event->x_root, event->y_root);

  canvas = goo_canvas_item_get_canvas (item);
  items = goo_canvas_get_items_at (canvas, event->x_root, event->y_root,
				   TRUE);
  for (elem = items; elem; elem = elem->next)
    g_print ("  found items: %p\n", elem->data);
  g_list_free (items);

  output_items_in_area (canvas, event->x_root, event->y_root);

  switch (event->button)
    {
    case 1:
      if (event->state & GDK_SHIFT_MASK)
	{
	  goo_canvas_item_remove (item);
	}
      else
	{
	  drag_x = event->x;
	  drag_y = event->y;

	  fleur = gdk_cursor_new (GDK_FLEUR);
	  goo_canvas_pointer_grab (canvas, item,
				   GdkEventMask(GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK),
				   fleur,
				   event->time);
	  gdk_cursor_unref (fleur);
	  dragging = TRUE;
	}
      break;

    case 2:
      goo_canvas_item_lower (item, NULL);
      break;

    case 3:
      goo_canvas_item_raise (item, NULL);
      break;

    default:
      break;
    }

  return TRUE;
}


gboolean
on_button_release (GooCanvasItem *item,
		   GooCanvasItem *target,
		   GdkEventButton *event,
		   gpointer data)
{
  GooCanvas *canvas;

#if 0
  g_print ("received 'button-release' signal\n");
#endif

  canvas = goo_canvas_item_get_canvas (item);
  goo_canvas_pointer_ungrab (canvas, item, event->time);
  dragging = FALSE;

  return TRUE;
}


gboolean
on_background_button_press (GooCanvasItem *item,
			    GooCanvasItem *target,
			    GdkEventButton *event,
			    gpointer data)
{
  GooCanvas *canvas;

#if 1
  g_print ("background received 'button-press' signal\n");
#endif

  canvas = goo_canvas_item_get_canvas (item);
  output_items_in_area (canvas, event->x_root, event->y_root);

  return TRUE;
}


void
setup_item_signals (GooCanvasItem *item)
{
  g_signal_connect (item, "motion_notify_event",
		    (GtkSignalFunc) on_motion_notify, NULL);
  g_signal_connect (item, "button_press_event",
		    (GtkSignalFunc) on_button_press, NULL);
  g_signal_connect (item, "button_release_event",
		    (GtkSignalFunc) on_button_release, NULL);
}

#if 0
GtkWidget *
create_canvas_primitives ()
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *w;
	GtkWidget *scrolled_win, *canvas;
	GtkAdjustment *adj;
	GSList *group = NULL;

	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
	gtk_widget_show (vbox);

	w = gtk_label_new ("Drag an item with button 1.  Click button 2 on an item to lower it, or button 3 to raise it.");
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	/* Create the canvas */

	canvas = goo_canvas_new ();
	g_object_set (G_OBJECT (canvas),
		      "background-color-rgb", 0xC3C3FF,
		      NULL);
	goo_canvas_set_bounds (GOO_CANVAS (canvas), 0, 0, 604, 454);

	/* Zoom */

	w = gtk_label_new ("Zoom:");
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);

	adj = GTK_ADJUSTMENT (gtk_adjustment_new (1.00, 0.05, 50.00, 0.05, 0.50, 0.50));
	w = gtk_spin_button_new (adj, 0.0, 2);
	g_signal_connect (adj, "value_changed",
			  (GtkSignalFunc) zoom_changed,
			  canvas);
	gtk_widget_set_size_request (w, 50, -1);
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);

	/* Center: */
	
	w = gtk_check_button_new_with_label("Center scroll region");
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	/*gtk_widget_show (w);*/
	g_signal_connect (w, "toggled",
			  (GtkSignalFunc) center_toggled,
			  canvas);

	/* Move Ellipse */

	w = gtk_button_new_with_label("Move Ellipse");
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	g_signal_connect (w, "clicked",
			  (GtkSignalFunc) move_ellipse_clicked,
			  canvas);

	/* Animate Ellipse */

	w = gtk_button_new_with_label("Animate Ellipse");
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	g_signal_connect (w, "clicked",
			  (GtkSignalFunc) animate_ellipse_clicked,
			  canvas);
	
	/* Stop Animation */

	w = gtk_button_new_with_label("Stop Animation");
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	g_signal_connect (w, "clicked",
			  (GtkSignalFunc) stop_animation_clicked,
			  canvas);
	
	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	/* Scroll to */

	w = gtk_label_new ("Scroll To:");
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);

	w = gtk_button_new_with_label("50,50");
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	g_signal_connect (w, "clicked",
			  (GtkSignalFunc) scroll_to_50_50_clicked,
			  canvas);

	w = gtk_button_new_with_label("250,250");
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	g_signal_connect (w, "clicked",
			  (GtkSignalFunc) scroll_to_250_250_clicked,
			  canvas);

	w = gtk_button_new_with_label("500,500");
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	g_signal_connect (w, "clicked",
			  (GtkSignalFunc) scroll_to_500_500_clicked,
			  canvas);

	/* Scroll anchor */

	w = gtk_label_new ("Anchor:");
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);

	w = gtk_radio_button_new_with_label (group, "NW");
	group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (w));
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	g_signal_connect (w, "toggled",
			  (GtkSignalFunc) anchor_toggled,
			  canvas);
	g_object_set_data (G_OBJECT (w), "anchor",
			   GINT_TO_POINTER (GTK_ANCHOR_NW));

	w = gtk_radio_button_new_with_label (group, "N");
	group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (w));
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	g_signal_connect (w, "toggled",
			  (GtkSignalFunc) anchor_toggled,
			  canvas);
	g_object_set_data (G_OBJECT (w), "anchor",
			   GINT_TO_POINTER (GTK_ANCHOR_N));

	w = gtk_radio_button_new_with_label (group, "NE");
	group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (w));
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	g_signal_connect (w, "toggled",
			  (GtkSignalFunc) anchor_toggled,
			  canvas);
	g_object_set_data (G_OBJECT (w), "anchor",
			   GINT_TO_POINTER (GTK_ANCHOR_NE));

	w = gtk_radio_button_new_with_label (group, "W");
	group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (w));
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	g_signal_connect (w, "toggled",
			  (GtkSignalFunc) anchor_toggled,
			  canvas);
	g_object_set_data (G_OBJECT (w), "anchor",
			   GINT_TO_POINTER (GTK_ANCHOR_W));

	w = gtk_radio_button_new_with_label (group, "C");
	group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (w));
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	g_signal_connect (w, "toggled",
			  (GtkSignalFunc) anchor_toggled,
			  canvas);
	g_object_set_data (G_OBJECT (w), "anchor",
			   GINT_TO_POINTER (GTK_ANCHOR_CENTER));

	w = gtk_radio_button_new_with_label (group, "E");
	group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (w));
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	g_signal_connect (w, "toggled",
			  (GtkSignalFunc) anchor_toggled,
			  canvas);
	g_object_set_data (G_OBJECT (w), "anchor",
			   GINT_TO_POINTER (GTK_ANCHOR_E));

	w = gtk_radio_button_new_with_label (group, "SW");
	group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (w));
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	g_signal_connect (w, "toggled",
			  (GtkSignalFunc) anchor_toggled,
			  canvas);
	g_object_set_data (G_OBJECT (w), "anchor",
			   GINT_TO_POINTER (GTK_ANCHOR_SW));

	w = gtk_radio_button_new_with_label (group, "S");
	group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (w));
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	g_signal_connect (w, "toggled",
			  (GtkSignalFunc) anchor_toggled,
			  canvas);
	g_object_set_data (G_OBJECT (w), "anchor",
			   GINT_TO_POINTER (GTK_ANCHOR_S));

	w = gtk_radio_button_new_with_label (group, "SE");
	group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (w));
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	g_signal_connect (w, "toggled",
			  (GtkSignalFunc) anchor_toggled,
			  canvas);
	g_object_set_data (G_OBJECT (w), "anchor",
			   GINT_TO_POINTER (GTK_ANCHOR_SE));

	
	/* Layout the stuff */

	scrolled_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolled_win);
	gtk_box_pack_start (GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);

	gtk_widget_show (canvas);
	gtk_container_add (GTK_CONTAINER (scrolled_win), canvas);

	/* Add all the canvas items. */
	setup_canvas (GOO_CANVAS (canvas));

#if 0
	g_signal_connect_after (canvas, "key_press_event",
				(GtkSignalFunc) key_press,
				NULL);

	GTK_WIDGET_SET_FLAGS (canvas, GTK_CAN_FOCUS);
	gtk_widget_grab_focus (canvas);
#endif

	return vbox;
}


void
setup_heading (GooCanvasItem *root, char *text, int pos)
{
  double x = (pos % 3) * 200 + 100;
  double y = (pos / 3) * 150 + 5;
  GooCanvasItem *item;

  item = goo_canvas_text_new (root, text, x, y, -1, GTK_ANCHOR_N,
			      "font", "Sans 12",
			      NULL);
  goo_canvas_item_skew_y (item, 30, x, y);
  /*goo_canvas_item_rotate (item, 30, x, y);*/
}


void
setup_divisions (GooCanvasItem *root)
{
  GooCanvasItem *group, *item;

  group = goo_canvas_group_new (root, NULL);
  goo_canvas_item_translate (group, 2, 2);

  item = goo_canvas_rect_new (group, 0, 0, 600, 450,
			      "line-width", 4.0,
			      NULL);

  item = goo_canvas_polyline_new_line (group, 0, 150, 600, 150,
			      "line-width", 4.0,
			      NULL);

  item = goo_canvas_polyline_new_line (group, 0, 300, 600, 300,
			      "line-width", 4.0,
			      NULL);

  item = goo_canvas_polyline_new_line (group, 200, 0, 200, 450,
			      "line-width", 4.0,
			      NULL);

  item = goo_canvas_polyline_new_line (group, 400, 0, 400, 450,
			      "line-width", 4.0,
			      NULL);

#if 1
  setup_heading (group, "Rectangles", 0);
  setup_heading (group, "Ellipses", 1);
  setup_heading (group, "Texts", 2);
  setup_heading (group, "Images", 3);
  setup_heading (group, "Lines", 4);
  /*setup_heading (group, "No Curves", 5);*/
  /*setup_heading (group, "Arcs", 6);*/
  setup_heading (group, "Polygons", 7);
  /*setup_heading (group, "Widgets", 8);*/
#endif
}


/* FIXME: I think we may need to check byte order. */
cairo_pattern_t*
create_stipple (const char *color_name, guchar stipple_data[16])
{
  cairo_surface_t *surface;
  cairo_pattern_t *pattern;
  GdkColor color;

  gdk_color_parse (color_name, &color);
  stipple_data[2] = stipple_data[14] = color.red >> 8;
  stipple_data[1] = stipple_data[13] = color.green >> 8;
  stipple_data[0] = stipple_data[12] = color.blue >> 8;
  surface = cairo_image_surface_create_for_data (stipple_data,
						 CAIRO_FORMAT_ARGB32,
						 2, 2, 8);
  pattern = cairo_pattern_create_for_surface (surface);
  cairo_surface_destroy (surface);
  cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);

  return pattern;
}


void
setup_rectangles (GooCanvasItem *root)
{
  GooCanvasItem *item;
  cairo_pattern_t *pattern;
  static guchar stipple_data[16] = {
    0, 0, 0, 255,   0, 0, 0, 0,   0, 0, 0, 0,     0, 0, 0, 255
  };

  item = goo_canvas_rect_new (root, 20, 30, 50, 30,
			      "stroke-color", "red",
			      "line-width", 8.0,
			      NULL);
  setup_item_signals (item);

  pattern = create_stipple ("mediumseagreen", stipple_data);
  item = goo_canvas_rect_new (root, 90, 40, 90, 60,
			      "fill-pattern", pattern,
			      "stroke-color", "black",
			      "line-width", 4.0,
			      NULL);
  cairo_pattern_destroy (pattern);
  setup_item_signals (item);

  item = goo_canvas_rect_new (root, 10, 80, 70, 60,
			      "fill-color", "steelblue",
			      /*"fill-pattern", NULL,*/
			      NULL);
  setup_item_signals (item);

  item = goo_canvas_rect_new (root, 20, 90, 70, 60,
			      "fill-color-rgba", 0x3cb37180,
			      "stroke-color", "blue",
			      "line-width", 2.0,
			      NULL);
  setup_item_signals (item);

  item = goo_canvas_rect_new (root, 110, 80, 50, 30,
			      "radius-x", 20.0,
			      "radius-y", 10.0,
			      "stroke-color", "yellow",
			      "fill-color-rgba", 0x3cb3f180,
			      NULL);
  setup_item_signals (item);

  item = goo_demo_item_new (root, 30, 20, 50, 30,
			    "fill-color", "yellow",
			    NULL);
  setup_item_signals (item);
}


void
setup_ellipses (GooCanvasItem *root)
{
  GooCanvasItem *ellipse1, *ellipse3;
  cairo_pattern_t *pattern;
  static guchar stipple_data[16] = {
    0, 0, 0, 255,   0, 0, 0, 0,
    0, 0, 0, 0,     0, 0, 0, 255
  };

#if 1
  ellipse1 = goo_canvas_ellipse_new (root, 245, 45, 25, 15,
				     "stroke-color", "goldenrod",
				     "line-width", 8.0,
				     NULL);
  setup_item_signals (ellipse1);
#endif

  ellipse2 = goo_canvas_ellipse_new (root, 335, 70, 45, 30,
				     "fill-color", "wheat",
				     "stroke-color", "midnightblue",
				     "line-width", 4.0,
				     "title", "An ellipse",
				     NULL);
#if 0
  goo_canvas_item_rotate (ellipse2, 30, 0, 0);
#endif
  setup_item_signals (ellipse2);

#if 1
  pattern = create_stipple ("cadetblue", stipple_data);
  ellipse3 = goo_canvas_ellipse_new (root, 245, 110, 35, 30,
				     "fill-pattern", pattern,
				     "stroke-color", "black",
				     "line-width", 1.0,
				     NULL);
  cairo_pattern_destroy (pattern);
  setup_item_signals (ellipse3);
#endif
}


#define VERTICES 10
#define RADIUS 60.0

void
polish_diamond (GooCanvasItem *root)
{
  GooCanvasItem *group, *item;
  int i, j;
  double a, x1, y1, x2, y2;

  group = goo_canvas_group_new (root,
				"line-width", 1.0,
				"line-cap", CAIRO_LINE_CAP_ROUND,
				NULL);
  goo_canvas_item_translate (group, 270, 230);
  setup_item_signals (group);

  for (i = 0; i < VERTICES; i++) {
    a = 2.0 * M_PI * i / VERTICES;
    x1 = RADIUS * cos (a);
    y1 = RADIUS * sin (a);

    for (j = i + 1; j < VERTICES; j++) {
      a = 2.0 * M_PI * j / VERTICES;
      x2 = RADIUS * cos (a);
      y2 = RADIUS * sin (a);
      item = goo_canvas_polyline_new_line (group, x1, y1, x2, y2, NULL);
    }
  }
}


#define SCALE 7.0

void
make_hilbert (GooCanvasItem *root)
{
  char hilbert[] = "urdrrulurulldluuruluurdrurddldrrruluurdrurddldrddlulldrdldrrurd";
  char *c;
  double *pp, *p;
  GooCanvasItem *item;
  GooCanvasPoints *points;
  cairo_pattern_t *pattern;
  static guchar stipple_data[16] = {
    0, 0, 0, 255,   0, 0, 0, 0,   0, 0, 0, 0,     0, 0, 0, 255
  };

  points = goo_canvas_points_new (strlen (hilbert) + 1);
  points->coords[0] = 340.0;
  points->coords[1] = 290.0;

  pp = points->coords;
  for (c = hilbert, p = points->coords + 2; *c; c++, p += 2, pp += 2)
    switch (*c) {
    case 'u':
      p[0] = pp[0];
      p[1] = pp[1] - SCALE;
      break;

    case 'd':
      p[0] = pp[0];
      p[1] = pp[1] + SCALE;
      break;

    case 'l':
      p[0] = pp[0] - SCALE;
      p[1] = pp[1];
      break;

    case 'r':
      p[0] = pp[0] + SCALE;
      p[1] = pp[1];
      break;
    }

  pattern = create_stipple ("red", stipple_data);
  item = goo_canvas_polyline_new (root, FALSE, 0,
				  "points", points,
				  "line-width", 4.0,
				  "stroke-pattern", pattern,
				  "line-cap", CAIRO_LINE_CAP_SQUARE,
				  "line-join", CAIRO_LINE_JOIN_MITER,
				  NULL);
  cairo_pattern_destroy (pattern);
  setup_item_signals (item);

  goo_canvas_points_unref (points);
}

void
setup_lines (GooCanvasItem *root)
{
  GooCanvasItem *polyline1, *polyline2, *polyline3, *polyline4, *polyline5;

  polish_diamond (root);
  make_hilbert (root);

  /* Arrow tests */

  polyline1 = goo_canvas_polyline_new (root, FALSE, 4,
				       340.0, 170.0,
				       340.0, 230.0,
				       390.0, 230.0,
				       390.0, 170.0,
				       "stroke-color", "midnightblue",
				       "line-width", 3.0,
				       "start-arrow", TRUE,
				       "end-arrow", TRUE,
				       "arrow-tip-length", 3.0,
				       "arrow-length", 4.0,
				       "arrow-width", 3.5,
				       NULL);
  setup_item_signals (polyline1);

  polyline2 = goo_canvas_polyline_new (root, FALSE, 2,
				       356.0, 180.0,
				       374.0, 220.0,
				       "stroke-color", "blue",
				       "line-width", 1.0,
				       "start-arrow", TRUE,
				       "end-arrow", TRUE,
				       "arrow-tip-length", 5.0,
				       "arrow-length", 6.0,
				       "arrow-width", 6.0,
				       NULL);
  setup_item_signals (polyline2);

  polyline3 = goo_canvas_polyline_new (root, FALSE, 2,
				       356.0, 220.0,
				       374.0, 180.0,
				       "stroke-color", "blue",
				       "line-width", 1.0,
				       "start-arrow", TRUE,
				       "end-arrow", TRUE,
				       "arrow-tip-length", 5.0,
				       "arrow-length", 6.0,
				       "arrow-width", 6.0,
				       NULL);
  setup_item_signals (polyline3);

  /* Test polyline without any coords. */
  polyline4 = goo_canvas_polyline_new (root, FALSE, 0,
				       NULL);
  setup_item_signals (polyline4);

  /* Test polyline with 1 coord and arrows. */
  polyline5 = goo_canvas_polyline_new (root, FALSE, 1,
				       356.0, 220.0,
				       "start-arrow", TRUE,
				       "end-arrow", TRUE,
				       NULL);
  setup_item_signals (polyline5);
}


void
setup_polygons (GooCanvasItem *root)
{
  GooCanvasItem *polyline1, *polyline2;
  GooCanvasPoints *points;
  cairo_pattern_t *pattern;
  static guchar stipple_data[16] = {
    0, 0, 0, 255,   0, 0, 0, 0,   0, 0, 0, 0,     0, 0, 0, 255
  };

  points = goo_canvas_points_new (3);
  points->coords[0] = 210.0;
  points->coords[1] = 320.0;
  points->coords[2] = 210.0;
  points->coords[3] = 380.0;
  points->coords[4] = 260.0;
  points->coords[5] = 350.0;
  pattern = create_stipple ("blue", stipple_data);
  polyline1 = goo_canvas_polyline_new (root, TRUE, 0,
				       "line-width", 1.0,
				       "points", points,
				       "fill_pattern", pattern,
				       "stroke_color", "black",
				       NULL);
  cairo_pattern_destroy (pattern);
  goo_canvas_points_unref (points);
  setup_item_signals (polyline1);

  points = goo_canvas_points_new (14);
  points->coords[0] = 270.0;
  points->coords[1] = 330.0;
  points->coords[2] = 270.0;
  points->coords[3] = 430.0;
  points->coords[4] = 390.0;
  points->coords[5] = 430.0;
  points->coords[6] = 390.0;
  points->coords[7] = 330.0;
  points->coords[8] = 310.0;
  points->coords[9] = 330.0;
  points->coords[10] = 310.0;
  points->coords[11] = 390.0;
  points->coords[12] = 350.0;
  points->coords[13] = 390.0;
  points->coords[14] = 350.0;
  points->coords[15] = 370.0;
  points->coords[16] = 330.0;
  points->coords[17] = 370.0;
  points->coords[18] = 330.0;
  points->coords[19] = 350.0;
  points->coords[20] = 370.0;
  points->coords[21] = 350.0;
  points->coords[22] = 370.0;
  points->coords[23] = 410.0;
  points->coords[24] = 290.0;
  points->coords[25] = 410.0;
  points->coords[26] = 290.0;
  points->coords[27] = 330.0;
  polyline2 = goo_canvas_polyline_new (root, TRUE, 0,
				       "points", points,
				       "fill_color", "tan",
				       "stroke_color", "black",
				       "line-width", 3.0,
				       NULL);
  goo_canvas_points_unref (points);
  setup_item_signals (polyline2);
}


GooCanvasItem *
make_anchor (GooCanvasItem *root, double x, double y)
{
  GooCanvasItem *group, *item;
  cairo_matrix_t transform = { 0.8, 0.2, -0.3, 0.5, x, y };

  group = goo_canvas_group_new (root, NULL);
  goo_canvas_item_translate (group, x, y);
#if 1
  g_object_set (group,
		"transform", &transform,
		NULL);
#endif

  item = goo_canvas_rect_new (group, -2.5, -2.5, 4, 4,
			      "line-width", 1.0,
			      NULL);
  setup_item_signals (item);

  return group;
}

void 
setup_texts (GooCanvasItem *root)
{
  GooCanvasItem *item;
  cairo_pattern_t *pattern;
  static guchar stipple_data[16] = {
    0, 0, 0, 255,   0, 0, 0, 0,   0, 0, 0, 0,     0, 0, 0, 255
  };

#if 1
  pattern = create_stipple ("blue", stipple_data);
  item = goo_canvas_text_new (make_anchor (root, 420, 20),
			      "Anchor NW", 0, 0, -1, GTK_ANCHOR_NW,
			      "font", "Sans Bold 24",
			      "fill_pattern", pattern,
			      NULL);
  cairo_pattern_destroy (pattern);
  setup_item_signals (item);

  item = goo_canvas_text_new (make_anchor (root, 470, 75),
			      "Anchor center\nJustify center\nMultiline text\nb8bit text ÅÄÖåäö",
			      0, 0, -1, GTK_ANCHOR_CENTER,
			      "font", "monospace bold 14",
			      "alignment", PANGO_ALIGN_CENTER,
			      "fill_color", "firebrick",
			      NULL);
  setup_item_signals (item);
#endif

#if 0
  item = goo_canvas_text_new (make_anchor (root, 590, 140),
			      "Clipped text\nClipped text\nClipped text\nClipped text\nClipped text\nClipped text",
			      0, 0, -1, GTK_ANCHOR_SE,
			      "font", "Sans 12",
			      /*"clip", TRUE,*/
			      /*"clip_width", 50.0,*/
			      /*"clip_height", 55.0,*/
			      /*"x_offset", 10.0,*/
			      "fill_color", "darkgreen",
			      NULL);
  setup_item_signals (item);
#endif

#if 1
  textitem = goo_canvas_text_new (make_anchor (root, 420, 240),
				  "This is a very long paragraph that will need to be wrapped over several lines so we can see what happens to line-breaking as the view is zoomed in and out.",
				  0, 0, 180, GTK_ANCHOR_W,
				  "font", "Sans 12",
				  "fill_color", "goldenrod",
				  NULL);
  setup_item_signals (textitem);
#endif

#if 1
  textitem = goo_canvas_text_new (root,
				  "Ellipsized text.",
				  20, 420, 115, GTK_ANCHOR_W,
				  "font", "Sans 12",
				  "fill_color", "blue",
				  "ellipsize", PANGO_ELLIPSIZE_END,
				  NULL);
  setup_item_signals (textitem);
#endif
}


void
setup_invisible_texts (GooCanvasItem *root)
{
  goo_canvas_text_new (root, "Visible above 0.8x", 500, 330, -1,
		       GTK_ANCHOR_CENTER,
		       "visibility", GOO_CANVAS_ITEM_VISIBLE_ABOVE_THRESHOLD,
		       "visibility-threshold", 0.8,
		       NULL);
  goo_canvas_rect_new (root, 410.5, 322.5, 180, 15,
		       "line-width", 1.0,
		       "visibility", GOO_CANVAS_ITEM_VISIBLE_ABOVE_THRESHOLD,
		       "visibility-threshold", 0.8,
		       NULL);

  goo_canvas_text_new (root, "Visible above 1.5x", 500, 350, -1,
		       GTK_ANCHOR_CENTER,
		       "visibility", GOO_CANVAS_ITEM_VISIBLE_ABOVE_THRESHOLD,
		       "visibility-threshold", 1.5,
		       NULL);
  goo_canvas_rect_new (root, 410.5, 342.5, 180, 15,
		       "line-width", 1.0,
		       "visibility", GOO_CANVAS_ITEM_VISIBLE_ABOVE_THRESHOLD,
		       "visibility-threshold", 1.5,
		       NULL);

  goo_canvas_text_new (root, "Visible above 3.0x", 500, 370, -1,
		       GTK_ANCHOR_CENTER,
		       "visibility", GOO_CANVAS_ITEM_VISIBLE_ABOVE_THRESHOLD,
		       "visibility-threshold", 3.0,
		       NULL);
  goo_canvas_rect_new (root, 410.5, 362.5, 180, 15,
		       "line-width", 1.0,
		       "visibility", GOO_CANVAS_ITEM_VISIBLE_ABOVE_THRESHOLD,
		       "visibility-threshold", 3.0,
		       NULL);

  /* This should never be seen. */
  goo_canvas_text_new (root, "Always Invisible", 500, 390, -1,
		       GTK_ANCHOR_CENTER,
		       "visibility", GOO_CANVAS_ITEM_INVISIBLE,
		       NULL);
  goo_canvas_rect_new (root, 410.5, 350.5, 180, 15,
		       "line-width", 1.0,
		       "visibility", GOO_CANVAS_ITEM_INVISIBLE,
		       NULL);
}

void
plant_flower (GooCanvasItem *root, double x, double y, GtkAnchorType anchor)
{
  cairo_pattern_t *pattern;
  cairo_surface_t *surface;
  GooCanvasItem *image;
  double w, h;

  surface = cairo_image_surface_create_from_png ("flower.png");
  w = cairo_image_surface_get_width (surface);
  h = cairo_image_surface_get_height (surface);

  pattern = cairo_pattern_create_for_surface (surface);
  cairo_surface_destroy (surface);

  image = goo_canvas_image_new (root, NULL, x, y,
			       "pattern", pattern,
			       "width", w,
			       "height", h,
			       NULL);
  cairo_pattern_destroy (pattern);
  setup_item_signals (image);
}
#endif

void
setup_images (GooCanvasItem *root)
{
  GdkPixbuf *im;
  GooCanvasItem *image;

  im = gdk_pixbuf_new_from_file ("toroid.png", NULL);

  if (im)
    {
      double w = gdk_pixbuf_get_width (im);
      double h = gdk_pixbuf_get_height (im);
      image = goo_canvas_image_new (root, im, 100.0 - w / 2, 225.0 - h / 2,
				   "width", w,
				   "height", h,
				   /* "anchor", GTK_ANCHOR_CENTER, */
				   NULL);
      g_object_unref(im);
      setup_item_signals (image);
    }
  else
    g_warning ("Could not find the toroid.png sample file");

  /*plant_flower (root,  20.0, 170.0, GTK_ANCHOR_NW);
  plant_flower (root, 180.0, 170.0, GTK_ANCHOR_NE);
  plant_flower (root,  20.0, 280.0, GTK_ANCHOR_SW);
  plant_flower (root, 180.0, 280.0, GTK_ANCHOR_SE);*/
}


void
setup_canvas (GooCanvas *canvas)
{
  GooCanvasItem *root;

  root = goo_canvas_get_root_item (canvas);
  g_signal_connect (root, "button_press_event",
		    (GtkSignalFunc) on_background_button_press, NULL);

  /* Setup canvas items */
#if 0
  setup_divisions (root);
  setup_rectangles (root);
  setup_ellipses (root);
  setup_lines (root);
  setup_polygons (root);
  setup_texts (root); 
  setup_images (root);
  setup_invisible_texts (root);

  GooCanvasItem *item;
  cairo_pattern_t *pattern;
  static guchar stipple_data[16] = {
    0, 0, 0, 255,   0, 0, 0, 0,   0, 0, 0, 0,     0, 0, 0, 255
  };

  item = goo_canvas_rect_new (root, 20, 30, 50, 30,
			      "stroke-color", "black",
				  "fill-color", "grey",
			      "line-width", 1.0,
			      NULL);
  setup_item_signals (item);
#endif
}


gboolean
on_delete_event (GtkWidget *window,
		 GdkEvent *event,
		 gpointer unused_data)
{
  gtk_main_quit ();
  return FALSE;
}


GtkWidget*
create_window ()
{
  GtkWidget *window;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), 640, 600);
  gtk_widget_show (window);
  g_signal_connect (window, "delete_event", (GtkSignalFunc) on_delete_event,
		    NULL);

  gtk_container_add (GTK_CONTAINER (window), create_page());

  return window;
}


int
main (int argc, char *argv[])
{
  GtkWidget *window;

  gtk_init (&argc, &argv);

  window = create_window ();

  gtk_main ();
  return 0;
}


void
setup_canvas (GtkWidget *canvas)
{
  GooCanvasItem *root;

  root = goo_canvas_get_root_item (GOO_CANVAS (canvas));

  srand(time(NULL));

  /* Absolute. */
  int x, y=0, w, h=5;

  for (x=10; x < 1000 ; ) {
	  w = (rand()%8+1) * 20;
	  y = (rand()%128+1) * 5;
	  rect1 = goo_canvas_rect_new (root, x, y, w, h,
			"fill-color", "lightgrey",
			"stroke-color", "darkgrey",
			"line-width", 0.25,
			NULL);
	  
	  if (rand()%4)
		  x += w;
	  //goo_canvas_item_translate (rect1, 100, 200);
  }
}


GtkWidget *
create_page (void)
{
  GtkWidget *vbox, *hbox, *w, *scrolled_win, *canvas;

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
				       GTK_SHADOW_IN);
  gtk_widget_show (scrolled_win);
  gtk_container_add (GTK_CONTAINER (vbox), scrolled_win);

  canvas = goo_canvas_new ();
  gtk_widget_set_size_request (canvas, 600, 450);
  goo_canvas_set_bounds (GOO_CANVAS (canvas), 0, 0, 1000, 1000);
  gtk_widget_show (canvas);
  gtk_container_add (GTK_CONTAINER (scrolled_win), canvas);

  setup_canvas (canvas);

  return vbox;
}
