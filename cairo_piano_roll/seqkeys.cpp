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
#include "seqkeys.h"
//#include "font.h"


seqkeys::seqkeys(sequence *a_seq,
                 Gtk::Adjustment *a_vadjust ): DrawingArea() 
{     
    m_seq = a_seq;
    
    m_vadjust = a_vadjust;
    
    add_events( Gdk::BUTTON_PRESS_MASK | 
		Gdk::BUTTON_RELEASE_MASK |
                Gdk::ENTER_NOTIFY_MASK |
		Gdk::LEAVE_NOTIFY_MASK |
		Gdk::POINTER_MOTION_MASK );

    /* set default size */
    set_size_request( c_keyarea_x +1, 10 );

    //m_window_x = 10;
    //m_window_y = c_keyarea_y;

    // in the construor you can only allocate colors, 
    // get_window() returns 0 because we have not be realized
    Glib::RefPtr<Gdk::Colormap> colormap = get_default_colormap();

    /*m_black = Gdk::Color( "black" );
    m_white = Gdk::Color( "white" );
    m_grey = Gdk::Color( "grey" );
  
    colormap->alloc_color( m_black );
    colormap->alloc_color( m_white );
    colormap->alloc_color( m_grey );*/
    
    m_keying = false;
    m_hint_state = false;

    m_scroll_offset_key = 0;
    m_scroll_offset_y = 0;

    m_scale = 0;
    m_key = 0;

    set_double_buffered( false );
}

void 
seqkeys::on_realize()
{
    // we need to do the default realize
    Gtk::DrawingArea::on_realize();

    // Now we can allocate any additional resources we need
    m_window = get_window();
    m_window->clear();

	/*
    m_pixmap = Gdk::Pixmap::create(m_window,
                                   c_keyarea_x,
                                   c_keyarea_y,
                                   -1 );*/
  
    update();

    m_vadjust->signal_value_changed().connect( mem_fun( *this, &seqkeys::change_vert ));

    change_vert();
}

/* sets the music scale */
void 
seqkeys::set_scale( int a_scale )
{
  if ( m_scale != a_scale ){
    m_scale = a_scale;
    reset();
  }

}

/* sets the key */
void 
seqkeys::set_key( int a_key )
{
  if ( m_key != a_key ){
    m_key = a_key;
    reset();
  }
}


void 
seqkeys::reset()
{
    update();
    queue_draw();
}


void 
seqkeys::update()
{
	printf("seqkeys update\n");
	
	cairo_t* cairo = gdk_cairo_create(m_window->gobj());

	update_on(cairo);

	cairo_destroy(cairo);
}


void 
seqkeys::update_on(cairo_t * cairo)
{
	cairo_set_line_width(cairo, 1.0);
	
	// Background
	cairo_set_source_rgb(cairo, 1, 1, 1);
    cairo_rectangle(cairo,
                    0, 0, 
                    c_keyarea_x, c_keyarea_y);
	cairo_fill(cairo);
    
	// White box to left of keys
	cairo_set_source_rgb(cairo, 1.0, 1.0, 1.0);
    cairo_rectangle(cairo,
                    1, 1,
                    c_keyoffset_x,
    				c_keyarea_y - 2  );
	cairo_fill(cairo);
    
    
	// Vertical line to right of keys
	/*cairo_set_source_rgb(cairo, 0, 0, 0);
	cairo_move_to(cairo, c_keyoffset_x + c_key_x - 1, 1);
	cairo_line_to(cairo, c_keyoffset_x + c_key_x - 1, c_keyarea_y);
	cairo_stroke(cairo);*/
	
    for ( int i=0; i < c_num_keys; i++ )
	{
		// Horizontal line separating keys
		/*cairo_set_source_rgba(cairo, 0.3, 0.3, 0.3, 1.0);
		cairo_move_to(cairo,
			c_keyoffset_x,
			(c_key_y * i) -  m_scroll_offset_y + 0.5);
		cairo_line_to(cairo, 
			c_keyoffset_x + c_key_x + 1, 
			(c_key_y * i) -  m_scroll_offset_y + 0.5);
		cairo_stroke(cairo);*/
	/*cairo_set_source_rgb(cairo, 0, 0, 0);
		cairo_rectangle(cairo,
			c_keyoffset_x + 1,
			(c_key_y * a_key) + 2 -  m_scroll_offset_y, 
			c_key_x - 4, 
			c_key_y - 3);
		cairo_fill(cairo);*/

		draw_key_on(cairo, i, false);
			
		char notes[20];

		int key = (c_num_keys - i - 1) % 12;
		if ( key == m_key  ){

			/* notes */
			int octave = ((c_num_keys - i - 1) / 12) - 1;
			if ( octave < 0 )
				octave *= -1;

			sprintf( notes, "%1s%1d", c_key_text[key], octave );

			cairo_set_source_rgb(cairo, 0.0, 0.0, 0.0);
			cairo_move_to(cairo, 0, c_key_y * i - m_scroll_offset_y + 4);
			cairo_show_text(cairo, notes);
			/*
			p_font_renderer->render_string_on_drawable(m_gc,
					2, 
					c_key_y * i - 1,
					m_pixmap, notes, font::BLACK );*/
		}

		/*sprintf( notes, "%c %d", c_scales_symbol[m_scale][key], m_scale );

		cairo_set_source_rgb(cairo, 0.0, 0.0, 0.0);
		cairo_move_to(cairo, 2 + (c_text_x * 4), c_key_y * i -1);
		cairo_show_text(cairo, notes);*/
		//p_font_renderer->render_string_on_drawable(m_gc,
		//                                             2 + (c_text_x * 4), 
		//                                             c_key_y * i - 1,
		//                                             m_pixmap, notes, font::BLACK );
	}
}

