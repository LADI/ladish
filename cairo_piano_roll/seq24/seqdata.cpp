//----------------------------------------------------------------------------
//
//  This file is part of seq24.
//
//  seq24 is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  seq24 is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with seq24; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//-----------------------------------------------------------------------------
#include "event.h"
#include "seqdata.h"
//#include "font.h"


seqdata::seqdata(sequence *a_seq, int a_zoom,  Gtk::Adjustment   *a_hadjust): DrawingArea() 
{     
    m_seq = a_seq;
    m_zoom = a_zoom;

    add_events( Gdk::BUTTON_PRESS_MASK | 
		Gdk::BUTTON_RELEASE_MASK |
		Gdk::POINTER_MOTION_MASK |
		Gdk::LEAVE_NOTIFY_MASK |
                Gdk::SCROLL_MASK );

    // in the construor you can only allocate colors, 
    // get_window() returns 0 because we have not be realized
    Glib::RefPtr<Gdk::Colormap> colormap = get_default_colormap();

    //m_text_font_5_7 = Gdk_Font( c_font_5_7 );

    /*m_black = Gdk::Color( "black" );
    m_white = Gdk::Color( "white" );
    m_grey  = Gdk::Color( "grey" );

    colormap->alloc_color( m_black );
    colormap->alloc_color( m_white );
    colormap->alloc_color( m_grey );
*/
    m_dragging = false;

    set_flags(Gtk::CAN_FOCUS );
    set_double_buffered( false );

    set_size_request( 10,  c_dataarea_y );

    m_hadjust = a_hadjust;
    
    m_scroll_offset_ticks = 0;
    m_scroll_offset_x = 0;
} 

void 
seqdata::update_sizes()
{   
    if( is_realized() ) {
        /* create pixmaps with window dimentions */
        
        /*m_pixmap = Gdk::Pixmap::create( m_window,
                                        m_window_x,
                                        m_window_y, -1  );
        update_pixmap();*/
        queue_draw();
    }

}

void 
seqdata::reset()
{
    update_sizes();
    update_pixmap();
    queue_draw();
}


void 
seqdata::redraw()
{
    update_pixmap();
    queue_draw();
}


void 
seqdata::on_realize()
{
    // we need to do the default realize
    Gtk::DrawingArea::on_realize();

    // Now we can allocate any additional resources we need
    m_window = get_window();
    m_window->clear();

    m_hadjust->signal_value_changed().connect( mem_fun( *this, &seqdata::change_horz ));
    
    update_sizes();
    
}


/** Draw the controller number for an event line (vertically) */
void
seqdata::draw_number_on(cairo_t* cairo, int value, double x, double y)
{
	char val[4];
	sprintf( val, "%3d\n", value);
	char num[6];
	memset( num, 0, 6);
	num[0] = val[0];
	num[2] = val[1];
	num[4] = val[2];

	cairo_set_source_rgb(cairo, 0.0, 0.0, 0.0);

	cairo_move_to(cairo, x, y);
	cairo_show_text(cairo, &num[0]);
	cairo_stroke(cairo);
	cairo_move_to(cairo, x, y + 10);
	cairo_show_text(cairo, &num[2]);
	cairo_stroke(cairo);
	cairo_move_to(cairo, x, y + 20);
	cairo_show_text(cairo, &num[4]);
	cairo_stroke(cairo);
}


void 
seqdata::set_zoom( int a_zoom )
{
    if ( m_zoom != a_zoom ){
        m_zoom = a_zoom;
        reset();
    }
}



void 
seqdata::set_data_type( unsigned char a_status, unsigned char a_control = 0  )
{
    m_status = a_status;
    m_cc = a_control;

    this->redraw();
}
   

void 
seqdata::update_pixmap()
{
    //draw_events_on_pixmap();
}

