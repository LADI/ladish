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
#include "sequence.h"
#include "seqedit.h"
#include <stdlib.h>
#include <iostream>

using namespace std;

sequence::sequence( )
{
 
    m_editing       = false;
    m_playing       = false;
    m_was_playing   = false;
    m_recording     = false;
    m_thru          = false;
    m_queued        = false;

    m_trigger_copied = false;
        
    m_time_beats_per_measure = 4;
    m_time_beat_width = 4;

    //m_tag           = 0;

    m_name          = c_dummy;
    m_bus           = 0;
    m_length        = 4 * c_ppqn;
    m_midi_channel  = 0;
  
    /* no notes are playing */
    for (int i=0; i< c_midi_notes; i++ )
        m_playing_notes[i] = 0;

    m_last_tick = 0;

    //m_masterbus = NULL;
    m_dirty_main = true;
    m_dirty_edit = true;
    m_dirty_perf = true;
    m_dirty_names = true;
    
    m_song_mute = false;

    m_trigger_offset = 0;
}

void 
sequence::push_undo( void )
{
    lock();
    m_list_undo.push( m_list_event );
    unlock();
}


void 
sequence::pop_undo( void )
{
    lock();

    if (m_list_undo.size() > 0 ){

        m_list_event = m_list_undo.top();
        m_list_undo.pop();
        verify_and_link();
    }

    unlock();
}


void 
sequence::push_trigger_undo( void )
{
    lock();
    m_list_trigger_undo.push( m_list_trigger );

    list<trigger>::iterator i;
    
    for ( i  = m_list_trigger_undo.top().begin();
          i != m_list_trigger_undo.top().end(); i++ ){
	  (*i).m_selected = false;
    }

    
    unlock();
}


void 
sequence::pop_trigger_undo( void )
{
    lock();

    if (m_list_trigger_undo.size() > 0 ){

        m_list_trigger = m_list_trigger_undo.top();
        m_list_trigger_undo.pop();
    }

    unlock();
}


/*
void 
sequence::set_master_midi_bus( mastermidibus *a_mmb )
{
    lock();

 //   m_masterbus = a_mmb;

    unlock();
}
*/

void
sequence::set_song_mute( bool a_mute )
{
    m_song_mute = a_mute;
}

bool
sequence::get_song_mute( void )
{
    return m_song_mute;
}

void 
sequence::set_bpm( long a_beats_per_measure )
{
    lock();
    m_time_beats_per_measure = a_beats_per_measure;
    set_dirty_mp();
    unlock();
}

long 
sequence::get_bpm( void )
{
    return m_time_beats_per_measure;
}

void 
sequence::set_bw( long a_beat_width )
{
    lock();
    m_time_beat_width = a_beat_width;
    set_dirty_mp();
    unlock();
}

long 
sequence::get_bw( void )
{
    return m_time_beat_width;
}


sequence::~sequence()
{
 
}

/* adds event in sorted manner */
void 
sequence::add_event( const event *a_e )
{
    lock();

    m_list_event.push_front( *a_e );
    m_list_event.sort( );

    reset_draw_marker();

    set_dirty();

    unlock();
}

void 
sequence::set_orig_tick( long a_tick )
{
    lock();
    m_last_tick = a_tick;
    unlock();
}


void 
sequence::toggle_queued( void )
{
    lock();
    
    set_dirty_mp();
    
    m_queued = !m_queued;
    m_queued_tick = m_last_tick - (m_last_tick % m_length) + m_length;
    
    unlock();
}

void
sequence::off_queued( void )
{
    
    lock();
    
    set_dirty_mp();
    
    m_queued = false;
    
    unlock();
}

bool 
sequence::get_queued( void )
{
    return m_queued;
}

long 
sequence::get_queued_tick( void )
{
    return m_queued_tick;
}


/* tick comes in as global tick */
    void 
sequence::play( long a_tick, bool a_playback_mode )
{

    lock();

    //printf( "a_tick[%ld] a_playback[%d]\n", a_tick, a_playback_mode );

    /* turns sequence off after we play in this frame */
    bool trigger_turning_off = false;

    long times_played  = m_last_tick / m_length;
    long offset_base   = times_played * m_length; 

    long start_tick = m_last_tick;
    long end_tick = a_tick;
    long trigger_offset = 0;

    /* if we are using our in sequence on/off triggers */
    if ( a_playback_mode ){

        bool trigger_state = false;
        long trigger_tick = 0;


        list<trigger>::iterator i = m_list_trigger.begin();

        while ( i != m_list_trigger.end()){

            if ( (*i).m_tick_start <= end_tick ){
                trigger_state = true;
                trigger_tick = (*i).m_tick_start;
                trigger_offset = (*i).m_offset;
            }

            if ( (*i).m_tick_end <= end_tick ){
                trigger_state = false;
                trigger_tick = (*i).m_tick_end;
                trigger_offset = (*i).m_offset;
            }

            if ( (*i).m_tick_start >  end_tick ||
                    (*i).m_tick_end   >  end_tick )
            {
                break;
            }

            i++;
        }

        /* we had triggers in our slice and its not equal to current state */
        if ( trigger_state != m_playing ){

            //printf( "trigger %d\n", trigger_state );

            /* we are turning on */
            if ( trigger_state ){

                if ( trigger_tick < m_last_tick )
                    start_tick = m_last_tick;
                else
                    start_tick = trigger_tick;

                set_playing( true );

            } else {

                /* we are on and turning off */
                end_tick = trigger_tick;
                trigger_turning_off = true;
            }

        }

        if( m_list_trigger.size() == 0 && 
                m_playing ){

            set_playing(false);

        }

        if (  m_song_mute ){
            set_playing(false);
        }

    }    

    set_trigger_offset(trigger_offset);

    long start_tick_offset = (start_tick + m_length - m_trigger_offset);
    long end_tick_offset = (end_tick + m_length - m_trigger_offset);

    /* play the notes in our frame */
    if ( m_playing ){

        list<event>::iterator e = m_list_event.begin();

        while ( e != m_list_event.end()){


            //printf ( "s[%ld] -> t[%ld] ", start_tick, end_tick  ); (*e).print();
            if ( ((*e).get_timestamp() + offset_base ) >= (start_tick_offset) &&
                    ((*e).get_timestamp() + offset_base ) <= (end_tick_offset) ){

                put_event_on_bus( &(*e) );
                //printf( "bus: ");(*e).print();
            }

            else if ( ((*e).get_timestamp() + offset_base) >  end_tick_offset ){
                break;
            }

            /* advance */
            e++;

            /* did we hit the end ? */
            if ( e == m_list_event.end() ){

                e = m_list_event.begin();
                offset_base += m_length;
            }
        }
    }

    /* if our triggers said we should turn off */
    if ( trigger_turning_off ){

        set_playing( false );
    }

    /* update for next frame */
    m_last_tick = end_tick + 1;
    m_was_playing = m_playing;

    unlock();
}







void 
sequence::zero_markers( void )
{
    lock();

    m_last_tick = 0;

    //m_masterbus->flush( );

    unlock();
}


/* verfies state, all noteons have an off,
   links noteoffs with their ons */
