//----------------------------------------------------------------------------
//
//  This file is part of seq24.
//
//  seq24 is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free So%ftware Foundation; either version 2 of the License, or
// (at your option) any later version.
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
#include "seqroll.h"
#include <math.h>

seqroll::seqroll(
                  sequence * a_seq,
                  int a_zoom,
                  int a_snap,
                  seqdata * a_seqdata_wid,
                  seqevent * a_seqevent_wid,
                  seqkeys * a_seqkeys_wid,
                  int a_pos, Adjustment * a_hadjust, Adjustment * a_vadjust):
		DrawingArea()
{
	using namespace Menu_Helpers;

	m_seq = a_seq;
	m_zoom = a_zoom;
	m_snap = a_snap;
	m_seqdata_wid = a_seqdata_wid;
	m_seqevent_wid = a_seqevent_wid;
	m_seqkeys_wid = a_seqkeys_wid;
	m_pos = a_pos;

	m_clipboard = new sequence();

	add_events(Gdk::BUTTON_PRESS_MASK |
	            Gdk::BUTTON_RELEASE_MASK |
	            Gdk::POINTER_MOTION_MASK |
	            Gdk::KEY_PRESS_MASK |
	            Gdk::KEY_RELEASE_MASK |
	            Gdk::FOCUS_CHANGE_MASK |
	            Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK);

	m_selecting = false;
	m_moving = false;
	m_moving_init = false;
	m_growing = false;
	m_adding = false;
	m_paste = false;

	m_old_progress_x = 0;

	m_scale = 0;
	m_key = 0;

	m_vadjust = a_vadjust;
	m_hadjust = a_hadjust;

	m_window_x = 10;
	m_window_y = 10;

	m_scroll_offset_ticks = 0;
	m_scroll_offset_key = 0;

	m_scroll_offset_x = 0;
	m_scroll_offset_y = 0;

	m_background_sequence = 0;
	m_drawing_background_seq = false;

	// FIXME: New notes being added need to not redraw everything
	set_double_buffered(true);
	//set_double_buffered(false);
}


void
seqroll::set_background_sequence(bool a_state, int a_seq)
{
	m_drawing_background_seq = a_state;
	m_background_sequence = a_seq;

	//update_pixmap();
	//queue_draw();
}



seqroll::~seqroll()
{
	delete m_clipboard;
}

/* popup menu calls this */
void
seqroll::set_adding(bool a_adding)
{
	if (a_adding) {

		get_window()->set_cursor(Gdk::Cursor(Gdk::PENCIL));
		m_adding = true;

	} else {

		get_window()->set_cursor(Gdk::Cursor(Gdk::LEFT_PTR));
		m_adding = false;
	}
}




void
seqroll::on_realize()
{
	printf("seqroll realize\n");
	
	// we need to do the default realize
	Gtk::DrawingArea::on_realize();

	set_flags(Gtk::CAN_FOCUS);

	// Now we can allocate any additional resources we need
	m_window = get_window();
	m_window->clear();
	
	m_hadjust->signal_value_changed().
	connect(mem_fun(*this, &seqroll::change_horz));
	m_vadjust->signal_value_changed().
	connect(mem_fun(*this, &seqroll::change_vert));

	update_sizes();
}

/*
 
use m_zoom and
 
i % m_seq->get_bpm() == 0
 
 int numberLines = 128 / m_seq->get_bw() / m_zoom; 
    int distance = c_ppqn / 32;
*/

void
seqroll::update_sizes()
{
	/* set default size */


	m_hadjust->set_lower(0);
	m_hadjust->set_upper(m_seq->get_length());
	m_hadjust->set_page_size(m_window_x * m_zoom);
	m_hadjust->set_step_increment(c_ppqn / 4);
	m_hadjust->set_page_increment(c_ppqn / 4);

	int h_max_value =(m_seq->get_length() -(m_window_x * m_zoom));

	if (m_hadjust->get_value() > h_max_value) {
		m_hadjust->set_value(h_max_value);
	}



	m_vadjust->set_lower(0);
	m_vadjust->set_upper(c_num_keys);
	m_vadjust->set_page_size(m_window_y / c_key_y);
	m_vadjust->set_step_increment(12);
	m_vadjust->set_page_increment(12);

	int v_max_value = c_num_keys -(m_window_y / c_key_y);

	if (m_vadjust->get_value() > v_max_value) {
		m_vadjust->set_value(v_max_value);
	}

	/* create pixmaps with window dimentions */
	if (is_realized()) {
		change_vert();
	}
}


