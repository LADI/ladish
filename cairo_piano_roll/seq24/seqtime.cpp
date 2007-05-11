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
#include "seqtime.h"
//#include "font.h"


seqtime::seqtime(sequence *a_seq, int a_zoom,
                 Gtk::Adjustment   *a_hadjust): DrawingArea()
{
	m_seq = a_seq;
	m_zoom = a_zoom;

	m_hadjust = a_hadjust;

	add_events( Gdk::BUTTON_PRESS_MASK |
	            Gdk::BUTTON_RELEASE_MASK );

	// in the construor you can only allocate colors,
	// get_window() returns 0 because we have not be realized
	/*Glib::RefPtr<Gdk::Colormap> colormap = get_default_colormap();

	m_black = Gdk::Color( "black" );
	m_white = Gdk::Color( "white" );
	m_grey  = Gdk::Color( "grey" );

	colormap->alloc_color( m_black );
	colormap->alloc_color( m_white );
	colormap->alloc_color( m_grey );*/

	/* set default size */
	set_size_request( 10, c_timearea_y );

	m_scroll_offset_ticks = 0;
	m_scroll_offset_x = 0;

	set_double_buffered( false );


}



void
seqtime::update_sizes()
{
	/* set these for later */
	if( is_realized() ) {
		/*
				m_pixmap = Gdk::Pixmap::create( m_window,
				                                m_window_x,
				                                m_window_y, -1 );
				update_pixmap();*/
		queue_draw();

	}
}

void
seqtime::on_realize()
{
	// we need to do the default realize
	Gtk::DrawingArea::on_realize();

	//Gtk::Main::idle.connect(mem_fun(this,&seqtime::idleProgress));
	Glib::signal_timeout().connect(mem_fun(*this,&seqtime::idle_progress), 50);

	// Now we can allocate any additional resources we need
	m_window = get_window();
	m_window->clear();
	
	m_hadjust->signal_value_changed().connect( mem_fun( *this, &seqtime::change_horz ));

	update_sizes();
}


void
seqtime::change_horz( )
{
	m_scroll_offset_ticks = (int) m_hadjust->get_value();
	m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;

	//update_pixmap();
	force_draw();
}



void
seqtime::on_size_allocate(Gtk::Allocation & a_r )
{
	Gtk::DrawingArea::on_size_allocate( a_r );

	m_window_x = a_r.get_width();
	m_window_y = a_r.get_height();

	update_sizes();

}



bool
seqtime::idle_progress( )
{
	return true;
}



void
seqtime::set_zoom( int a_zoom )
{
	m_zoom = a_zoom;

	reset();

}

void
seqtime::reset()
{
	m_scroll_offset_ticks = (int) m_hadjust->get_value();
	m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;

	update_sizes();
	//update_pixmap();
}


void
seqtime::redraw()
{

	m_scroll_offset_ticks = (int) m_hadjust->get_value();
	m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;

	//update_pixmap();
	queue_draw();
}