void 
sequence::verify_and_link()
{
    
    list<event>::iterator i;
    list<event>::iterator on;
    list<event>::iterator off;

    lock();

    /* unselect all */
    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){
	(*i).unselect();
	(*i).clear_link();
    }

    on = m_list_event.begin();
	    
    /* pair ons and offs */
    while ( on != m_list_event.end() ){

	/* check for a note on, then look for its
	   note off */
	if ( (*on).is_note_on() ){

	    /* get next possible off node */
	    off = on; off++;

	    while ( off != m_list_event.end() ){
		
		/* is a off event, == notes, and isnt
		   selected  */
		if ( (*off).is_note_off()                  &&
		     (*off).get_note() == (*on).get_note() && 
		     ! (*off).is_selected()                  ){

		    /* link + select */
		    (*on).link( &(*off) );
		    (*off).link( &(*on) );
		    (*on).select(  );
		    (*off).select( );

		    break;
		}
		off++;
	    }
	}    
	on++;
    }

    /* unselect all */
    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){
	(*i).unselect();
    }

    /* kill those not in range */
    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){
	
	/* if our current time stamp is greater then the length */
	if ( (*i).get_timestamp() >= m_length ||
	     (*i).get_timestamp() < 0            ){
	    
	    /* we have to prune it */
	    (*i).select();
	    if ( (*i).is_linked() )
		(*i).get_linked()->select();
	}
    }

    remove_selected( );
    unlock();
}
    




void 
sequence::link_new( )
{
    list<event>::iterator on;
    list<event>::iterator off;

    lock();

    on = m_list_event.begin();

    /* pair ons and offs */
    while ( on != m_list_event.end()){

	/* check for a note on, then look for its
	   note off */
	if ( (*on).is_note_on() &&
	     ! (*on).is_linked() ){
	    
	    /* get next element */
	    off = on; off++;
	    
	    while ( off != m_list_event.end()){

		/* is a off event, == notes, and isnt
		   selected  */
		if ( (*off).is_note_off()                    &&
		     (*off).get_note() == (*on).get_note() && 
		     ! (*off).is_linked()                    ){
		    
		    /* link */
		    (*on).link( &(*off) );
		    (*off).link( &(*on) );
		    
		    break;
		}
		off++;
	    }
	}    
	on++;
    }
    unlock();
}




void 
sequence::remove_selected( )
{
    list<event>::iterator i, t;

    lock();

    i = m_list_event.begin();
    while( i != m_list_event.end() ){
	
	if ((*i).is_selected()){
	    
	    /* if its a note off, and that note is currently
	       playing, send a note off */
	    if ( (*i).is_note_off()  &&
		 m_playing_notes[ (*i).get_note()] > 0 ){
		
			std::cout << "NOTE OFF " << (*i).get_note() << std::endl;
                //m_masterbus->play( m_bus, &(*i), m_midi_channel );
                m_playing_notes[(*i).get_note()]--;
	    }
	   
	    t = i; t++;
	    m_list_event.erase(i);
	    i = t;
	}
	else {

	    i++;
	}
    }

    reset_draw_marker();

    unlock();
}

/* returns the 'box' of the selected items */
void 
sequence::get_selected_box( long *a_tick_s, int *a_note_h, 
			    long *a_tick_f, int *a_note_l )
{

    list<event>::iterator i;

    *a_tick_s = c_maxbeats * c_ppqn;
    *a_tick_f = 0;

    *a_note_h = 0;
    *a_note_l = 128;

    long time;
    int note;

    lock();

    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){

	if( (*i).is_selected() ){
	    
	    time = (*i).get_timestamp();
	    
	    if ( time < *a_tick_s ) *a_tick_s = time;
	    if ( time > *a_tick_f ) *a_tick_f = time;
	    
	    note = (*i).get_note();

	    if ( note < *a_note_l ) *a_note_l = note;
	    if ( note > *a_note_h ) *a_note_h = note;
	}
    }
    
    unlock();
}

void 
sequence::get_clipboard_box( long *a_tick_s, int *a_note_h, 
			     long *a_tick_f, int *a_note_l )
{

    list<event>::iterator i;
    
    *a_tick_s = c_maxbeats * c_ppqn;
    *a_tick_f = 0;
    
    *a_note_h = 0;
    *a_note_l = 128;
    
    long time;
    int note;
    
    lock();

    if ( m_list_clipboard.size() == 0 ) {
	*a_tick_s = *a_tick_f = *a_note_h = *a_note_l = 0; 
    }

    for ( i = m_list_clipboard.begin(); i != m_list_clipboard.end(); i++ ){
	
	time = (*i).get_timestamp();
	
	if ( time < *a_tick_s ) *a_tick_s = time;
	if ( time > *a_tick_f ) *a_tick_f = time;
	
	note = (*i).get_note();
	
	if ( note < *a_note_l ) *a_note_l = note;
	if ( note > *a_note_h ) *a_note_h = note;
    }
   
    unlock();
}



int 
sequence::get_num_selected_notes( )
{
    int ret = 0;

    list<event>::iterator i;

    lock();

    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){

        if( (*i).is_note_on()                &&
            (*i).is_selected() ){
            ret++;
        }
    }

    unlock();

    return ret;

}


int 
sequence::get_num_selected_events( unsigned char a_status, 
                                   unsigned char a_cc )
{
    int ret = 0;
    list<event>::iterator i;

    lock();

    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){

        if( (*i).get_status()    == a_status ){

            unsigned char d0,d1;
            (*i).get_data( &d0, &d1 );

            if ( (a_status == EVENT_CONTROL_CHANGE && d0 == a_cc )
                 || (a_status != EVENT_CONTROL_CHANGE) ){

                if ( (*i).is_selected( ))
                    ret++;
            }
        }
    }
    
    unlock();
    
    return ret;
}


/* selects events in range..  tick start, note high, tick end
   note low */
int
sequence::select_note_events( long a_tick_s, int a_note_h,
			      long a_tick_f, int a_note_l )
{
    int ret=0;
    list<event>::iterator i;

    lock();

    
    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){

        

        
        if( (*i).is_note_off()                &&
            (*i).get_timestamp() >= a_tick_s &&
            (*i).get_note()      <= a_note_h &&
            (*i).get_note()      >= a_note_l ){

              
	    if ( (*i).is_linked() ){

		event *ev = (*i).get_linked();

		if ( ev->get_timestamp() <= a_tick_f ){
		    
		    (*i).select( );
		    ev->select( );
		    ret++;
		}
	    }
        }
	
	if ( ! (*i).is_linked() &&
	     ( (*i).is_note_on() ||
	       (*i).is_note_off() ) &&
	     (*i).get_timestamp()  >= a_tick_s - 16 &&
	     (*i).get_timestamp()  <= a_tick_f && 
	     (*i).get_note()       <= a_note_h &&
	     (*i).get_note()       >= a_note_l ) {

	    (*i).select( );
	    ret++;
	}
    }

    unlock();

    return ret;
}




/* select events in range, returns number
   selected */
int 
sequence::select_events( long a_tick_s, long a_tick_f, 
			 unsigned char a_status, 
			 unsigned char a_cc )
{
    int ret=0;
    list<event>::iterator i;

    lock();

    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){

	if( (*i).get_status()    == a_status && 
	    (*i).get_timestamp() >= a_tick_s &&
	    (*i).get_timestamp() <= a_tick_f ){

	    unsigned char d0,d1;
	    (*i).get_data( &d0, &d1 );

	    if ( (a_status == EVENT_CONTROL_CHANGE &&
		  d0 == a_cc )
		 || (a_status != EVENT_CONTROL_CHANGE) ){

		(*i).select( );
		ret++;
	    }
	}
    }
    
    unlock();
    
    return ret;
}




void
sequence::select_all( void )
{
    lock();

    list<event>::iterator i;

    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ )
	(*i).select( );

    unlock();
}


/* unselects every event */
void 
sequence::unselect( void )
{
    lock();

    list<event>::iterator i;
    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ )
	(*i).unselect();

    unlock();
}