void
seqroll::change_horz()
{
	m_scroll_offset_ticks =(int) m_hadjust->get_value();
	m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;

	force_draw();
}

void
seqroll::change_vert()
{
	m_scroll_offset_key =(int) m_vadjust->get_value();
	m_scroll_offset_y = m_scroll_offset_key * c_key_y;

	force_draw();
}

/** Basically resets the whole widget as if it was realized again */
void
seqroll::reset()
{
	m_scroll_offset_ticks =(int) m_hadjust->get_value();
	m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;

	update_sizes();
	//update_pixmap();
	queue_draw();
}

void
seqroll::redraw()
{
	printf("seqroll redraw\n");

	m_scroll_offset_ticks =(int) m_hadjust->get_value();
	m_scroll_offset_x = m_scroll_offset_ticks / m_zoom;

	//update_pixmap();
	queue_draw();
}


/** Updates background */
void
seqroll::draw_background_on(cairo_t * cairo, int clip_x, int clip_y, int clip_w, int clip_h)
{
	//printf("draw_background()\n" );

	cairo_save(cairo);

	// Clear background(white)
	cairo_rectangle(cairo, 0, 0, m_window_x, m_window_y);
	cairo_set_source_rgb(cairo, 1.0, 1.0, 1.0);
	cairo_fill(cairo);

	/* Draw horizontal lines */

	cairo_set_source_rgb(cairo, 0.0, 0.0, 0.0);
	cairo_set_line_width(cairo, 0.1);
	
	int start_tick = clip_y / c_key_y;
	int end_tick = start_tick + (clip_h / c_key_y) + 1;
	
	//printf( "start_tick[%d] end_tick[%d]\n",
	//         start_tick, end_tick);

	for (int i = start_tick; i < end_tick + 1; i++) {
		
		cairo_move_to(cairo, 0.5, (i * c_key_y) + 0.5);
		cairo_line_to(cairo, m_window_x + 0.5, (i * c_key_y) + 0.5);
		cairo_stroke(cairo);

		/*m_gc->set_foreground(m_grey);
		   m_gc->set_line_attributes( 1,
		   Gdk::LINE_ON_OFF_DASH,
		   Gdk::CAP_NOT_LAST,
		   Gdk::JOIN_MITER );
		   gint8 dash = 1;
		   m_gc->set_dashes( 0, &dash, 1 ); */

		/* ????
		if (m_scale != c_scale_off) {

			if (!c_scales_policy[m_scale][((c_num_keys - i)
			                               - m_scroll_offset_key
			                               - 1 +(12 - m_key)) % 12]) {

				cairo_rectangle(cairo,
				                 0, i * c_key_y + 1, m_window_x, c_key_y - 1);

				cairo_fill(cairo);



			}
		}*/
	}

	/*int measure_length_64ths =  m_seq->get_bpm() * 64 /
	   m_seq->get_bw(); */

	//printf( "measure_length_64ths[%d]\n", measure_length_64ths );

	//int measures_per_line =(256 / measure_length_64ths) /(32 / m_zoom);
	//if ( measures_per_line <= 0
	int measures_per_line = 1;

	//printf( "measures_per_line[%d]\n", measures_per_line );

	int ticks_per_measure = m_seq->get_bpm() *(4 * c_ppqn) / m_seq->get_bw();
	int ticks_per_beat =(4 * c_ppqn) / m_seq->get_bw();
	int ticks_per_step = 6 * m_zoom;
	int ticks_per_m_line = ticks_per_measure * measures_per_line;
	/*int start_tick =
	    m_scroll_offset_ticks -(m_scroll_offset_ticks % ticks_per_step);
	int end_tick =(m_window_x * m_zoom) + m_scroll_offset_ticks;*/
	start_tick =
	    (clip_x - (clip_x % ticks_per_step)) * 2 - ticks_per_step;
	end_tick = start_tick + (clip_w*2 - (clip_w % ticks_per_step)) + 2*ticks_per_step;

	//printf( "ticks_per_step[%d] start_tick[%d] end_tick[%d] sot[%d]\n",
	  //       ticks_per_step, start_tick, end_tick, m_scroll_offset_ticks );

	cairo_set_source_rgb(cairo, 0.5, 0.5, 0.5);

	// Draw vertical lines
	for(int i = start_tick; i < end_tick; i += ticks_per_step) {
		int base_line = i / m_zoom;

		// Bar
		if (i % ticks_per_m_line == 0) {
			cairo_set_source_rgba(cairo, 0, 0, 0, 1.0);
			cairo_set_line_width(cairo, 1.0);

		// Beat
		} else if (i % ticks_per_beat == 0) {
			cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 1.0);
			cairo_set_line_width(cairo, 0.5);

		// Tick
		} else {
			cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 1.0);
			cairo_set_line_width(cairo, 0.1);
		}

		cairo_move_to(cairo, base_line - m_scroll_offset_x + 0.5, 0);
		cairo_line_to(cairo, base_line - m_scroll_offset_x + 0.5, m_window_y);
		cairo_stroke(cairo);
	}
	
	cairo_restore(cairo);
}