void
seqkeys::draw_key(int a_key, bool a_state)
{
	cairo_t* cairo = gdk_cairo_create(m_window->gobj());

	draw_key_on(cairo, a_key, a_state);

	cairo_destroy(cairo);
}


/* a_state, false = normal, true = grayed */
void 
seqkeys::draw_key_on(cairo_t* cairo, int a_key, bool a_state )
{
	cairo_save(cairo);

	//cairo_set_line_width(cairo, 0.1);
	//cairo_set_miter_limit(cairo, 0.5);

	cairo_set_line_width(cairo, 0.5);

	/* the key in the octave */
	int key = a_key % 12;

	a_key = c_num_keys - a_key - 1; 

	// Horizontal separating line (vertically centered on key)
	cairo_set_source_rgba(cairo, 0, 0, 0, 0.5);
	cairo_move_to(cairo,
			c_keyoffset_x,
			(c_key_y * a_key) + 3.5 - m_scroll_offset_y + ((c_key_y-4)/4.0));
	cairo_line_to(cairo,
			c_keyoffset_x + c_key_x + 1,
			(c_key_y * a_key) + 3.5 - m_scroll_offset_y + ((c_key_y-4)/4.0));
	cairo_stroke(cairo);

	if (key == 1 || key == 3 || key == 6 || key == 8 || key == 10) {
		// Black key

		// Fill (solid)
		if (a_state)
			cairo_set_source_rgba(cairo, 1.0, 0.3, 0.3, 1.0);
		else
			cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 1.0);

		cairo_rectangle(cairo,
				c_keyoffset_x,
				(c_key_y * a_key) + 2.0 -  m_scroll_offset_y, 
				c_key_x - 5, 
				c_key_y - 3);
		cairo_fill(cairo);

		// Border (AA line)
		/*cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 1.0);
		cairo_rectangle(cairo,
				c_keyoffset_x + 0.5,
				(c_key_y * a_key) + 2.5 -  m_scroll_offset_y, 
				c_key_x - 4, 
				c_key_y - 4);
		cairo_stroke(cairo);*/

	} else {
		// White key

		// Horizontal separating line (aligned with top of key)
		/*cairo_set_source_rgba(cairo, 0, 0, 0, 0.5);
		  cairo_move_to(cairo,
		  c_keyoffset_x,
		  (c_key_y * a_key) + 3.5 - m_scroll_offset_y + ((c_key_y-4)/4.0));
		  cairo_line_to(cairo,
		  c_keyoffset_x + c_key_x + 1,
		  (c_key_y * a_key) + 3.5 - m_scroll_offset_y + ((c_key_y-4)/4.0));
		  cairo_stroke(cairo);*/
		/*
		// Fill (solid)
		if (a_state)
		cairo_set_source_rgba(cairo, 1.0, 0.3, 0.3, 1.0);
		else
		cairo_set_source_rgba(cairo, 1.0, 1.0, 0, 1.0);

		cairo_rectangle(cairo,
		c_keyoffset_x + 0.5,
		(c_key_y * a_key) + 2.5 -  m_scroll_offset_y, 
		c_key_x - 1, 
		c_key_y - 4);
		cairo_fill(cairo);

		// Border (AA line)
		cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 0.0);
		cairo_rectangle(cairo,
		c_keyoffset_x + 0.5,
		(c_key_y * a_key) + 2.5 -  m_scroll_offset_y, 
		c_key_x - 1, 
		c_key_y - 4);
		cairo_stroke(cairo);*/
	}

	cairo_restore(cairo);
}