/* removes and adds readds selected in position */
void 
sequence::move_selected_notes( long a_delta_tick, int a_delta_note )
{
    event e;

    lock();

    list<event>::iterator i;

    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){

        /* is it being moved ? */
        if ( (*i).is_selected() ){

            /* copy event */
            e  = (*i);
            e.unselect();

            if ( (e.get_timestamp() + a_delta_tick) >= 0   &&
                 (e.get_note() + a_delta_note)      >= 0   &&
                 (e.get_note() + a_delta_note)      <  c_num_keys ){

                e.set_timestamp( e.get_timestamp() + a_delta_tick );
                e.set_note( e.get_note() + a_delta_note );

                add_event( &e );
            }
        }
    }

    remove_selected();
    verify_and_link();

    unlock();
}

    



/* moves note off event */
void 
sequence::grow_selected( long a_delta_tick )
{
    event *on, *off, e;

    lock();

    list<event>::iterator i;

    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){

        if ( (*i).is_selected() &&
                (*i).is_note_on() &&
                (*i).is_linked() ){

            on = &(*i);
            off = (*i).get_linked();

            long length = 
                off->get_timestamp() - 
                on->get_timestamp() +
                a_delta_tick;

            if ( length < 1 )
                length = 1;

            /* unselect it */
            on->unselect();

            /* copy event */
            e  = *off;
            e.unselect();

            e.set_timestamp( on->get_timestamp() + length );
            add_event( &e );
        }
    }

    remove_selected();
    verify_and_link();
    
    unlock();
}


void 
sequence::increment_selected( unsigned char a_status, unsigned char a_control )
{
    lock();

    list<event>::iterator i;

    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){
	
        if ( (*i).is_selected() &&
             (*i).get_status() == a_status ){
			
            if ( a_status == EVENT_NOTE_ON || 
                 a_status == EVENT_NOTE_OFF || 
                 a_status == EVENT_AFTERTOUCH ||
                 a_status == EVENT_CONTROL_CHANGE || 
                 a_status == EVENT_PITCH_WHEEL ){
				
                (*i).increment_data2();
            }
			
            if ( a_status == EVENT_PROGRAM_CHANGE ||
                 a_status == EVENT_CHANNEL_PRESSURE ){
				
                (*i).increment_data1();
            }
        }
    }
        
    unlock();
}


void 
sequence::decrement_selected(unsigned char a_status, unsigned char a_control )
{
    lock();

    list<event>::iterator i;

    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){
	
        if ( (*i).is_selected() &&
             (*i).get_status() == a_status ){
			
            if ( a_status == EVENT_NOTE_ON || 
                 a_status == EVENT_NOTE_OFF || 
                 a_status == EVENT_AFTERTOUCH ||
                 a_status == EVENT_CONTROL_CHANGE || 
                 a_status == EVENT_PITCH_WHEEL ){
				
                (*i).decrement_data2();
            }
			
            if ( a_status == EVENT_PROGRAM_CHANGE ||
                 a_status == EVENT_CHANNEL_PRESSURE ){
				
                (*i).decrement_data1();
            }
			
        }
    }
        
    unlock();
}



void sequence::clear_clipboard( void )
{
    m_list_clipboard.clear( );
}

void
sequence::copy_selected( void )
{
    list<event>::iterator i;

    lock();

    m_list_clipboard.clear( );

    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){

	if ( (*i).is_selected() ){
	    m_list_clipboard.push_back( (*i) );
	}
    }

    long first_tick = (*m_list_clipboard.begin()).get_timestamp();
    
    for ( i = m_list_clipboard.begin(); i != m_list_clipboard.end(); i++ ){

	(*i).set_timestamp((*i).get_timestamp() - first_tick );
    }

    unlock();
}

void 
sequence::paste_selected( long a_tick, int a_note )
{
    list<event>::iterator i;
    int highest_note = 0;

    lock();

    for ( i = m_list_clipboard.begin(); i != m_list_clipboard.end(); i++ ){
	(*i).set_timestamp((*i).get_timestamp() + a_tick );
    }

    if ((*m_list_clipboard.begin()).is_note_on() ||
	(*m_list_clipboard.begin()).is_note_off() ){
	
	for ( i = m_list_clipboard.begin(); i != m_list_clipboard.end(); i++ )
	    if ( (*i).get_note( ) > highest_note ) highest_note = (*i).get_note();

       

	for ( i = m_list_clipboard.begin(); i != m_list_clipboard.end(); i++ ){

	    (*i).set_note( (*i).get_note( ) - (highest_note - a_note) );
	}
    }

    m_list_event.merge( m_list_clipboard );
    m_list_event.sort();

    verify_and_link();

    reset_draw_marker();
    
    unlock();

}


/* change */
void 
sequence::change_event_data_range( long a_tick_s, long a_tick_f,
				   unsigned char a_status,
				   unsigned char a_cc,
				   unsigned char a_data_s,
				   unsigned char a_data_f )
{
    lock();

    unsigned char d0, d1;
    list<event>::iterator i;

    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){

	/* initially false */
	bool set = false;	
	(*i).get_data( &d0, &d1 );
	
	/* correct status and not CC */
	if ( a_status != EVENT_CONTROL_CHANGE &&
	     (*i).get_status() == a_status )
	    set = true;
	
	/* correct status and correct cc */
	if ( a_status == EVENT_CONTROL_CHANGE &&
	     (*i).get_status() == a_status &&
	     d0 == a_cc )
	    set = true;
	
	/* in range? */
	if ( !((*i).get_timestamp() >= a_tick_s &&
	       (*i).get_timestamp() <= a_tick_f ))
	    set = false;
	
	if ( set ){
	    
	    float weight;
	    
	    /* no divide by 0 */
	    if( a_tick_f == a_tick_s )
		a_tick_f = a_tick_s + 1;
	    
	    /* ratio of v1 to v2 */
	    weight = 
		(float)( (*i).get_timestamp() - a_tick_s ) /
		(float)( a_tick_f - a_tick_s ); 
	    
	    int newdata = (int) 
		((weight         * (float) a_data_f ) +
		 ((1.0f - weight) * (float) a_data_s ));
	    
	    if ( a_status == EVENT_NOTE_ON )
		d1 = newdata;

	    if ( a_status == EVENT_NOTE_OFF )
		d1 = newdata;

	    if ( a_status == EVENT_AFTERTOUCH )
		d1 = newdata;

	    if ( a_status == EVENT_CONTROL_CHANGE )
		d1 = newdata;

	    if ( a_status == EVENT_PROGRAM_CHANGE )
		d0 = newdata; /* d0 == new patch */

	    if ( a_status == EVENT_CHANNEL_PRESSURE )
		d0 = newdata; /* d0 == pressure */

	    if ( a_status == EVENT_PITCH_WHEEL )
		d1 = newdata;

	    (*i).set_data( d0, d1 );
	}	    
    }

    unlock();
}




void 
sequence::add_note( long a_tick, long a_length, int a_note )
{

    lock();

    if ( a_tick >= 0 && 
	 a_note >= 0 &&
	 a_note < c_num_keys ){

	event e;

	e.set_status( EVENT_NOTE_ON );
	e.set_data( a_note, 100 );
	e.set_timestamp( a_tick );

	add_event( &e );

	e.set_status( EVENT_NOTE_OFF );
	e.set_data( a_note, 100 );
	e.set_timestamp( a_tick + a_length );

	add_event( &e );
    }

    verify_and_link();
    unlock();
}


void 
sequence::add_event( long a_tick, 
		     unsigned char a_status,
		     unsigned char a_d0,
		     unsigned char a_d1 )
{
    lock();

    if ( a_tick >= 0 ){
	
	event e;

	e.set_status( a_status );
	e.set_data( a_d0, a_d1 );
	e.set_timestamp( a_tick );

	add_event( &e );
    }
    verify_and_link();

    unlock();
}


void 
sequence::stream_event(  event *a_ev  )
{
    lock();

    /* adjust tick */

    a_ev->mod_timestamp( m_length );

    if ( m_recording ){
	
        add_event( a_ev );
        set_dirty();
    }

    if ( m_thru )
    {
        put_event_on_bus( a_ev );
    }

    link_new();
    /* update view */

    unlock();
} 