/* sets zoom, resets */
void
seqroll::set_zoom(int a_zoom)
{
	if (m_zoom != a_zoom) {

		m_zoom = a_zoom;
		reset();
	}
}

/** Simply sets the snap member */
void
seqroll::set_snap(int a_snap)
{
	m_snap = a_snap;
}


void
seqroll::set_note_length(int a_note_length)
{
	m_note_length = a_note_length;
}

/** Sets the music scale */
void
seqroll::set_scale(int a_scale)
{
	if (m_scale != a_scale) {
		m_scale = a_scale;
		reset();
	}

}

/** Sets the key */
void
seqroll::set_key(int a_key)
{
	if (m_key != a_key) {
		m_key = a_key;
		reset();
	}
}


/** Draw progress line (playhead) on roll.  If \a clear_old is true, the old
 * progress indicator will be cleared (and redrawn with background lines and
 * events as necessary).
 */
void
seqroll::draw_progress_on(cairo_t* cairo, bool clear_old)
{
	cairo_save(cairo);

	cairo_set_line_width(cairo, 1.0);

	// Clear old line
	if (clear_old && m_old_progress_x != 0) {

		cairo_save(cairo);

		cairo_rectangle(cairo, m_old_progress_x - 1.0, 0,
				2.0, m_window_y);
		cairo_clip(cairo);

		//draw_background_on(cairo);
		draw_events_on(cairo);
		draw_selection_on(cairo);

		cairo_restore(cairo);
	}

	// Update position
	m_old_progress_x =(m_seq->get_last_tick() / m_zoom) - m_scroll_offset_x;

	// Draw new line
	if (m_old_progress_x != 0) {
		
		cairo_rectangle(cairo, m_old_progress_x - 0.5, 0,
				1.0, m_window_y);
		cairo_clip(cairo);
		
		if (m_seq->get_recording())
			cairo_set_source_rgb(cairo, 0.7, 0.0, 0.0);
		else if (m_seq->get_playing())
			cairo_set_source_rgb(cairo, 0.0, 0.7, 0.0);
		else
			cairo_set_source_rgb(cairo, 0.5, 0.5, 0.5);
			
		cairo_move_to(cairo, m_old_progress_x + 0.5, 0);
		cairo_line_to(cairo, m_old_progress_x + 0.5, m_window_y);
		cairo_stroke(cairo);
	}

	cairo_restore(cairo);
}


void
seqroll::draw_progress_on_window()
{
	cairo_t* cairo = gdk_cairo_create(m_window->gobj());

	draw_progress_on(cairo, true);

	cairo_destroy(cairo);
}