void 
seqkeys::draw_area()
{
      update();
	  queue_draw();
	  /*
      m_window->draw_drawable(m_gc, 
                              m_pixmap, 
                              0,
                              m_scroll_offset_y,
                              0,
                              0,
                              c_keyarea_x,
                              c_keyarea_y );*/
}


bool
seqkeys::on_expose_event(GdkEventExpose* event)
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
	/*
    m_window->draw_drawable(m_gc, 
                            m_pixmap, 
                            a_e->area.x,
                            a_e->area.y + m_scroll_offset_y,
                            a_e->area.x,
                            a_e->area.y,
                            a_e->area.width,
                            a_e->area.height );*/
    return true;
}


void
seqkeys::force_draw( void )
{
	queue_draw();
	/*
    m_window->draw_drawable(m_gc, 
                            m_pixmap, 
                            0,m_scroll_offset_y,
                            0,0,
                            m_window_x,
                            m_window_y );*/
}


/* takes screen corrdinates, give us notes and ticks */
void 
seqkeys::convert_y( int a_y, int *a_note)
{
    *a_note = (c_rollarea_y - a_y - 2) / c_key_y; 
}


bool
seqkeys::on_button_press_event(GdkEventButton *a_e)
{
    int y,note;
   
    if ( a_e->type == GDK_BUTTON_PRESS ){

	y = (int) a_e->y + m_scroll_offset_y;

	if ( a_e->button == 1 ){
	    
	    m_keying = true;

	    convert_y( y,&note );
	    m_seq->play_note_on(  note );

	    m_keying_note = note;
	}
    }
    return true;
}


bool
seqkeys::on_button_release_event(GdkEventButton* a_e)
{   
    if ( a_e->type == GDK_BUTTON_RELEASE ){

	if ( a_e->button == 1 && m_keying ){
	    
	    m_keying = false;
	    m_seq->play_note_off( m_keying_note );
	}
    }
    return true;
}


bool
seqkeys::on_motion_notify_event(GdkEventMotion* a_p0)
{

    int y, note;
 
    y = (int) a_p0->y + m_scroll_offset_y;
    convert_y( y,&note );

    set_hint_key( note );
    
    if ( m_keying ){

        if ( note != m_keying_note ){

	    m_seq->play_note_off( m_keying_note );
	    m_seq->play_note_on(  note );
	    m_keying_note = note;

	}
    }

    return false;
}



bool
seqkeys::on_enter_notify_event(GdkEventCrossing* a_p0)
{
  set_hint_state( true );
  return false;
}



bool
seqkeys::on_leave_notify_event(GdkEventCrossing* p0)
{
    if ( m_keying ){

	m_keying = false;
	m_seq->play_note_off( m_keying_note );

    }
    set_hint_state( false );

    return true;
}

/* sets key to grey */
void 
seqkeys::set_hint_key( int a_key )
{
    draw_key( m_hint_key, false );
    
    m_hint_key = a_key;
    
    if ( m_hint_state )
        draw_key( a_key, true );
}

/* true == on, false == off */
void 
seqkeys::set_hint_state( bool a_state )
{
    m_hint_state = a_state;
    
    if ( !a_state )
        draw_key( m_hint_key, false );
}


void
seqkeys::change_vert( )
{
   
    m_scroll_offset_key = (int) m_vadjust->get_value();
    m_scroll_offset_y = m_scroll_offset_key * c_key_y,
    
    force_draw();
    
}



void
seqkeys::on_size_allocate(Gtk::Allocation& a_r )
{
    Gtk::DrawingArea::on_size_allocate( a_r );

    m_window_x = a_r.get_width();
    m_window_y = a_r.get_height();

  

    queue_draw();
 
}