void
sequence::set_dirty_mp()
{
    //printf( "set_dirtymp\n" );
    m_dirty_names =  m_dirty_main =  m_dirty_perf = true; 
}


void
sequence::set_dirty()
{
    //printf( "set_dirty\n" );
    m_dirty_names = m_dirty_main =  m_dirty_perf = m_dirty_edit = true;
}


bool
sequence::is_dirty_names( )
{
    lock();

    bool ret = m_dirty_names;
    m_dirty_names = false;
    
    unlock();

    return ret;
}

bool
sequence::is_dirty_main( )
{
    lock();

    bool ret = m_dirty_main;
    m_dirty_main = false;

    unlock();

    return ret;
}


bool
sequence::is_dirty_perf( )
{
    lock();

    bool ret = m_dirty_perf;
    m_dirty_perf = false;

    unlock();

    return ret;
}


bool
sequence::is_dirty_edit( )
{
    lock();

    bool ret = m_dirty_edit;
    m_dirty_edit = false;

    unlock();

    return ret;
}


/* plays a note from the paino roll */
void 
sequence::play_note_on( int a_note )
{
    lock();

    event e;

    e.set_status( EVENT_NOTE_ON );
    e.set_data( a_note, 127 );
	cout << "NOTE ON " << a_note << endl;
 //   m_masterbus->play( m_bus, &e, m_midi_channel ); 

   // m_masterbus->flush();

    unlock();
}


/* plays a note from the paino roll */
void 
sequence::play_note_off( int a_note )
{
    lock();

    event e;

    e.set_status( EVENT_NOTE_OFF );
    e.set_data( a_note, 127 );
	cout << "NOTE OFF " << a_note << endl;
 //   m_masterbus->play( m_bus, &e, m_midi_channel ); 

   // m_masterbus->flush();

    unlock();
}


void 
sequence::clear_triggers( void )
{
    lock();
    m_list_trigger.clear();
    unlock();
}



/* adds trigger, a_state = true, range is on.
   a_state = false, range is off


   is      ie     
   <      ><        ><        >
   es             ee
   <               >
   XX
            
   es ee
   <   >
   <>
               
   es    ee
   <      >
   <    >
                
   es     ee
   <       >
   <    >            
*/ 
void 
sequence::add_trigger( long a_tick, long a_length, long a_offset, bool a_adjust_offset )
{
    lock();

    trigger e;
    
    if ( a_adjust_offset )
        e.m_offset = adjust_offset(a_offset);
    else
        e.m_offset = a_offset;
    
    e.m_selected = false;

    e.m_tick_start  = a_tick;
    e.m_tick_end    = a_tick + a_length - 1;

    list<trigger>::iterator i = m_list_trigger.begin();

    while ( i != m_list_trigger.end() ){

        // Is it inside the new one ? erase
        if ((*i).m_tick_start >= e.m_tick_start &&
            (*i).m_tick_end   <= e.m_tick_end  )
        {
            //printf ( "erase start[%d] end[%d]\n", (*i).m_tick_start, (*i).m_tick_end );
            m_list_trigger.erase(i);
            i = m_list_trigger.begin();
            continue;
        }
        // Is the e's end inside  ?
        else if ( (*i).m_tick_end   >= e.m_tick_end &&
                  (*i).m_tick_start <= e.m_tick_end )
        {
            (*i).m_tick_start = e.m_tick_end + 1;
            //printf ( "mvstart start[%d] end[%d]\n", (*i).m_tick_start, (*i).m_tick_end );
        }
        // Is the last start inside the new end ?
        else if ((*i).m_tick_end   >= e.m_tick_start &&
                 (*i).m_tick_start <= e.m_tick_start )
        {
            (*i).m_tick_end = e.m_tick_start - 1;
            //printf ( "mvend start[%d] end[%d]\n", (*i).m_tick_start, (*i).m_tick_end );
        }

        ++i;
    }

    m_list_trigger.push_front( e );
    m_list_trigger.sort();
    
    unlock();
}

void
sequence::grow_trigger (long a_tick_from, long a_tick_to, long a_length)
{
    lock();

    list<trigger>::iterator i = m_list_trigger.begin();
    
    while ( i != m_list_trigger.end() ){
        
        // Find our pair
        if ((*i).m_tick_start <= a_tick_from &&
            (*i).m_tick_end   >= a_tick_from  )
        {
            long start = (*i).m_tick_start;
            long end   = (*i).m_tick_end;
            
            if ( a_tick_to < start )
            {
                start = a_tick_to;
            }
            
            if ( (a_tick_to + a_length - 1) > end )
            {
                end = (a_tick_to + a_length - 1);
            }
            
            add_trigger( start, end - start + 1, (*i).m_offset );
            break;
        }
        ++i;
    }
    
    unlock();
}


void 
sequence::del_trigger( long a_tick )
{
    lock();

    list<trigger>::iterator i = m_list_trigger.begin();

    while ( i != m_list_trigger.end() ){
        if ((*i).m_tick_start <= a_tick &&
            (*i).m_tick_end   >= a_tick ){
            
            m_list_trigger.erase(i);
            break;
        }
        ++i;
    }

    unlock();
}

void
sequence::set_trigger_offset( long a_trigger_offset )
{
    lock();

    m_trigger_offset = (a_trigger_offset % m_length);
    m_trigger_offset += m_length;
    m_trigger_offset %= m_length;
    
    unlock();
}


long
sequence::get_trigger_offset( void )
{
    return m_trigger_offset;
}

void
sequence::split_trigger( trigger &trig, long a_split_tick)
{
    lock();

    long new_tick_end   = trig.m_tick_end;
    long new_tick_start = a_split_tick;
    
    trig.m_tick_end = a_split_tick - 1;
    
    long length = new_tick_end - new_tick_start;
    if ( length > 1 )
        add_trigger( new_tick_start, length + 1, trig.m_offset );
    
    unlock();
}

#if 0
/*
  |...|...|...|...|...|...|...

  0123456789abcdef0123456789abcdef
  [      ][      ][      ][      ][      ][

  [  ][      ][  ][][][][][      ]  [  ][  ]
  0   4       4   0 7 4 2 0         6   2  
  0   4       4   0 1 4 6 0         2   6 inverse offset

  [              ][              ][              ]
  [  ][      ][  ][][][][][      ]  [  ][  ]
  0   c       4   0 f c a 8         e   a
  0   4       c   0 1 4 6 8         2   6  inverse offset

  [                              ][
  [  ][      ][  ][][][][][      ]  [  ][  ]
  k   g f c a 8
  0   4       c   g h k m n       inverse offset

  0123456789abcdefghijklmonpq
  ponmlkjihgfedcba9876543210
  0fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210

*/

#endif


void
sequence::adjust_trigger_offsets_to_legnth( long a_new_len )
{
    lock();
    
    // for all triggers, and undo triggers
    list<trigger>::iterator i = m_list_trigger.begin();
    
    while ( i != m_list_trigger.end() ){

        i->m_offset = adjust_offset( i->m_offset );
        i->m_offset = m_length - i->m_offset; // flip
        long inverse_offset = m_length - (i->m_tick_start % m_length);

        long local_offset = (inverse_offset - i->m_offset);
        local_offset %= m_length;

        long inverse_offset_new = a_new_len - (i->m_tick_start % a_new_len);

        long new_offset = inverse_offset_new - local_offset;

        i->m_offset = (new_offset % a_new_len);
        i->m_offset = a_new_len - i->m_offset;
        
        ++i;
    }
    
    unlock();
    
}

#if 0    

... a
[      ][      ]
...
... a
...

         
    
5   7    play
3        offset
8   10   play

    