void
seqroll::draw_events_on(cairo_t * cairo)
{
	long tick_s;
	long tick_f;
	int note;

	int note_x;
	int note_width;
	int note_y;
	int note_height;

	bool selected;

	int velocity;

	draw_type dt;

	cairo_save(cairo);

	int start_tick = m_scroll_offset_ticks;
	int end_tick =(m_window_x * m_zoom) + m_scroll_offset_ticks;

	sequence *seq = NULL;
	for(int method = 0; method < 2; ++method) {

		if (method == 0 && m_drawing_background_seq) {

			/*if (m_perform->is_active(m_background_sequence)) {
				seq = m_perform->get_sequence(m_background_sequence);
			} else {*/
				method++;
			//}
		} else if (method == 0) {
			method++;
		}


		if (method == 1) {
			seq = m_seq;
		}

		/* draw boxes from sequence */
		cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 0.7);
		cairo_set_line_width(cairo, 1.0);

		seq->reset_draw_marker();

		while((dt = seq->get_next_note_event(&tick_s, &tick_f, &note,
		                                       &selected,
		                                       &velocity)) != DRAW_FIN) {

			if ((tick_s >= start_tick && tick_s <= end_tick) ||
			       ((dt == DRAW_NORMAL_LINKED) &&
			        (tick_f >= start_tick && tick_f <= end_tick))) {

				/* turn into screen coords */
				note_x = tick_s / m_zoom;
				note_y = c_rollarea_y -(note * c_key_y) - c_key_y;
				note_height = c_key_y - 2;

				//printf( "drawing note[%d] tick_start[%d] tick_end[%d]\n",
				//          note, tick_start, tick_end );

				//int in_shift = 0;
				//int length_add = 0;

				if (dt == DRAW_NORMAL_LINKED) {

					note_width = ((tick_f - tick_s) / m_zoom) + 1;
					if (note_width < 1)
						note_width = 1;

				} else {
					note_width = 16 / m_zoom;
				}
/*
				if (dt == DRAW_NOTE_ON) {

					in_shift = 0;
					length_add = 2;
				}

				if (dt == DRAW_NOTE_OFF) {

					in_shift = -1;
					length_add = 1;
				}
*/
				note_x -= m_scroll_offset_x;
				note_y -= m_scroll_offset_y;

				//m_gc->set_foreground(m_black);

				

				/* draw boxes from sequence */

				//if (method == 0)
				//	cairo_set_source_rgb(cairo, 0.3, 0.3, 0.3);
	
				// Fill
				if (selected)
					cairo_set_source_rgba(cairo, 1.0, 0.0, 0.0, 0.3);
				else
					cairo_set_source_rgba(cairo, 0.3, 0.3, 0.3, 0.3);

				cairo_rectangle(cairo,
				                note_x + 0.5, note_y + 0.5,
								note_width, note_height);
				cairo_fill(cairo);
				
				// Outline
				if (selected)
					cairo_set_source_rgba(cairo, 1.0, 0.0, 0.0, 1.0);
				else
					cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 1.0);
				
				cairo_rectangle(cairo,
				                note_x + 0.5, note_y + 0.5,
								note_width, note_height);
				cairo_stroke(cairo);

#if 0
				/* draw inside box if there is room */
				if (note_width > 3) {

					if (selected)
						cairo_set_source_rgb(cairo, 1.0, 0.0, 0.0);
					else
						cairo_set_source_rgb(cairo, 1.0, 1.0, 1.0);


					/*if ( selected )
					   m_gc->set_foreground(m_red);
					   else
					   m_gc->set_foreground(m_white); */

					if (method == 1) {
						cairo_rectangle(cairo,
						                 note_x + 1 + in_shift,
						                 note_y + 1,
						                 note_width - 3 + length_add,
						                 note_height - 3);
						cairo_fill(cairo);
						/*a_draw->draw_rectangle(m_gc,true,
						   note_x + 1 + in_shift,
						   note_y + 1,
						   note_width - 3 + length_add,
						   note_height - 3); */
					}
				}
#endif
			}

		}
	}
	cairo_restore(cairo);

}