void 
seqdata::draw_events_on(cairo_t* cairo)
{   
    long tick;

    unsigned char d0,d1;

    int event_x;
    int event_width;
    int event_height;

    bool selected;

    int start_tick = m_scroll_offset_ticks ;
    int end_tick = (m_window_x * m_zoom) + m_scroll_offset_ticks;
    
	cairo_save(cairo);

	cairo_set_line_width(cairo, 1.0);

	//printf( "draw_events_on\n" );

	cairo_set_source_rgb(cairo, 1.0, 1.0, 1.0);
    cairo_rectangle(cairo,
                           0.0,
                           0.0, 
                           m_window_x, 
                           m_window_y);
	cairo_fill(cairo);

  
    m_seq->reset_draw_marker();
    while ( m_seq->get_next_event( m_status,
				   m_cc,
				   &tick, &d0, &d1, 
				   &selected ) == true )
    {

        if ( tick >= start_tick && tick <= end_tick ){
            
            /* turn into screen corrids */
            
            event_x = tick / m_zoom;
            event_width  = c_data_x;
            
            /* generate the value */
            event_height = d1;
            
            if ( m_status == EVENT_PROGRAM_CHANGE ||
                 m_status == EVENT_CHANNEL_PRESSURE  ){
                
                event_height = d0;
            }
            
            /*m_gc->set_line_attributes( 2,
                                       Gdk::LINE_SOLID,
                                       Gdk::CAP_NOT_LAST,
                                       Gdk::JOIN_MITER );*/
            
            // Draw vertical line
			if (selected)
				cairo_set_source_rgb(cairo, 1.0, 0.0, 0.0);
			else
				cairo_set_source_rgb(cairo, 0, 0, 0);

			cairo_move_to(cairo, 
                              event_x -  m_scroll_offset_x + 1.5,
                              c_dataarea_y - event_height);
			cairo_line_to(cairo,
                              event_x -  m_scroll_offset_x + 1.5, 
                              c_dataarea_y );
			cairo_stroke(cairo);
            
			/*a_draw->draw_line(m_gc,
                              event_x -  m_scroll_offset_x +1,
                              c_dataarea_y - event_height, 
                              event_x -  m_scroll_offset_x + 1, 
                              c_dataarea_y );*/
					
			// Draw number
			draw_number_on(cairo,
				event_height,
				event_x + 3 - m_scroll_offset_x,
				c_dataarea_y - 23);
        }
    }        

	cairo_restore(cairo);
    
}



/*
void 
seqdata::draw_events_on_pixmap()
{
    draw_events_on( m_pixmap );
}*/


int 
seqdata::idle_redraw()
{
    /* no flicker, redraw */
    if ( !m_dragging ){
	//draw_events_on( m_window );
	//draw_events_on( m_pixmap );
    }
	return true;
}

bool
seqdata::on_expose_event(GdkEventExpose* event)
{
	cairo_t* cairo = gdk_cairo_create(m_window->gobj());

	// Clip
	cairo_rectangle(cairo,
	                 event->area.x, event->area.y,
	                 event->area.width, event->area.height);
	cairo_clip(cairo);

	// Draw everything
	draw_line_on(cairo);
	draw_events_on(cairo);

	cairo_destroy(cairo);
	
 /*   m_window->draw_drawable(m_gc, 
                            m_pixmap, 
                            a_e->area.x,
                            a_e->area.y,
                            a_e->area.x,
                            a_e->area.y,
                            a_e->area.width,
                            a_e->area.height );*/
    return true;
}

/* takes screen corrdinates, give us notes and ticks */
void 
seqdata::convert_x( int a_x, long *a_tick )
{
    *a_tick = a_x * m_zoom; 
}

bool
seqdata::on_scroll_event( GdkEventScroll* a_ev )
{
    if (  a_ev->direction == GDK_SCROLL_UP ){
        m_seq->increment_selected( m_status, m_cc );
    }
    if (  a_ev->direction == GDK_SCROLL_DOWN ){
        m_seq->decrement_selected( m_status, m_cc );
    }

    update_pixmap();
    queue_draw();	 

    return true;
}

bool
seqdata::on_button_press_event(GdkEventButton* a_p0)
{
    if ( a_p0->type == GDK_BUTTON_PRESS ){

        m_seq->push_undo();
        
        /* set values for line */
        m_drop_x = (int) a_p0->x + m_scroll_offset_x;
        m_drop_y = (int) a_p0->y;
        
        /* reset box that holds dirty redraw spot */
        m_old.x = 0;
        m_old.y = 0;
        m_old.width = 0;
        m_old.height = 0;
        
        m_dragging = true;
    }

    return true;
}

bool
seqdata::on_button_release_event(GdkEventButton* a_p0)
{
    m_current_x = (int) a_p0->x + m_scroll_offset_x;
    m_current_y = (int) a_p0->y;

    if ( m_dragging ){

	long tick_s, tick_f;

	if ( m_current_x < m_drop_x ){
	    swap( m_current_x, m_drop_x );
	    swap( m_current_y, m_drop_y );
	}

	convert_x( m_drop_x, &tick_s );
	convert_x( m_current_x, &tick_f );


	if ( m_drop_y < 0 ) m_drop_y = 0;
	if ( m_drop_y > 127 ) m_drop_y = 127;

	if ( m_current_y < 0 ) m_current_y = 0;
	if ( m_current_y > 127 ) m_current_y = 127;

  	m_seq->change_event_data_range( tick_s, tick_f,
					m_status,
					m_cc,
					c_dataarea_y - m_drop_y -1, 
					c_dataarea_y - m_current_y-1 );

	/* convert x,y to ticks, then set events in range */
	m_dragging = false;
    }

    update_pixmap();
    queue_draw();	    
    return true;
}