X...X...X...X...X...X...X...X...X...X...
L       R    
[        ] [     ]  []  orig
[                    ]
        
<<
    [     ]    [  ][ ]  [] split on the R marker, shift first
    [     ]        [     ]   
    delete middle
    [     ][ ]  []         move ticks
    [     ][     ]

    L       R        
    [     ][ ] [     ]  [] split on L
    [     ][             ]
        
    [     ]        [ ] [     ]  [] increase all after L
    [     ]        [             ]

        
#endif

    void
    sequence::copy_triggers( long a_start_tick, 
                             long a_distance  )
{

    long from_start_tick = a_start_tick + a_distance;
    long from_end_tick = from_start_tick + a_distance - 1;

    lock();

    move_triggers( a_start_tick, 
		   a_distance, 
		   true );
    
    list<trigger>::iterator i = m_list_trigger.begin();
    while(  i != m_list_trigger.end() ){


        
	if ( (*i).m_tick_start >= from_start_tick &&
             (*i).m_tick_start <= from_end_tick )
        {
            trigger e;
            e.m_offset = (*i).m_offset;
            e.m_selected = false;

            e.m_tick_start  = (*i).m_tick_start - a_distance;
        
            if ((*i).m_tick_end   <= from_end_tick )
            {
                e.m_tick_end  = (*i).m_tick_end - a_distance;
            }

            if ((*i).m_tick_end   > from_end_tick )
            {
                e.m_tick_end = from_start_tick -1;
            }

            e.m_offset += (m_length - (a_distance % m_length));
            e.m_offset %= m_length;

            if ( e.m_offset < 0 )
                e.m_offset += m_length;
            
            

            m_list_trigger.push_front( e );
        }

        ++i;
    }
    
    m_list_trigger.sort();
    
    unlock();

}


void
sequence::split_trigger( long a_tick )
{

    lock();
    
    list<trigger>::iterator i = m_list_trigger.begin();
    while(  i != m_list_trigger.end() ){

        // trigger greater than L and R
        if ( (*i).m_tick_start <= a_tick &&
             (*i).m_tick_end >= a_tick )
        {
            //printf( "split trigger %ld %ld\n", (*i).m_tick_start, (*i).m_tick_end );
            {
                long tick = (*i).m_tick_end - (*i).m_tick_start;
                tick += 1;
                tick /= 2;
                
                split_trigger(*i, (*i).m_tick_start + tick);              
                break;
            }
        }        
        ++i;
    }
    unlock();
}

void 
sequence::move_triggers( long a_start_tick, 
			 long a_distance, 
			 bool a_direction )
{

    long a_end_tick = a_start_tick + a_distance;
    //printf( "move_triggers() a_start_tick[%d] a_distance[%d] a_direction[%d]\n",
    //        a_start_tick, a_distance, a_direction );
    
    lock();
    
    list<trigger>::iterator i = m_list_trigger.begin();
    while(  i != m_list_trigger.end() ){

        // trigger greater than L and R
	if ( (*i).m_tick_start < a_start_tick &&
             (*i).m_tick_end > a_start_tick )
        {
            if ( a_direction ) // forward
            {
                split_trigger(*i,a_start_tick);              
            }
            else // back
            {
                split_trigger(*i,a_end_tick);
            }
        }        
        
        // triggers on L
	if ( (*i).m_tick_start < a_start_tick &&
             (*i).m_tick_end > a_start_tick )
        {
            if ( a_direction ) // forward
            {
                split_trigger(*i,a_start_tick);           
            }
            else // back
            {
                (*i).m_tick_end = a_start_tick - 1;
            }
        }

        // In betweens
        if ( (*i).m_tick_start >= a_start_tick &&
             (*i).m_tick_end <= a_end_tick &&
             !a_direction )
        {
            m_list_trigger.erase(i);
            i = m_list_trigger.begin();
        }

        // triggers on R
	if ( (*i).m_tick_start < a_end_tick &&
             (*i).m_tick_end > a_end_tick )
        {
            if ( !a_direction ) // forward
            {
                (*i).m_tick_start = a_end_tick;
            }
        }

        ++i;
    }


    i = m_list_trigger.begin();
    while(  i != m_list_trigger.end() ){

        if ( a_direction ) // forward
        {
            if ( (*i).m_tick_start >= a_start_tick ){
                (*i).m_tick_start += a_distance;
                (*i).m_tick_end   += a_distance;
                (*i).m_offset += a_distance;
                (*i).m_offset %= m_length;
                
            }         
        }
        else // back
        {
            if ( (*i).m_tick_start >= a_end_tick ){
                (*i).m_tick_start -= a_distance;
                (*i).m_tick_end   -= a_distance;

                (*i).m_offset += (m_length - (a_distance % m_length));
                (*i).m_offset %= m_length;
                
            }  
        }

        (*i).m_offset = adjust_offset( (*i).m_offset );


        ++i;
    }
    

 
    unlock();

}

long
sequence::get_selected_trigger_start_tick( void )
{
    long ret = -1;
    lock();

    list<trigger>::iterator i = m_list_trigger.begin();
    
    while(  i != m_list_trigger.end() ){
        
	if ( i->m_selected ){

            ret = i->m_tick_start;
        }

        ++i;
    }
    
    unlock();

    return ret;
}

long
sequence::get_selected_trigger_end_tick( void )
{
    long ret = -1;
    lock();

    list<trigger>::iterator i = m_list_trigger.begin();
    
    while(  i != m_list_trigger.end() ){
        
	if ( i->m_selected ){

            ret = i->m_tick_end;
        }

        ++i;
    }
    
    unlock();

    return ret;
}


void
sequence::move_selected_triggers_to( long a_tick, bool a_adjust_offset, int a_which )
{

    lock();

    long min_tick = 0;
    long max_tick = 0x7ffffff;

    list<trigger>::iterator i = m_list_trigger.begin();
    list<trigger>::iterator s = m_list_trigger.begin();


    // min_tick][0                1][max_tick
    //                   2
    
    while(  i != m_list_trigger.end() ){
        
	if ( i->m_selected ){

            s = i;

            if (     i != m_list_trigger.end() &&
                     ++i != m_list_trigger.end())
            {
                max_tick = (*i).m_tick_start - 1;
            }

            
            // if we are moving the 0, use first as offset
            // if we are moving the 1, use the last as the offset
            // if we are moving both (2), use first as offset
            
            long a_delta_tick = 0;

            if ( a_which == 1 )
            {
                a_delta_tick = a_tick - s->m_tick_end;
            
                if (  a_delta_tick > 0 &&
                      (a_delta_tick + s->m_tick_end) > max_tick )
                {
                    a_delta_tick = ((max_tick) - s->m_tick_end);
                }

                // not past first         
                if (  a_delta_tick < 0 &&
                      (a_delta_tick + s->m_tick_end <= (s->m_tick_start + c_ppqn / 8 )))
                {
                    a_delta_tick = ((s->m_tick_start + c_ppqn / 8 ) - s->m_tick_end);
                }
            }

            if ( a_which == 0 )
            {
                a_delta_tick = a_tick - s->m_tick_start;
                
                if (  a_delta_tick < 0 &&
                      (a_delta_tick + s->m_tick_start) < min_tick )
                {
                    a_delta_tick = ((min_tick) - s->m_tick_start);
                }

                // not past last        
                if (  a_delta_tick > 0 &&
                      (a_delta_tick + s->m_tick_start >= (s->m_tick_end - c_ppqn / 8 )))
                {
                    a_delta_tick = ((s->m_tick_end - c_ppqn / 8 ) - s->m_tick_start);
                }                
            }
         
            if ( a_which == 2 )
            {
                a_delta_tick = a_tick - s->m_tick_start;
                
                if (  a_delta_tick < 0 &&
                      (a_delta_tick + s->m_tick_start) < min_tick )
                {
                    a_delta_tick = ((min_tick) - s->m_tick_start);
                }
            
                if ( a_delta_tick > 0 &&
                     (a_delta_tick + s->m_tick_end) > max_tick )
                {
                    a_delta_tick = ((max_tick) - s->m_tick_end);
                }
            }
            
            if ( a_which == 0 || a_which == 2 )
                s->m_tick_start += a_delta_tick;
            
            if ( a_which == 1 || a_which == 2 )
                s->m_tick_end   += a_delta_tick;
            
            if ( a_adjust_offset ){
                
                s->m_offset += a_delta_tick;
                s->m_offset = adjust_offset( s->m_offset );
            }
            
            break;
	}
        else {
            min_tick = (*i).m_tick_end + 1;
        }

        ++i;
    }

    unlock();
}