int
seqroll::idle_redraw()
{
	//printf("seqroll idle redraw\n");

	//draw_events_on( m_window );
	//draw_events_on( m_pixmap );

	return true;
}


void
seqroll::draw_selection_on_window()
{
	cairo_t* cairo = gdk_cairo_create(m_window->gobj());
	
	draw_selection_on(cairo);

	cairo_destroy(cairo);
}


void
seqroll::draw_selection_on(cairo_t * cairo)
{
	cairo_save(cairo);

	int x, y, w, h;

	
	/* Replace old */
	if (m_selecting || m_moving || m_paste || m_growing) {
		cairo_save(cairo);
		
		cairo_rectangle(cairo,
				m_old.x-1, m_old.y-1,
				m_old.width+2, m_old.height+2);
		cairo_clip(cairo); // comment this out to test background drawing 'clipping'
		draw_background_on(cairo, m_old.x-1, m_old.y-1, m_old.width+2, m_old.height+2);
		draw_events_on(cairo);

		cairo_restore(cairo);
	}

	if (m_selecting) {

		xy_to_rect(m_drop_x,
		            m_drop_y, m_current_x, m_current_y, &x, &y, &w, &h);

		x -= m_scroll_offset_x;
		y -= m_scroll_offset_y;

		m_old.x = x;
		m_old.y = y;
		m_old.width = w;
		m_old.height = h + c_key_y;
		
		cairo_rectangle(cairo, x, y, w, h + c_key_y);
		cairo_set_source_rgba(cairo, 1.0, 0.0, 0.0, 0.3);
		//cairo_fill(cairo);
		//cairo_fill_preserve(cairo);
		//cairo_set_source_rgba(cairo, 1.0, 0.0, 0.0, 1.0);
		cairo_stroke(cairo);
	}

	if (m_moving || m_paste) {

		int delta_x = m_current_x - m_drop_x;
		int delta_y = m_current_y - m_drop_y;

		x = m_selected.x + delta_x;
		y = m_selected.y + delta_y;

		x -= m_scroll_offset_x;
		y -= m_scroll_offset_y;

		cairo_set_source_rgb(cairo, 0.0, 0.0, 0.0);
		cairo_rectangle(cairo, x, y, m_selected.width, m_selected.height);
		cairo_stroke(cairo);
		m_old.x = x;
		m_old.y = y;
		m_old.width = m_selected.width;
		m_old.height = m_selected.height;
	}

	if (m_growing) {

		int delta_x = m_current_x - m_drop_x;
		int width = delta_x + m_selected.width;

		if (width < 1)
			width = 1;

		x = m_selected.x;
		y = m_selected.y;

		x -= m_scroll_offset_x;
		y -= m_scroll_offset_y;

		cairo_set_source_rgb(cairo, 0.0, 0.0, 0.0);
		cairo_rectangle(cairo, x+0.5, y+0.5, width, m_selected.height);
		cairo_stroke(cairo);

		m_old.x = x;
		m_old.y = y;
		m_old.width = width;
		m_old.height = m_selected.height;

	}

	cairo_restore(cairo);
}

bool
seqroll::on_expose_event(GdkEventExpose * event)
{
	cairo_t* cairo = gdk_cairo_create(m_window->gobj());

	// Clip
	cairo_rectangle(cairo,
	                 event->area.x, event->area.y,
	                 event->area.width, event->area.height);
	cairo_clip(cairo);

	// Draw everything
	
	draw_background_on(cairo, event->area.x, event->area.y, event->area.width, event->area.height);
	draw_events_on(cairo);
	
	//draw_progress_on(cairo, true);
	
	//draw_selection_on(cairo);

	cairo_destroy(cairo);
	
	return true;
}


/** Force a redraw of the entire piano roll.
 * Used when scroll offsets change */
void
seqroll::force_draw()
{
	// FIXME: set a cairo clip region?  does this draw too much?
	//printf("seqroll force draw.\n");
    
	Gdk::Region clip = m_window->get_clip_region();
	m_window->invalidate_region(clip);
	m_window->process_updates(true);
}