// Takes two points, returns a Xwin rectangle 
void 
seqdata::xy_to_rect(  int a_x1,  int a_y1,
		      int a_x2,  int a_y2,
		      int *a_x,  int *a_y,
		      int *a_w,  int *a_h )
{
    /* checks mins / maxes..  the fills in x,y
       and width and height */

    if ( a_x1 < a_x2 ){
	*a_x = a_x1; 
	*a_w = a_x2 - a_x1;
    } else {
	*a_x = a_x2; 
	*a_w = a_x1 - a_x2;
    }

    if ( a_y1 < a_y2 ){
	*a_y = a_y1; 
	*a_h = a_y2 - a_y1;
    } else {
	*a_y = a_y2; 
	*a_h = a_y1 - a_y2;
    }
}

bool 
seqdata::on_motion_notify_event(GdkEventMotion* a_p0)
{
#if 0
    if ( m_dragging ){

	m_current_x = (int) a_p0->x + m_scroll_offset_x;
	m_current_y = (int) a_p0->y;

	long tick_s, tick_f;

	int adj_x_min, adj_x_max,
	    adj_y_min, adj_y_max;

	if ( m_current_x < m_drop_x ){

	    adj_x_min = m_current_x;
	    adj_y_min = m_current_y;
	    adj_x_max = m_drop_x;
	    adj_y_max = m_drop_y;

	} else {

	    adj_x_max = m_current_x;
	    adj_y_max = m_current_y;
	    adj_x_min = m_drop_x;
	    adj_y_min = m_drop_y;
	}

	convert_x( adj_x_min, &tick_s );
	convert_x( adj_x_max, &tick_f );

	if ( adj_y_min < 0   ) adj_y_min = 0;
	if ( adj_y_min > 127 ) adj_y_min = 127;
	if ( adj_y_max < 0   ) adj_y_max = 0;
	if ( adj_y_max > 127 ) adj_y_max = 127;	

	m_seq->change_event_data_range( tick_s, tick_f,
					m_status,
					m_cc,
					c_dataarea_y - adj_y_min -1, 
					c_dataarea_y - adj_y_max -1 );
	
	/* convert x,y to ticks, then set events in range */
	update_pixmap();
	  

	draw_events_on( m_window );

        draw_line_on_window();
    }
#endif
    return true;
}


bool
seqdata::on_leave_notify_event(GdkEventCrossing* p0)
{
    m_dragging = false;
    update_pixmap();
    queue_draw();	    
    return true;
}




void 
seqdata::draw_line_on(cairo_t* cairo)
{
    int x,y,w,h;
	cairo_set_source_rgb(cairo, 0, 0, 0);
    /*m_gc->set_line_attributes( 1,
                               Gdk::LINE_SOLID,
                               Gdk::CAP_NOT_LAST,
                               Gdk::JOIN_MITER );*/

   /* replace old */
    /*m_window->draw_drawable(m_gc, 
                            m_pixmap, 
                            m_old.x,
                            m_old.y,
                            m_old.x,
                            m_old.y,
                            m_old.width + 1,
                            m_old.height + 1 );*/

    xy_to_rect ( m_drop_x,
		 m_drop_y,
		 m_current_x,
		 m_current_y,
		 &x, &y,
		 &w, &h );

    x -= m_scroll_offset_x;
    
    m_old.x = x;
    m_old.y = y;
    m_old.width = w;
    m_old.height = h;

	cairo_set_source_rgb(cairo, 0, 0, 0);
    cairo_move_to(cairo,
                        m_current_x - m_scroll_offset_x,
                        m_current_y);
	cairo_line_to(cairo,
                        m_drop_x - m_scroll_offset_x,
                        m_drop_y );
	cairo_stroke(cairo);

}



void
seqdata::change_horz( )
{
    m_scroll_offset_ticks = (int) m_hadjust->get_value();
    m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;

    update_pixmap();
    force_draw();    
}


void
seqdata::on_size_allocate(Gtk::Allocation& a_r )
{
    Gtk::DrawingArea::on_size_allocate( a_r );
    
    m_window_x = a_r.get_width();
    m_window_y = a_r.get_height();
    
    update_sizes(); 
 
}


void
seqdata::force_draw(void )
{
	/*
    m_window->draw_drawable(m_gc, 
                            m_pixmap, 
                            0,
                            0,
                            0,
                            0,
                            m_window_x,
                            m_window_y );*/
}