long
sequence::get_max_trigger( void )
{
    lock();

    long ret;

    if ( m_list_trigger.size() > 0 )
	ret = m_list_trigger.back().m_tick_end;
    else
	ret = 0;

    unlock();

    return ret;
}

long
sequence::adjust_offset( long a_offset )
{    
    a_offset %= m_length;
    
    if ( a_offset < 0 )
        a_offset += m_length;

    return a_offset;
}

bool 
sequence::get_trigger_state( long a_tick )
{
    lock();

    bool ret = false;
    list<trigger>::iterator i;

    for ( i = m_list_trigger.begin(); i != m_list_trigger.end(); i++ ){

	if ( (*i).m_tick_start <= a_tick &&
             (*i).m_tick_end >= a_tick){
	    ret = true;
            break;
        }
    }

    unlock();

    return ret;
}



bool 
sequence::select_trigger( long a_tick )
{
    lock();

    bool ret = false;
    list<trigger>::iterator i;
    
    for ( i = m_list_trigger.begin(); i != m_list_trigger.end(); i++ ){

	if ( (*i).m_tick_start <= a_tick &&
             (*i).m_tick_end   >= a_tick){
            
            (*i).m_selected = true;
	    ret = true;
        }        
    }
    
    unlock();
    
    return ret;
}


bool 
sequence::unselect_triggers( void )
{
    lock();

    bool ret = false;
    list<trigger>::iterator i;

    for ( i = m_list_trigger.begin(); i != m_list_trigger.end(); i++ ){
        (*i).m_selected = false;
    }

    unlock();

    return ret;
}



void
sequence::del_selected_trigger( void )
{
    lock();
    
    list<trigger>::iterator i;
    
    for ( i = m_list_trigger.begin(); i != m_list_trigger.end(); i++ ){

        if ( i->m_selected ){
            m_list_trigger.erase(i);
            break;
        }
    }
    
    unlock();
}


void
sequence::cut_selected_trigger( void )
{
    copy_selected_trigger();
    del_selected_trigger();
}


void
sequence::copy_selected_trigger( void )
{
    lock();
    
    list<trigger>::iterator i;
    
    for ( i = m_list_trigger.begin(); i != m_list_trigger.end(); i++ ){
        
        if ( i->m_selected ){
            m_trigger_clipboard = *i;
            m_trigger_copied = true;
            break;
        }
    }
    
    unlock();
}


void
sequence::paste_trigger( void )
{
    if ( m_trigger_copied ){
        long length =  m_trigger_clipboard.m_tick_end -
            m_trigger_clipboard.m_tick_start + 1;
        // paste at copy end
        add_trigger( m_trigger_clipboard.m_tick_end + 1,
                     length,
                     m_trigger_clipboard.m_offset + length );
        
        m_trigger_clipboard.m_tick_start = m_trigger_clipboard.m_tick_end +1;
        m_trigger_clipboard.m_tick_end = m_trigger_clipboard.m_tick_start + length - 1;
        
        m_trigger_clipboard.m_offset += length;
        m_trigger_clipboard.m_offset = adjust_offset(m_trigger_clipboard.m_offset);
    }
}


/* this refreshes the play marker to the LastTick */
void 
sequence::reset_draw_marker( void )
{
    lock();
    
    m_iterator_draw = m_list_event.begin();
    
    unlock();
}

void
sequence::reset_draw_trigger_marker( void )
{
    lock();

    m_iterator_draw_trigger = m_list_trigger.begin();

    unlock();
}


int
sequence::get_lowest_note_event( void )
{
    lock();

    int ret = 127;
    list<event>::iterator i;

    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){

	if ( (*i).is_note_on() || (*i).is_note_off() )
	    if ( (*i).get_note() < ret )
		ret = (*i).get_note();
    }

    unlock();

    return ret;
}



int
sequence::get_highest_note_event( void )
{
    lock();

    int ret = 0;
    list<event>::iterator i;

    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){

	if ( (*i).is_note_on() || (*i).is_note_off() )
	    if ( (*i).get_note() > ret )
		ret = (*i).get_note();
    }

    unlock();

    return ret;
}

draw_type 
sequence::get_next_note_event( long *a_tick_s,
			       long *a_tick_f,
			       int  *a_note,
			       bool *a_selected,
			       int  *a_velocity  )
{

    draw_type ret = DRAW_FIN;
    *a_tick_f = 0;

    while (  m_iterator_draw  != m_list_event.end() )
    {
	*a_tick_s   = (*m_iterator_draw).get_timestamp();
	*a_note     = (*m_iterator_draw).get_note();
	*a_selected = (*m_iterator_draw).is_selected();
	*a_velocity = (*m_iterator_draw).get_note_velocity();

	/* note on, so its linked */
	if( (*m_iterator_draw).is_note_on() &&
	    (*m_iterator_draw).is_linked() ){

	    *a_tick_f   = (*m_iterator_draw).get_linked()->get_timestamp();
	    
	    ret = DRAW_NORMAL_LINKED;
	    m_iterator_draw++;
	    return ret;
	}
	
	else if( (*m_iterator_draw).is_note_on() &&
		 (! (*m_iterator_draw).is_linked()) ){
	    
	    ret = DRAW_NOTE_ON;
	    m_iterator_draw++;
	    return ret;
	}
	
	else if( (*m_iterator_draw).is_note_off() &&
		 (! (*m_iterator_draw).is_linked()) ){
	    
	    ret = DRAW_NOTE_OFF;
	    m_iterator_draw++;
	    return ret;
	}
	
	/* keep going until we hit null or find a NoteOn */
	m_iterator_draw++;
    }
    return DRAW_FIN;
}


bool
sequence::get_next_event( unsigned char *a_status,
                          unsigned char *a_cc)
{
    unsigned char j;
    
    while (  m_iterator_draw  != m_list_event.end() ){
        
        *a_status = (*m_iterator_draw).get_status();
        (*m_iterator_draw).get_data( a_cc, &j );
        
        /* we have a good one */
        /* update and return */
        m_iterator_draw++;
        return true;
    }
    return false;
}


bool 
sequence::get_next_event( unsigned char a_status,
			  unsigned char a_cc,
			  long *a_tick,
			  unsigned char *a_D0,
			  unsigned char *a_D1,
			  bool *a_selected )
{
    while (  m_iterator_draw  != m_list_event.end() ){ 
	
	/* note on, so its linked */
	if( (*m_iterator_draw).get_status() == a_status ){
	    
	    (*m_iterator_draw).get_data( a_D0, a_D1 );
	    *a_tick   = (*m_iterator_draw).get_timestamp();
	    *a_selected = (*m_iterator_draw).is_selected();
	    
	    /* either we have a control chage with the right CC
	       or its a different type of event */
	    if ( (a_status == EVENT_CONTROL_CHANGE &&
		  *a_D0 == a_cc )
		 || (a_status != EVENT_CONTROL_CHANGE) ){
		
		/* we have a good one */
		/* update and return */
		m_iterator_draw++;
		return true;
	    }
	}
	/* keep going until we hit null or find a NoteOn */
	m_iterator_draw++;
    }
    return false;
}