/* takes screen corrdinates, give us notes and ticks */
void
seqroll::convert_xy(int a_x, int a_y, long *a_tick, int *a_note)
{
	*a_tick = a_x * m_zoom;
	*a_note =(c_rollarea_y - a_y - 2) / c_key_y;
}


/* notes and ticks to screen corridinates */
void
seqroll::convert_tn(long a_ticks, int a_note, int *a_x, int *a_y)
{
	*a_x = a_ticks / m_zoom;
	*a_y = c_rollarea_y -((a_note + 1) * c_key_y) - 1;
}



/* checks mins / maxes..  the fills in x,y
   and width and height */
void
seqroll::xy_to_rect(int a_x1, int a_y1,
                     int a_x2, int a_y2,
                     int *a_x, int *a_y, int *a_w, int *a_h)
{
	if (a_x1 < a_x2) {
		*a_x = a_x1;
		*a_w = a_x2 - a_x1;
	} else {
		*a_x = a_x2;
		*a_w = a_x1 - a_x2;
	}

	if (a_y1 < a_y2) {
		*a_y = a_y1;
		*a_h = a_y2 - a_y1;
	} else {
		*a_y = a_y2;
		*a_h = a_y1 - a_y2;
	}
}

void
seqroll::convert_tn_box_to_rect(long a_tick_s, long a_tick_f,
                                 int a_note_h, int a_note_l,
                                 int *a_x, int *a_y, int *a_w, int *a_h)
{
	int x1, y1, x2, y2;

	/* convert box to X,Y values */
	convert_tn(a_tick_s, a_note_h, &x1, &y1);
	convert_tn(a_tick_f, a_note_l, &x2, &y2);

	xy_to_rect(x1, y1, x2, y2, a_x, a_y, a_w, a_h);

	*a_h += c_key_y;
}



void
seqroll::start_paste()
{
	long tick_s;
	long tick_f;
	int note_h;
	int note_l;

	snap_x(&m_current_x);
	snap_y(&m_current_x);

	m_drop_x = m_current_x;
	m_drop_y = m_current_y;

	m_paste = true;

	/* get the box that selected elements are in */
	m_seq->get_clipboard_box(&tick_s, &note_h, &tick_f, &note_l);

	convert_tn_box_to_rect(tick_s, tick_f, note_h, note_l,
	                        &m_selected.x,
	                        &m_selected.y,
	                        &m_selected.width, &m_selected.height);

	/* adjust for clipboard being shifted to tick 0 */
	m_selected.x += m_drop_x;
	m_selected.y +=(m_drop_y - m_selected.y);
}