void
seqtime::update_on(cairo_t* cairo)
{
	/* clear background */

	cairo_set_source_rgb(cairo, 1, 1, 1);
	cairo_rectangle(cairo,
	                0,
	                0,
	                m_window_x,
	                m_window_y );
	cairo_fill(cairo);


	// Horizontal bottom border line
	cairo_move_to(cairo, 0, m_window_y - 0.5);
	cairo_line_to(cairo, m_window_x, m_window_y - 0.5);
	cairo_set_line_width(cairo, 1.0);
	cairo_set_source_rgb(cairo, 0, 0, 0);
	cairo_stroke(cairo);

	// at 32, a bar every measure
	// at 16
	/*
	 
	    zoom   32         16         8   m_seq->get_bpm() 
	    
	    ml
	    c_ppqn  
	    *
	    1      128
	    2      64
	    4      32        16         8
	    8      16m       8          4          2       1
	    16     8m        4          2          1       1
	    32     4m        2          1          1       1
	    64     2m        1          1          1       1 
	    128    1m        1          1          1       1
	    
	    
	*/

	int measure_length_32nds =  m_seq->get_bpm() * 32 /
	                            m_seq->get_bw();

	//printf ( "measure_length_32nds[%d]\n", measure_length_32nds );

	int measures_per_line = (128 / measure_length_32nds) / (32 / m_zoom);
	if ( measures_per_line <= 0 )
		measures_per_line = 1;

	//printf( "measures_per_line[%d]\n", measures_per_line );

	int ticks_per_measure = m_seq->get_bpm() *(4 * c_ppqn) / m_seq->get_bw();
	int ticks_per_beat =(4 * c_ppqn) / m_seq->get_bw();
	int ticks_per_step = 6 * m_zoom;
	//int ticks_per_m_line = ticks_per_measure * measures_per_line;
	int start_tick =
	    m_scroll_offset_ticks -(m_scroll_offset_ticks % ticks_per_step);
	int end_tick =(m_window_x * m_zoom) + m_scroll_offset_ticks;
	
	// Beat and Bar lines
	cairo_set_source_rgb(cairo, 0, 0, 0);
	for ( int i=start_tick; i<end_tick; i += ticks_per_beat) {
		if (i > m_seq->get_length())
			break;

		int base_line = i / m_zoom;

		// Bar
		if (i % ticks_per_measure == 0) {
			cairo_move_to(cairo,
					base_line -  m_scroll_offset_x + 0.5,
					0);
			cairo_line_to(cairo,
					base_line -  m_scroll_offset_x + 0.5,
					m_window_y );
			cairo_stroke(cairo);

			// Bar number
			char bar[5];
			sprintf( bar, "%d", (i / ticks_per_measure) + 1  );
			cairo_set_source_rgb(cairo, 0, 0, 0);
			cairo_move_to(cairo, base_line + 2 -  m_scroll_offset_x, 15);
			cairo_show_text(cairo, bar);
		
		// Beat
		} else if (i % ticks_per_beat == 0) {
			cairo_save(cairo);
			
			// Beat number
			char beat[5];
			sprintf( beat, "%ld", (i / ticks_per_beat) % m_seq->get_bpm() + 1  );
			cairo_set_source_rgb(cairo, 0.3, 0.3, 0.3);
			cairo_move_to(cairo, base_line - 2 - m_scroll_offset_x, 16);
			cairo_set_font_size(cairo, 7);
			cairo_show_text(cairo, beat);
			
			cairo_restore(cairo);
			
		}
	}

	long end_x = m_seq->get_length() / m_zoom - m_scroll_offset_x;

	// END rectangle
	cairo_set_source_rgb(cairo, 0, 0, 0);
	cairo_rectangle(cairo,
	                end_x,
	                7,
	                25,
	                10);
	cairo_fill(cairo);

	// END text
	cairo_set_source_rgb(cairo, 1, 1, 1);
	cairo_move_to(cairo, end_x + 1, 16);
	cairo_show_text(cairo, "END");
}


bool
seqtime::on_expose_event(GdkEventExpose* event)
{
	cairo_t* cairo = gdk_cairo_create(m_window->gobj());

	// Clip
	cairo_rectangle(cairo,
	                event->area.x, event->area.y,
	                event->area.width, event->area.height);
	cairo_clip(cairo);

	// Draw everything
	update_on(cairo);

	cairo_destroy(cairo);

	return true;
}

void
seqtime::force_draw( void )
{
	/*
	m_window->draw_drawable(m_gc,
	                        m_pixmap,
	                        0,0,
	                        0,0,
	                        m_window_x,
	                        m_window_y );*/
}

bool
seqtime::on_button_press_event(GdkEventButton* p0)
{
	return false;
}

bool
seqtime::on_button_release_event(GdkEventButton* p0)
{
	return false;
};