bool 
sequence::get_next_trigger( long *a_tick_on, long *a_tick_off, bool *a_selected, long *a_offset )
{
    while (  m_iterator_draw_trigger  != m_list_trigger.end() ){ 

	*a_tick_on  = (*m_iterator_draw_trigger).m_tick_start;
        *a_selected = (*m_iterator_draw_trigger).m_selected;
        *a_offset =   (*m_iterator_draw_trigger).m_offset;
	*a_tick_off = (*m_iterator_draw_trigger).m_tick_end;
	m_iterator_draw_trigger++;

	return true;
    }
    return false;
}


void 
sequence::remove_all( void )
{
    lock();

	

    m_list_event.clear();

    unlock();

}

sequence& 
sequence::operator= (const sequence& a_rhs)
{
    lock();

    /* dont copy to self */
    if (this != &a_rhs){
	
	m_list_event   = a_rhs.m_list_event;
	m_list_trigger   = a_rhs.m_list_trigger;

	m_midi_channel = a_rhs.m_midi_channel;
	//m_masterbus    = a_rhs.m_masterbus;
	m_bus          = a_rhs.m_bus;
	m_name         = a_rhs.m_name;
	m_length       = a_rhs.m_length;

	m_time_beats_per_measure = a_rhs.m_time_beats_per_measure;
	m_time_beat_width = a_rhs.m_time_beat_width;

	m_playing      = false;

	/* no notes are playing */
	for (int i=0; i< c_midi_notes; i++ )
	    m_playing_notes[i] = 0;

	/* reset */
	zero_markers( );

    }

    verify_and_link();

    unlock();

    return *this;

}



void 
sequence::lock( )
{
//    m_mutex.lock();
}


void 
sequence::unlock( )
{   
    //m_mutex.unlock();
}



const char* 
sequence::get_name()
{
    return m_name.c_str();
}

long 
sequence::get_last_tick( )
{
    return (m_last_tick + (m_length -  m_trigger_offset)) % m_length;
}

void
sequence::set_midi_bus( char  a_mb )
{
    lock();
    
    /* off notes except initial */
    off_playing_notes( );
    
    this->m_bus = a_mb;
    set_dirty();

    unlock();
}

char
sequence::get_midi_bus(  )
{
    return this->m_bus;
}



void 
sequence::set_length( long a_len, bool a_adjust_triggers )
{
    lock();
 
    bool was_playing = get_playing();

    /* turn everything off */
    set_playing( false );

    if ( a_len < (c_ppqn / 4) )
        a_len = (c_ppqn /4);

    if ( a_adjust_triggers )
        adjust_trigger_offsets_to_legnth( a_len  );
    
    m_length = a_len;
    
    verify_and_link();

    reset_draw_marker();
    
    /* start up and refresh */
    if ( was_playing )
	set_playing( true );
    
    unlock();
}


long 
sequence::get_length( )
{
    return m_length;
}



void 
sequence::set_playing( bool a_p )
{
    lock();

    if ( a_p != get_playing() )
    {
    
        if (a_p){
	
            /* turn on */
            m_playing = true;

        } else {

            /* turn off */
            m_playing = false;
            off_playing_notes();

        }

        set_dirty();
    }
    
    m_queued = false;

    unlock();
}


void 
sequence::toggle_playing()
{
    set_playing( ! get_playing() );
}

bool 
sequence::get_playing( )
{
    return m_playing;
}



void 
sequence::set_recording( bool a_r )
{
    lock();
    m_recording = a_r;
    unlock();
}


bool 
sequence::get_recording( )
{
    return m_recording;
}



void 
sequence::set_thru( bool a_r )
{
    lock();
    m_thru = a_r;
    unlock();
}


bool 
sequence::get_thru( )
{
    return m_thru;
}


/* sets sequence name */
void 
sequence::set_name( char *a_name )
{
    m_name = a_name;
    set_dirty_mp();
}

void
sequence::set_name( string a_name )
{
    m_name = a_name;
    set_dirty_mp();
}

void 
sequence::set_midi_channel( unsigned char a_ch )
{
    lock();
    off_playing_notes( );
    m_midi_channel = a_ch;
    set_dirty();
    unlock();
}

unsigned char 
sequence::get_midi_channel( )
{
    return m_midi_channel;
}


void 
sequence::print()
{
    printf("[%s]\n", m_name.c_str()  );

    for( list<event>::iterator i = m_list_event.begin(); i != m_list_event.end(); i++ )
	(*i).print();
    printf("events[%zd]\n\n",m_list_event.size());

}


void 
sequence::print_triggers()
{
    printf("[%s]\n", m_name.c_str()  );

    for( list<trigger>::iterator i = m_list_trigger.begin();
         i != m_list_trigger.end(); i++ ){

        //long d= c_ppqn / 8;
        
        printf ("  tick_start[%ld] tick_end[%ld] off[%ld]\n", (*i).m_tick_start, (*i).m_tick_end, (*i).m_offset );

    }
}


void 
sequence::put_event_on_bus( event *a_e )
{		
    lock();

   unsigned char note = a_e->get_note();
    bool skip = false;
	
    if ( a_e->is_note_on() ){
		
        m_playing_notes[note]++;
    }
    if ( a_e->is_note_off() ){

        if (  m_playing_notes[note] <= 0 ){
            skip = true;
        }
        else {
            m_playing_notes[note]--;
        }		
    }

    if ( !skip ){
    //    m_masterbus->play( m_bus, a_e,  m_midi_channel );   
    }

    //m_masterbus->flush();
    
    unlock();
}


void 
sequence::off_playing_notes()
{
    lock();


    event e;
    
    for ( int x=0; x< c_midi_notes; x++ ){
	
        while( m_playing_notes[x] > 0 ){
			
            e.set_status( EVENT_NOTE_OFF );
            e.set_data( x, 0 );
			
            //m_masterbus->play( m_bus, &e, m_midi_channel ); 
			
            m_playing_notes[x]--;
        }
    }
    
    //m_masterbus->flush();
    
 
    unlock();
}










/* change */
void 
sequence::select_events( unsigned char a_status, unsigned char a_cc, bool a_inverse )
{
    lock();

    unsigned char d0, d1;
    list<event>::iterator i;

    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){

	/* initially false */
	bool set = false;	
	(*i).get_data( &d0, &d1 );
	
	/* correct status and not CC */
	if ( a_status != EVENT_CONTROL_CHANGE &&
	     (*i).get_status() == a_status )
	    set = true;
	
	/* correct status and correct cc */
	if ( a_status == EVENT_CONTROL_CHANGE &&
	     (*i).get_status() == a_status &&
	     d0 == a_cc )
	    set = true;
	
        if ( set ){

            if ( a_inverse ){
                if ( !(*i).is_selected( ) )
                    (*i).select( );
                else
                    (*i).unselect( );
                    
            }
            else 
                (*i).select( );
	}	    
    }

    unlock();
}

void
sequence::transpose_notes( int a_steps, int a_scale )
{
    event e;

    //unselect( );
    list<event> transposed_events;

    lock();

    list<event>::iterator i;

    const int *transpose_table = NULL;

    if ( a_steps < 0 ){
        transpose_table = &c_scales_transpose_dn[a_scale][0];
        a_steps *= -1;
    }
    else {
        transpose_table = &c_scales_transpose_up[a_scale][0];
    }

    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){
	
	/* is it being moved ? */
	if ( ((*i).get_status() ==  EVENT_NOTE_ON ||
              (*i).get_status() ==  EVENT_NOTE_OFF) &&
             (*i).is_selected() ){

            e = (*i);

            int  note = e.get_note();

            bool off_scale = false;
            if (  transpose_table[note % 12] == 0 ){
                off_scale = true;
                note -= 1;
            }

            for( int x=0; x<a_steps; ++x )
                note += transpose_table[note % 12];

            if ( off_scale )
                note += 1;
            
            e.set_note( note );
   
            transposed_events.push_front(e);

            (*i).select();
	}
    }

    remove_selected();
    transposed_events.sort();
    m_list_event.merge( transposed_events);
    
   
    verify_and_link();

    unlock();

    
}