bool seqroll::on_button_press_event(GdkEventButton * a_ev)
{
	int
	numsel;

	long
	tick_s;
	long
	tick_f;
	int
	note_h;
	int
	note_l;

	int
	norm_x,
	norm_y,
	snapped_x,
	snapped_y;

	grab_focus();

	bool
	needs_update = false;

	snapped_x = norm_x =(int)(a_ev->x + m_scroll_offset_x);
	snapped_y = norm_y =(int)(a_ev->y + m_scroll_offset_y);

	snap_x(&snapped_x);
	snap_y(&snapped_y);

	/* y is always snapped */
	m_current_y = m_drop_y = snapped_y;

	/* reset box that holds dirty redraw spot */
	m_old.x = 0;
	m_old.y = 0;
	m_old.width = 0;
	m_old.height = 0;

	if (m_paste) {

		convert_xy(snapped_x, snapped_y, &tick_s, &note_h);
		m_paste = false;
		m_seq->push_undo();
		m_seq->paste_selected(tick_s, note_h);

		needs_update = true;

	} else {

		/*      left mouse button     */
		if (a_ev->button == 1) {

			/* selection, normal x */
			m_current_x = m_drop_x = norm_x;

			/* turn x,y in to tick/note */
			convert_xy(m_drop_x, m_drop_y, &tick_s, &note_h);

			/* set to playing */
			numsel = m_seq->select_note_events(tick_s, note_h, tick_s, note_h);

			/* none selected, start selection box */
			if (numsel == 0 && !m_adding) {

				m_seq->unselect();
				m_selecting = true;
			}

			/* add a new note if we didnt select anything */
			else if (numsel == 0 && m_adding) {

				/* adding, snapped x */
				m_current_x = m_drop_x = snapped_x;
				convert_xy(m_drop_x, m_drop_y, &tick_s, &note_h);

				/* add note, length = little less than snap */
				m_seq->push_undo();
				m_seq->add_note(tick_s, m_note_length - 2, note_h);

				needs_update = true;
			}

			/* if we clicked on an event, we can start moving all selected
			   notes */
			else if (numsel > 0) {

				m_moving_init = true;
				needs_update = true;


				/* get the box that selected elements are in */
				m_seq->get_selected_box(&tick_s, &note_h, &tick_f, &note_l);


				convert_tn_box_to_rect(tick_s, tick_f, note_h, note_l,
				                        &m_selected.x,
				                        &m_selected.y,
				                        &m_selected.width, &m_selected.height);



				/* save offset that we get from the snap above */
				int
				adjusted_selected_x = m_selected.x;
				snap_x(&adjusted_selected_x);
				m_move_snap_offset_x =(m_selected.x - adjusted_selected_x);

				/* align selection for drawing */
				snap_x(&m_selected.x);

				m_current_x = m_drop_x = snapped_x;
			}
		}

		/*     right mouse button      */
		if (a_ev->button == 3) {
			set_adding(true);
		}

		/*     middle mouse button      */
		if (a_ev->button == 2) {

			/* moving, normal x */
			m_current_x = m_drop_x = norm_x;
			convert_xy(m_drop_x, m_drop_y, &tick_s, &note_h);

			/* set to playing */
			numsel = m_seq->select_note_events(tick_s, note_h,
			                                    tick_s + 1, note_h);
			if (numsel > 0) {

				m_growing = true;

				/* get the box that selected elements are in */
				m_seq->get_selected_box(&tick_s, &note_h, &tick_f, &note_l);

				convert_tn_box_to_rect(tick_s, tick_f, note_h, note_l,
				                        &m_selected.x,
				                        &m_selected.y,
				                        &m_selected.width, &m_selected.height);
			}
		}
	}

	/* if they clicked, something changed */
	if (needs_update) {
		// FIXME: draws the whole thing
		//update_pixmap();
		queue_draw();
		m_seqevent_wid->redraw();
		m_seqdata_wid->redraw();
	}
	return true;

}


bool seqroll::on_button_release_event(GdkEventButton * a_ev)
{
	long tick_s;
	long tick_f;
	int note_h;
	int note_l;
	int x, y, w, h;
	int numsel;

	bool needs_update = false;

	m_current_x =(int)(a_ev->x + m_scroll_offset_x);
	m_current_y =(int)(a_ev->y + m_scroll_offset_y);

	snap_y(&m_current_y);

	if (m_moving)
		snap_x(&m_current_x);

	int
	delta_x = m_current_x - m_drop_x;
	int
	delta_y = m_current_y - m_drop_y;

	long
	delta_tick;
	int
	delta_note;

	if (a_ev->button == 1) {

		if (m_selecting) {

			xy_to_rect(m_drop_x,
			            m_drop_y, m_current_x, m_current_y, &x, &y, &w, &h);

			convert_xy(x, y, &tick_s, &note_h);
			convert_xy(x + w, y + h, &tick_f, &note_l);

			numsel = m_seq->select_note_events(tick_s, note_h, tick_f, note_l);

			needs_update = true;
		}

		if (m_moving) {

			/* adjust for snap */
			delta_x -= m_move_snap_offset_x;

			/* convert deltas into screen corridinates */
			convert_xy(delta_x, delta_y, &delta_tick, &delta_note);

			/* since delta_note was from delta_y, it will be filpped
			  ( delta_y[0] = note[127], etc.,so we have to adjust */
			delta_note = delta_note -(c_num_keys - 1);

			m_seq->push_undo();
			m_seq->move_selected_notes(delta_tick, delta_note);
			needs_update = true;
		}
	}

	if (a_ev->button == 2) {

		if (m_growing) {

			/* convert deltas into screen corridinates */
			convert_xy(delta_x, delta_y, &delta_tick, &delta_note);
			m_seq->push_undo();
			m_seq->grow_selected(delta_tick);
			needs_update = true;
		}
	}

	if (a_ev->button == 3) {
		set_adding(false);
	}

	/* turn off */
	m_selecting = false;
	m_moving = false;
	m_growing = false;
	m_paste = false;
	m_moving_init = false;

	/* if they clicked, something changed */
	if (needs_update) {

		//update_pixmap();
		queue_draw();
		m_seqevent_wid->redraw();
		m_seqdata_wid->redraw();
	}
	return true;
}