// NOT DELETING THE ENDS, NOT SELECTED.
void
sequence::quanize_events( unsigned char a_status, unsigned char a_cc,
                          long a_snap_tick,  int a_divide, bool a_linked )
{
    event e,f;

    lock();

    unsigned char d0, d1;
    list<event>::iterator i;

    //unselect( );
    list<event> quantized_events;

    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){

        /* initially false */
	bool set = false;	
	(*i).get_data( &d0, &d1 );
	
	/* correct status and not CC */
	if ( a_status != EVENT_CONTROL_CHANGE &&
	     (*i).get_status() == a_status )
	    set = true;
	
	/* correct status and correct cc */
	if ( a_status == EVENT_CONTROL_CHANGE &&
	     (*i).get_status() == a_status &&
	     d0 == a_cc )
	    set = true;

        if( !(*i).is_selected() )
            set = false;
	
        if ( set ){

            /* copy event */
	    e = (*i);
            (*i).select();

            long timestamp = e.get_timestamp();
            long timestamp_remander = (timestamp % a_snap_tick);
            long timestamp_delta = 0;
            
            if ( timestamp_remander < a_snap_tick/2 ){
                timestamp_delta = - (timestamp_remander / a_divide );
            }
            else {
                timestamp_delta = (a_snap_tick - timestamp_remander) / a_divide;
            }
            
            e.set_timestamp( e.get_timestamp() + timestamp_delta );
            quantized_events.push_front(e);
            
            if ( (*i).is_linked() && a_linked ){
                
                f = *(*i).get_linked();
                (*i).get_linked()->select();
                
                f.set_timestamp( f.get_timestamp() + timestamp_delta );
                quantized_events.push_front(f);
            }
        }

    }

    remove_selected();
    quantized_events.sort();
    m_list_event.merge(quantized_events);
    verify_and_link();

    unlock();

}






void 
addListVar( list<char> *a_list, long a_var )
{
    long buffer;
    buffer = a_var & 0x7F;

    /* we shift it right 7, if there is 
       still set bits, encode into buffer
       in reverse order */
    while ( ( a_var >>= 7) ){
	buffer <<= 8;
	buffer |= ((a_var & 0x7F) | 0x80);
    }

    while (true){
	
	a_list->push_front( (char) buffer & 0xFF );

	if (buffer & 0x80)
	    buffer >>= 8;
	else
	    break;
    }
}

void
addLongList( list<char> *a_list, long a_x )
{
    a_list->push_front(  (a_x & 0xFF000000) >> 24 );
    a_list->push_front(  (a_x & 0x00FF0000) >> 16 );
    a_list->push_front(  (a_x & 0x0000FF00) >> 8  );
    a_list->push_front(  (a_x & 0x000000FF)       );
}
 

void 
sequence::fill_list( list<char> *a_list, int a_pos )
{

    lock();

    /* clear list */
    *a_list = list<char>();
    
    /* sequence number */
    addListVar( a_list, 0 );
    a_list->push_front( 0xFF );
    a_list->push_front( 0x00 );
    a_list->push_front( 0x02 );
    a_list->push_front( (a_pos & 0xFF00) >> 8 );
    a_list->push_front( (a_pos & 0x00FF)      );
            
    /* name */
    addListVar( a_list, 0 );
    a_list->push_front( 0xFF );
    a_list->push_front( 0x03 );

    int length =  m_name.length();
    if ( length > 0x7F ) length = 0x7f;
    a_list->push_front( length );

    for ( int i=0; i< length; i++ )
	a_list->push_front( m_name.c_str()[i] );	
 
    long timestamp = 0, delta_time = 0, prev_timestamp = 0;
    list<event>::iterator i;

    for ( i = m_list_event.begin(); i != m_list_event.end(); i++ ){

	event e = (*i);
	timestamp = e.get_timestamp();
	delta_time = timestamp - prev_timestamp;
	prev_timestamp = timestamp;

	/* encode delta_time */
	addListVar( a_list, delta_time );

	/* now that the timestamp is encoded, do the status and
	   data */

	a_list->push_front( e.m_status | m_midi_channel );

	switch( e.m_status & 0xF0 ){

            case 0x80:	  
            case 0x90:
            case 0xA0:
            case 0xB0:
            case 0xE0: 
	    
                a_list->push_front(  e.m_data[0] );
                a_list->push_front(  e.m_data[1] );

                //printf ( "- d[%2X %2X]\n" , e.m_data[0], e.m_data[1] ); 

                break;
	    
            case 0xC0:
            case 0xD0:
	    
                a_list->push_front(  e.m_data[0] );

                //printf ( "- d[%2X]\n" , e.m_data[0] ); 

                break;
	    
            default: 
                break;
	}
    }

    int num_triggers = m_list_trigger.size();
    list<trigger>::iterator t = m_list_trigger.begin();
    list<trigger>::iterator p;

    addListVar( a_list, 0 );
    a_list->push_front( 0xFF );
    a_list->push_front( 0x7F );
    addListVar( a_list, (num_triggers * 3 * 4) + 4);
    addLongList( a_list, c_triggers_new );

    //printf( "num_triggers[%d]\n", num_triggers );

    for ( int i=0; i<num_triggers; i++ ){

        p = t;
        //printf( "> start[%d] end[%d] offset[%d]\n",
        //        (*t).m_tick_start, (*t).m_tick_end, (*t).m_offset );
        
	addLongList( a_list, (*t).m_tick_start );
        addLongList( a_list, (*t).m_tick_end );
        addLongList( a_list, (*t).m_offset ); 
	t++;
    }

    /* bus */
    addListVar( a_list, 0 );
    a_list->push_front( 0xFF );
    a_list->push_front( 0x7F );
    a_list->push_front( 0x05 );
    addLongList( a_list, c_midibus );
    a_list->push_front( m_bus  );

    /* timesig */
    addListVar( a_list, 0 );
    a_list->push_front( 0xFF );
    a_list->push_front( 0x7F );
    a_list->push_front( 0x06 );
    addLongList( a_list, c_timesig );
    a_list->push_front( m_time_beats_per_measure  );
    a_list->push_front( m_time_beat_width  );

    /* channel */
    addListVar( a_list, 0 );
    a_list->push_front( 0xFF );
    a_list->push_front( 0x7F );
    a_list->push_front( 0x05 );
    addLongList( a_list, c_midich );
    a_list->push_front( m_midi_channel );

    delta_time = m_length - prev_timestamp;
 
    /* meta track end */
    addListVar( a_list, delta_time );
    a_list->push_front( 0xFF );
    a_list->push_front( 0x2F );
    a_list->push_front( 0x00 );

    unlock();
}








//     list<char> triggers;

//     /* triggers */

//     list<trigger>::iterator t;
//     for ( t = m_list_trigger.begin(); t != m_list_trigger.end(); t++ ){

// 	addLongList( &triggers, (*t).m_tick );
// 	printf ( "[%ld]\n", (*t).m_tick );
//     }    

//     addListVar( a_list, 0 );
//     a_list->push_front( 0xFF );
//     a_list->push_front( 0x7F );

//     a_list->push_front( 0x05 );
//     addLongList( a_list, c_triggersmidibus );


   