bool seqroll::on_motion_notify_event(GdkEventMotion * a_ev)
{
	m_current_x =(int)(a_ev->x + m_scroll_offset_x);
	m_current_y =(int)(a_ev->y + m_scroll_offset_y);

	int
	note;
	long
	tick;

	if (m_moving_init) {
		m_moving_init = false;
		m_moving = true;
	}


	snap_y(&m_current_y);
	convert_xy(0, m_current_y, &tick, &note);

	m_seqkeys_wid->set_hint_key(note);

	if (m_selecting || m_moving || m_growing || m_paste) {

		if (m_moving || m_paste) {
			snap_x(&m_current_x);
		}

		draw_selection_on_window();
		return true;

	}
	return false;
}



/* performs a 'snap' on y */
void
seqroll::snap_y(int *a_y)
{
	*a_y = *a_y -(*a_y % c_key_y);
}

/* performs a 'snap' on x */
void
seqroll::snap_x(int *a_x)
{
	//snap = number pulses to snap to
	//m_zoom = number of pulses per pixel
	//so snap / m_zoom  = number pixels to snap to
	int mod =(m_snap / m_zoom);
	if (mod <= 0)
		mod = 1;

	*a_x = *a_x -(*a_x % mod);
}




bool seqroll::on_enter_notify_event(GdkEventCrossing * a_p0)
{
	m_seqkeys_wid->set_hint_state(true);
	return false;
}



bool seqroll::on_leave_notify_event(GdkEventCrossing * a_p0)
{
	m_seqkeys_wid->set_hint_state(false);
	return false;
}



bool seqroll::on_focus_in_event(GdkEventFocus *)
{
	set_flags(Gtk::HAS_FOCUS);

	m_seq->clear_clipboard();

	return false;
}


bool seqroll::on_focus_out_event(GdkEventFocus *)
{
	unset_flags(Gtk::HAS_FOCUS);

	return false;
}

bool seqroll::on_key_press_event(GdkEventKey * a_p0)
{
	bool
		ret = false;

	if (a_p0->type == GDK_KEY_PRESS) {

		if (a_p0->keyval == GDK_Delete) {

			m_seq->push_undo();
			m_seq->remove_selected();
			ret = true;
		}

		if (a_p0->state & GDK_CONTROL_MASK) {

			/* cut */
			if (a_p0->keyval == GDK_x || a_p0->keyval == GDK_X) {

				m_seq->push_undo();
				m_seq->copy_selected();
				m_seq->remove_selected();

				ret = true;
			}
			/* copy */
			if (a_p0->keyval == GDK_c || a_p0->keyval == GDK_C) {

				m_seq->copy_selected();
				m_seq->unselect();
				ret = true;
			}

			/* paste */
			if (a_p0->keyval == GDK_v || a_p0->keyval == GDK_V) {

				start_paste();
				ret = true;
			}
		}
	}

	if ( ret == true ){
		//update_pixmap();
		queue_draw();
		m_seqevent_wid->redraw();
		m_seqdata_wid->redraw();
		return true;
	} else{
		return false;
	}
}




void
seqroll::set_data_type(unsigned char a_status, unsigned char a_control = 0)
{
	m_status = a_status;
	m_cc = a_control;
}



void
seqroll::on_size_allocate(Gtk::Allocation & a_r)
{
	Gtk::DrawingArea::on_size_allocate(a_r);

	m_window_x = a_r.get_width();
	m_window_y = a_r.get_height();

	update_sizes();

}
