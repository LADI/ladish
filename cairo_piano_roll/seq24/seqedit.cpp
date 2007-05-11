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

#include "seqedit.h" 
#include "sequence.h"
//#include "midibus.h"
//#include "controllers.h"
#include "event.h"
//#include "options.h"


#include "pixmaps/play.xpm"
#include "pixmaps/rec.xpm"
#include "pixmaps/thru.xpm"
#include "pixmaps/bus.xpm"
#include "pixmaps/midi.xpm"
#include "pixmaps/snap.xpm"
#include "pixmaps/zoom.xpm"
#include "pixmaps/length.xpm"
#include "pixmaps/scale.xpm"
#include "pixmaps/key.xpm"   
#include "pixmaps/down.xpm"
#include "pixmaps/note_length.xpm"
#include "pixmaps/undo.xpm"
#include "pixmaps/menu_empty.xpm"
#include "pixmaps/menu_full.xpm"
#include "pixmaps/sequences.xpm"
#include "pixmaps/tools.xpm"

int seqedit::m_initial_zoom = 2;
int seqedit::m_initial_snap = c_ppqn / 4;
int seqedit::m_initial_note_length = c_ppqn / 4;
int seqedit::m_initial_scale = 0;
int seqedit::m_initial_key = 0;
int seqedit::m_initial_sequence = -1;


// Actions
const int select_all_notes      = 1;
const int select_all_events     = 2;
const int select_inverse_notes  = 3;
const int select_inverse_events = 4;

const int quantize_notes    = 5;
const int quantize_events   = 6;

const int tighten_events   =  8;
const int tighten_notes    =  9;

const int transpose     = 10;
const int transpose_h   = 12;

/* connects to a menu item, tells the performance 
   to launch the timer thread */
void 
seqedit::menu_action_quantise( void )
{
}



seqedit::seqedit( sequence *a_seq, 
		  //perform *a_perf,  
		  // mainwid *a_mainwid, 
		  int a_pos  )
{

    /* set the performance */
    m_seq = a_seq;

    m_zoom        =  m_initial_zoom;
    m_snap        =  m_initial_snap;
    m_note_length = m_initial_note_length;
    m_scale       = m_initial_scale;
    m_key         =   m_initial_key;
    m_sequence    = m_initial_sequence;

    //m_mainperf = a_perf;
    // m_mainwid = a_mainwid;
    m_pos = a_pos;

    /* main window */
    set_title ( m_seq->get_name());
    set_size_request(700, 500);

    m_seq->set_editing( true );

    /* scroll bars */
    m_vadjust = manage( new Adjustment(55,0, c_num_keys,           1,1,1 ));
    m_hadjust = manage( new Adjustment(0, 0, 1,  1,1,1 ));    
    m_vscroll_new   =  manage(new VScrollbar( *m_vadjust ));
    m_hscroll_new   =  manage(new HScrollbar( *m_hadjust ));

    /* get some new objects */

    m_seqkeys_wid  = manage( new seqkeys(  m_seq,
                                           m_vadjust ));

    m_seqtime_wid  = manage( new seqtime(  m_seq,
                                           m_zoom,
                                           m_hadjust ));
    
    m_seqdata_wid  = manage( new seqdata(  m_seq,
                                           m_zoom,
                                           m_hadjust));
    
    m_seqevent_wid = manage( new seqevent( m_seq,
                                           m_zoom,
                                           m_snap,
                                           m_seqdata_wid,
                                           m_hadjust));
    
    m_seqroll_wid  = manage( new seqroll(
                                           m_seq,
                                           m_zoom,
                                           m_snap,
                                           m_seqdata_wid,
                                           m_seqevent_wid,
                                           m_seqkeys_wid,
                                           m_pos,
                                           m_hadjust,
                                           m_vadjust ));
    

    
    /* menus */
    m_menubar   =  manage( new MenuBar());
    m_menu_tools = manage( new Menu() );
    m_menu_zoom =  manage( new Menu());
    m_menu_snap =   manage( new Menu());
    m_menu_note_length = manage( new Menu());
    m_menu_length = manage( new Menu());
    m_menu_bpm = manage( new Menu() );
    m_menu_bw = manage( new Menu() );

    m_menu_midich = manage( new Menu());
    m_menu_midibus = NULL;
    m_menu_sequences = NULL;


    m_menu_key = manage( new Menu());
    m_menu_scale = manage( new Menu());
    m_menu_data = NULL;

    

    create_menus(); 

    /* tooltips */
    m_tooltips = manage( new Tooltips( ) );

 
    /* init table, viewports and scroll bars */
    m_table     = manage( new Table( 7, 4, false));
    m_vbox      = manage( new VBox( false, 2 ));
    m_hbox      = manage( new HBox( false, 2 ));
    m_hbox2     = manage( new HBox( false, 2 ));
    m_hbox3     = manage( new HBox( false, 2 ));
    HBox *dhbox = manage( new HBox( false, 2 ));

    m_vbox->set_border_width( 2 );

    /* fill table */
    m_table->attach( *m_seqkeys_wid,    0, 1, 1, 2, Gtk::SHRINK, Gtk::FILL );

    m_table->attach( *m_seqtime_wid, 1, 2, 0, 1, Gtk::FILL, Gtk::SHRINK );
    m_table->attach( *m_seqroll_wid , 1, 2, 1, 2,
    	      Gtk::FILL |  Gtk::SHRINK,  
    	      Gtk::FILL |  Gtk::SHRINK );

    m_table->attach( *m_seqevent_wid, 1, 2, 2, 3, Gtk::FILL, Gtk::SHRINK );
    m_table->attach( *m_seqdata_wid, 1, 2, 3, 4, Gtk::FILL, Gtk::SHRINK );
    m_table->attach( *dhbox,      1, 2, 4, 5, Gtk::FILL | Gtk::EXPAND, Gtk::SHRINK, 0, 2 );

    m_table->attach( *m_vscroll_new, 2, 3, 1, 2, Gtk::SHRINK, Gtk::FILL | Gtk::EXPAND  );
    m_table->attach( *m_hscroll_new, 1, 2, 5, 6, Gtk::FILL | Gtk::EXPAND, Gtk::SHRINK  );

    /* no expand, just fit the widgets */
    /* m_vbox->pack_start(*m_menubar, false, false, 0); */
    m_vbox->pack_start(*m_hbox,  false, false, 0);
    m_vbox->pack_start(*m_hbox2, false, false, 0);
    m_vbox->pack_start(*m_hbox3, false, false, 0);

    /* exapand, cause rollview expands */
    m_vbox->pack_start(*m_table, true, true, 0);




    /* data button */
    m_button_data = manage( new Button( " Event " ));
    m_button_data->signal_clicked().connect( mem_fun( *this, &seqedit::popup_event_menu));

    m_entry_data = manage( new Entry( ));
    m_entry_data->set_size_request(40,-1);
    m_entry_data->set_editable( false );

    dhbox->pack_start( *m_button_data, false, false );
    dhbox->pack_start( *m_entry_data, true, true );

    /* play, rec, thru */
    m_toggle_play = manage( new ToggleButton() );
    m_toggle_play->add(  *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( play_xpm ))));
    m_toggle_play->signal_clicked().connect( mem_fun( *this, &seqedit::play_change_callback));
    m_tooltips->set_tip( *m_toggle_play, "Sequence dumps data to midi bus." );
    
    m_toggle_record = manage( new ToggleButton(  ));
    m_toggle_record->add( *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( rec_xpm ))));
    m_toggle_record->signal_clicked().connect( mem_fun( *this, &seqedit::record_change_callback));
    m_tooltips->set_tip( *m_toggle_record, "Records incoming midi data." );

    m_toggle_thru = manage( new ToggleButton(  ));
    m_toggle_thru->add( *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( thru_xpm ))));
    m_toggle_thru->signal_clicked().connect( mem_fun( *this, &seqedit::thru_change_callback));
    m_tooltips->set_tip( *m_toggle_thru, "Incoming midi data passes thru to sequences midi bus and channel." );

    m_toggle_play->set_active( m_seq->get_playing());
    m_toggle_record->set_active( m_seq->get_recording());
    m_toggle_thru->set_active( m_seq->get_thru());
 
    dhbox->pack_end( *m_toggle_record, false, false, 4);
    dhbox->pack_end( *m_toggle_thru, false, false, 4);
    dhbox->pack_end( *m_toggle_play, false, false, 4);
    dhbox->pack_end( *(manage(new VSeparator( ))), false, false, 4);

    fill_top_bar();


    /* add table */
    this->add( *m_vbox );
    /* show everything */
    show_all();

    /* sets scroll bar to the middle */
    //gfloat middle = m_vscroll->get_adjustment()->get_upper() / 3;
    //m_vscroll->get_adjustment()->set_value(middle);

    set_zoom( m_zoom );
    set_snap( m_snap );
    set_note_length( m_note_length );
    set_background_sequence( m_sequence );


    set_bpm( m_seq->get_bpm() );
    set_bw( m_seq->get_bw() );
    set_measures( get_measures() );

    set_midi_channel( m_seq->get_midi_channel() );
    set_midi_bus( m_seq->get_midi_bus() );
    set_data_type( EVENT_NOTE_ON );

    set_scale( m_scale );
    set_key( m_key );
    set_key( m_key );

}




void 
seqedit::create_menus( void )
{
    using namespace Menu_Helpers;

    char b[20];
    
    /* zoom */
    m_menu_zoom->items().push_back(MenuElem("1:1",  SigC::bind(mem_fun(*this,&seqedit::set_zoom), 1 )));
    m_menu_zoom->items().push_back(MenuElem("1:2",  SigC::bind(mem_fun(*this,&seqedit::set_zoom), 2 )));
    m_menu_zoom->items().push_back(MenuElem("1:4",  SigC::bind(mem_fun(*this,&seqedit::set_zoom), 4 )));
    m_menu_zoom->items().push_back(MenuElem("1:8",  SigC::bind(mem_fun(*this,&seqedit::set_zoom), 8 )));
    m_menu_zoom->items().push_back(MenuElem("1:16", SigC::bind(mem_fun(*this,&seqedit::set_zoom), 16 )));
    m_menu_zoom->items().push_back(MenuElem("1:32", SigC::bind(mem_fun(*this,&seqedit::set_zoom), 32 )));
      
    /* note snap */
    m_menu_snap->items().push_back(MenuElem("1",     SigC::bind(mem_fun(*this,&seqedit::set_snap), c_ppqn * 4  )));
    m_menu_snap->items().push_back(MenuElem("1/2",   SigC::bind(mem_fun(*this,&seqedit::set_snap), c_ppqn * 2  )));
    m_menu_snap->items().push_back(MenuElem("1/4",   SigC::bind(mem_fun(*this,&seqedit::set_snap), c_ppqn * 1  )));
    m_menu_snap->items().push_back(MenuElem("1/8",   SigC::bind(mem_fun(*this,&seqedit::set_snap), c_ppqn / 2  )));
    m_menu_snap->items().push_back(MenuElem("1/16",  SigC::bind(mem_fun(*this,&seqedit::set_snap), c_ppqn / 4  )));
    m_menu_snap->items().push_back(MenuElem("1/32",  SigC::bind(mem_fun(*this,&seqedit::set_snap), c_ppqn / 8  )));
    m_menu_snap->items().push_back(MenuElem("1/64",  SigC::bind(mem_fun(*this,&seqedit::set_snap), c_ppqn / 16 )));
    m_menu_snap->items().push_back(MenuElem("1/128", SigC::bind(mem_fun(*this,&seqedit::set_snap), c_ppqn / 32 )));
    m_menu_snap->items().push_back(SeparatorElem());
    m_menu_snap->items().push_back(MenuElem("1/3",   SigC::bind(mem_fun(*this,&seqedit::set_snap), c_ppqn * 4  / 3 )));
    m_menu_snap->items().push_back(MenuElem("1/6",   SigC::bind(mem_fun(*this,&seqedit::set_snap), c_ppqn * 2  / 3 )));
    m_menu_snap->items().push_back(MenuElem("1/12",  SigC::bind(mem_fun(*this,&seqedit::set_snap), c_ppqn * 1  / 3 )));
    m_menu_snap->items().push_back(MenuElem("1/24",  SigC::bind(mem_fun(*this,&seqedit::set_snap), c_ppqn / 2  / 3 )));
    m_menu_snap->items().push_back(MenuElem("1/48",  SigC::bind(mem_fun(*this,&seqedit::set_snap), c_ppqn / 4  / 3 )));
    m_menu_snap->items().push_back(MenuElem("1/96",  SigC::bind(mem_fun(*this,&seqedit::set_snap), c_ppqn / 8  / 3 )));
    m_menu_snap->items().push_back(MenuElem("1/192", SigC::bind(mem_fun(*this,&seqedit::set_snap), c_ppqn / 16 / 3 )));
    
    /* note note_length */
    m_menu_note_length->items().push_back(MenuElem("1",     SigC::bind(mem_fun(*this,&seqedit::set_note_length), c_ppqn * 4  )));
    m_menu_note_length->items().push_back(MenuElem("1/2",   SigC::bind(mem_fun(*this,&seqedit::set_note_length), c_ppqn * 2  )));
    m_menu_note_length->items().push_back(MenuElem("1/4",   SigC::bind(mem_fun(*this,&seqedit::set_note_length), c_ppqn * 1  )));
    m_menu_note_length->items().push_back(MenuElem("1/8",   SigC::bind(mem_fun(*this,&seqedit::set_note_length), c_ppqn / 2  )));
    m_menu_note_length->items().push_back(MenuElem("1/16",  SigC::bind(mem_fun(*this,&seqedit::set_note_length), c_ppqn / 4  )));
    m_menu_note_length->items().push_back(MenuElem("1/32",  SigC::bind(mem_fun(*this,&seqedit::set_note_length), c_ppqn / 8  )));
    m_menu_note_length->items().push_back(MenuElem("1/64",  SigC::bind(mem_fun(*this,&seqedit::set_note_length), c_ppqn / 16 )));
    m_menu_note_length->items().push_back(MenuElem("1/128", SigC::bind(mem_fun(*this,&seqedit::set_note_length), c_ppqn / 32 )));
    m_menu_note_length->items().push_back(SeparatorElem());
    m_menu_note_length->items().push_back(MenuElem("1/3",   SigC::bind(mem_fun(*this,&seqedit::set_note_length), c_ppqn * 4  / 3 )));
    m_menu_note_length->items().push_back(MenuElem("1/6",   SigC::bind(mem_fun(*this,&seqedit::set_note_length), c_ppqn * 2  / 3 )));
    m_menu_note_length->items().push_back(MenuElem("1/12",  SigC::bind(mem_fun(*this,&seqedit::set_note_length), c_ppqn * 1  / 3 )));
    m_menu_note_length->items().push_back(MenuElem("1/24",  SigC::bind(mem_fun(*this,&seqedit::set_note_length), c_ppqn / 2  / 3 )));
    m_menu_note_length->items().push_back(MenuElem("1/48",  SigC::bind(mem_fun(*this,&seqedit::set_note_length), c_ppqn / 4  / 3 )));
    m_menu_note_length->items().push_back(MenuElem("1/96",  SigC::bind(mem_fun(*this,&seqedit::set_note_length), c_ppqn / 8  / 3 )));
    m_menu_note_length->items().push_back(MenuElem("1/192", SigC::bind(mem_fun(*this,&seqedit::set_note_length), c_ppqn / 16 / 3 )));
    
    /* Key */
    m_menu_key->items().push_back(MenuElem( c_key_text[0],  SigC::bind(mem_fun(*this,&seqedit::set_key), 0 )));
    m_menu_key->items().push_back(MenuElem( c_key_text[1],  SigC::bind(mem_fun(*this,&seqedit::set_key), 1 )));
    m_menu_key->items().push_back(MenuElem( c_key_text[2],  SigC::bind(mem_fun(*this,&seqedit::set_key), 2 )));
    m_menu_key->items().push_back(MenuElem( c_key_text[3],  SigC::bind(mem_fun(*this,&seqedit::set_key), 3 )));
    m_menu_key->items().push_back(MenuElem( c_key_text[4],  SigC::bind(mem_fun(*this,&seqedit::set_key), 4 )));
    m_menu_key->items().push_back(MenuElem( c_key_text[5],  SigC::bind(mem_fun(*this,&seqedit::set_key), 5 )));
    m_menu_key->items().push_back(MenuElem( c_key_text[6],  SigC::bind(mem_fun(*this,&seqedit::set_key), 6 )));
    m_menu_key->items().push_back(MenuElem( c_key_text[7],  SigC::bind(mem_fun(*this,&seqedit::set_key), 7 )));
    m_menu_key->items().push_back(MenuElem( c_key_text[8],  SigC::bind(mem_fun(*this,&seqedit::set_key), 8 )));
    m_menu_key->items().push_back(MenuElem( c_key_text[9],  SigC::bind(mem_fun(*this,&seqedit::set_key), 9 )));
    m_menu_key->items().push_back(MenuElem( c_key_text[10], SigC::bind(mem_fun(*this,&seqedit::set_key), 10 )));
    m_menu_key->items().push_back(MenuElem( c_key_text[11], SigC::bind(mem_fun(*this,&seqedit::set_key), 11 )));
    
    /* bw */
    m_menu_bw->items().push_back(MenuElem("1", SigC::bind(mem_fun(*this,&seqedit::set_bw), 1  )));
    m_menu_bw->items().push_back(MenuElem("2", SigC::bind(mem_fun(*this,&seqedit::set_bw), 2  )));
    m_menu_bw->items().push_back(MenuElem("4", SigC::bind(mem_fun(*this,&seqedit::set_bw), 4  )));
    m_menu_bw->items().push_back(MenuElem("8", SigC::bind(mem_fun(*this,&seqedit::set_bw), 8  )));
    m_menu_bw->items().push_back(MenuElem("16", SigC::bind(mem_fun(*this,&seqedit::set_bw), 16 )));
    
    
    /* music scale */
    m_menu_scale->items().push_back(MenuElem(c_scales_text[0], SigC::bind(mem_fun(*this,&seqedit::set_scale), c_scale_off )));
    m_menu_scale->items().push_back(MenuElem(c_scales_text[1], SigC::bind(mem_fun(*this,&seqedit::set_scale), c_scale_major )));
    m_menu_scale->items().push_back(MenuElem(c_scales_text[2], SigC::bind(mem_fun(*this,&seqedit::set_scale), c_scale_minor )));
    
    /* midi channel menu */
    for( int i=0; i<16; i++ ){
        
        sprintf( b, "%d", i+1 );
        
        m_menu_midich->items().push_back(MenuElem(b, 
                                                  SigC::bind(mem_fun(*this,&seqedit::set_midi_channel), 
                                                       i )));
        /* length */
        m_menu_length->items().push_back(MenuElem(b, 
                                                  SigC::bind(mem_fun(*this,&seqedit::set_measures),   
                                                       i+1 )));
        /* length */
        m_menu_bpm->items().push_back(MenuElem(b, 
                                               SigC::bind(mem_fun(*this,&seqedit::set_bpm),   
                                                    i+1 )));
    }

    m_menu_length->items().push_back(MenuElem("32", SigC::bind(mem_fun(*this,&seqedit::set_measures), 32 )));
    m_menu_length->items().push_back(MenuElem("64", SigC::bind(mem_fun(*this,&seqedit::set_measures), 64 )));





  //m_menu_tools->items().push_back( SeparatorElem( )); 
 
}



void
seqedit::popup_tool_menu( void )
{

    using namespace Menu_Helpers;

    m_menu_tools = manage( new Menu());

    /* tools */
    Menu *holder;
    Menu *holder2;
    
    holder = manage( new Menu());
    holder->items().push_back( MenuElem( "All Notes",       SigC::bind(mem_fun(*this,&seqedit::do_action), select_all_notes,0 )));
    holder->items().push_back( MenuElem( "Inverse Notes",   SigC::bind(mem_fun(*this,&seqedit::do_action), select_inverse_notes,0 )));
   
    if ( m_editing_status !=  EVENT_NOTE_ON &&
         m_editing_status !=  EVENT_NOTE_OFF ){

        holder->items().push_back( SeparatorElem( )); 
        holder->items().push_back( MenuElem( "All Events",      SigC::bind(mem_fun(*this,&seqedit::do_action), select_all_events,0 )));
        holder->items().push_back( MenuElem( "Inverse Events",  SigC::bind(mem_fun(*this,&seqedit::do_action), select_inverse_events,0 )));
    }
    
    m_menu_tools->items().push_back( MenuElem( "Select", *holder ));
    
    holder = manage( new Menu());
    holder->items().push_back( MenuElem( "Quantize Selected Notes",       SigC::bind(mem_fun(*this,&seqedit::do_action), quantize_notes,0 )));
    holder->items().push_back( MenuElem( "Tighten Selected Notes",       SigC::bind(mem_fun(*this,&seqedit::do_action), tighten_notes,0 )));

    if ( m_editing_status !=  EVENT_NOTE_ON &&
         m_editing_status !=  EVENT_NOTE_OFF ){
        
        holder->items().push_back( SeparatorElem( )); 
        holder->items().push_back( MenuElem( "Quantize Selected Events",      SigC::bind(mem_fun(*this,&seqedit::do_action), quantize_events,0 )));
        holder->items().push_back( MenuElem( "Tighten Selected Events",      SigC::bind(mem_fun(*this,&seqedit::do_action), tighten_events,0 )));
        
    }
    m_menu_tools->items().push_back( MenuElem( "Modify Time", *holder ));


    
    holder = manage( new Menu());

    char num[6];

    holder2 = manage( new Menu());
    for ( int i=-12; i<=12; ++i ){

        if ( i!=0 ){
            sprintf( num, "%+d [%s]", i, c_interval_text[ abs(i) ]  );
            holder2->items().push_front( MenuElem( num, SigC::bind(mem_fun(*this,&seqedit::do_action), transpose, i )));
        }
    }
    
    holder->items().push_back( MenuElem( "Transpose Selected",*holder2));


    
    holder2 = manage( new Menu());
    for ( int i=-7; i<=7; ++i ){

        if ( i!=0 ){
            sprintf( num, "%+d [%s]",  (i<0) ? i-1 : i+1, c_chord_text[ abs(i) ]  );
            holder2->items().push_front( MenuElem( num, SigC::bind(mem_fun(*this,&seqedit::do_action), transpose_h, i )));
        }
    }

    if ( m_scale != 0 ){
        holder->items().push_back( MenuElem( "Harmonic Transpose Selected",*holder2));
    }
    
    m_menu_tools->items().push_back( MenuElem( "Modify Pitch", *holder ));


    m_menu_tools->popup(0,0);

}

void
seqedit::do_action( int a_action, int a_var )
{
    switch( a_action ){

        case select_all_notes:
        {
            m_seq->select_events( EVENT_NOTE_ON, 0 );
            m_seq->select_events( EVENT_NOTE_OFF, 0 );
            
        } break;
        
        case select_all_events:
        {
            m_seq->select_events(  m_editing_status,  m_editing_cc );
            
        } break;
        
        case select_inverse_notes:
        {
            m_seq->select_events( EVENT_NOTE_ON, 0, true );
            m_seq->select_events( EVENT_NOTE_OFF, 0, true );

        } break;
        
        case select_inverse_events:
        {
            m_seq->select_events(  m_editing_status,  m_editing_cc, true );
            
        } break;

            // !!! m_seq->push_undo( );

        
            
        case quantize_notes:
        {
            m_seq->push_undo( );
            m_seq->quanize_events( EVENT_NOTE_ON, 0, m_snap, 1 , true );
            
        } break;
        
        case quantize_events:
        {
            m_seq->push_undo( );
            m_seq->quanize_events( m_editing_status,  m_editing_cc, m_snap, 1 );
            
        } break;

        

        case tighten_notes:
        {
            m_seq->push_undo( );
            m_seq->quanize_events( EVENT_NOTE_ON, 0, m_snap, 2 , true );
            
        } break;
        
        case tighten_events:
        {
            m_seq->push_undo( );
            m_seq->quanize_events( m_editing_status,  m_editing_cc, m_snap, 2 );
            
        } break;
     
        case transpose:
        {
            m_seq->push_undo( );
            m_seq->transpose_notes(a_var, 0 ); 
        } break; 
    
        
        case transpose_h:
        {
            m_seq->push_undo( );
            m_seq->transpose_notes(a_var, m_scale ); 
        } break;
        
        default: break;
    }

    m_seqroll_wid->redraw();
    m_seqtime_wid->redraw();
    m_seqdata_wid->redraw();  
    m_seqevent_wid->redraw();

}


void  
seqedit::fill_top_bar( void )
{

     /* name */

    m_entry_name = manage( new Entry(  ));
    m_entry_name->set_max_length(26);
    m_entry_name->set_width_chars(26);
    m_entry_name->set_text( m_seq->get_name());
    m_entry_name->select_region(0,0);
    m_entry_name->set_position(0);
    m_entry_name->signal_changed().connect( mem_fun( *this, &seqedit::name_change_callback));

    m_hbox->pack_start( *m_entry_name, true, true );

    
   
    m_hbox->pack_start( *(manage(new VSeparator( ))), false, false, 4);

              

    /* beats per measure */ 
    m_button_bpm = manage( new Button());
    m_button_bpm->add( *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( down_xpm  ))));
    m_button_bpm->signal_clicked().connect(  SigC::bind<Menu *>( mem_fun( *this, &seqedit::popup_menu), m_menu_bpm  ));
    m_tooltips->set_tip( *m_button_bpm, "Time Signature. Beats per Measure" );
    m_entry_bpm = manage( new Entry());
    m_entry_bpm->set_width_chars(2);
    m_entry_bpm->set_editable( false );

    m_hbox->pack_start( *m_button_bpm , false, false );
    m_hbox->pack_start( *m_entry_bpm , false, false );
 
    m_hbox->pack_start( *(manage(new Label( "/" ))), false, false, 4);

    /* beat width */
    m_button_bw = manage( new Button());
    m_button_bw->add( *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( down_xpm  ))));
    m_button_bw->signal_clicked().connect(  SigC::bind<Menu *>( mem_fun( *this, &seqedit::popup_menu), m_menu_bw  ));
    m_tooltips->set_tip( *m_button_bw, "Time Signature.  Length of Beat" );
    m_entry_bw = manage( new Entry());
    m_entry_bw->set_width_chars(2);
    m_entry_bw->set_editable( false );

    m_hbox->pack_start( *m_button_bw , false, false );
    m_hbox->pack_start( *m_entry_bw , false, false );


    /* length */
    m_button_length = manage( new Button());
    m_button_length->add( *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( length_xpm  ))));
    m_button_length->signal_clicked().connect(  SigC::bind<Menu *>( mem_fun( *this, &seqedit::popup_menu), m_menu_length  ));
    m_tooltips->set_tip( *m_button_length, "Sequence length in Bars." );
    m_entry_length = manage( new Entry());
    m_entry_length->set_width_chars(2);
    m_entry_length->set_editable( false );

    m_hbox->pack_start( *m_button_length , false, false );
    m_hbox->pack_start( *m_entry_length , false, false );


    m_hbox->pack_start( *(manage(new VSeparator( ))), false, false, 4);
    
    /* midi bus */
    m_button_bus = manage( new Button());
    m_button_bus->add( *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( bus_xpm  ))));
    m_button_bus->signal_clicked().connect( mem_fun( *this, &seqedit::popup_midibus_menu));
    m_tooltips->set_tip( *m_button_bus, "Select Output Bus." );

    m_entry_bus = manage( new Entry());
    m_entry_bus->set_max_length(26);
    m_entry_bus->set_width_chars(26);
    m_entry_bus->set_editable( false );

    m_hbox->pack_start( *m_button_bus , false, false );
    m_hbox->pack_start( *m_entry_bus , true, true );

    /* midi channel */
    m_button_channel = manage( new Button());
    m_button_channel->add( *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( midi_xpm  ))));
    m_button_channel->signal_clicked().connect(  SigC::bind<Menu *>( mem_fun( *this, &seqedit::popup_menu), m_menu_midich  ));
    m_tooltips->set_tip( *m_button_channel, "Select Midi channel." );
    m_entry_channel = manage( new Entry());
    m_entry_channel->set_width_chars(2);
    m_entry_channel->set_editable( false );

    m_hbox->pack_start( *m_button_channel , false, false );
    m_hbox->pack_start( *m_entry_channel , false, false );





    /* undo */
    m_button_undo = manage( new Button());
    m_button_undo->add( *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( undo_xpm  ))));
    m_button_undo->signal_clicked().connect(  mem_fun( *this, &seqedit::undo_callback));
    m_tooltips->set_tip( *m_button_undo, "Undo." );
 
    m_hbox2->pack_start( *m_button_undo , false, false );
    m_hbox2->pack_start( *(manage(new VSeparator( ))), false, false, 4);

    /* tools button */
    m_button_tools = manage( new Button( ));
    m_button_tools->add( *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( tools_xpm  ))));
    m_button_tools->signal_clicked().connect( mem_fun( *this, &seqedit::popup_tool_menu ));
    m_tooltips->set_tip(  *m_button_tools, "Tools." );

    m_hbox2->pack_start( *m_button_tools , false, false );
    m_hbox2->pack_start( *(manage(new VSeparator( ))), false, false, 4);



    
    /* snap */
    m_button_snap = manage( new Button());
    m_button_snap->add( *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( snap_xpm  ))));
    m_button_snap->signal_clicked().connect(  SigC::bind<Menu *>( mem_fun( *this, &seqedit::popup_menu), m_menu_snap  ));
    m_tooltips->set_tip( *m_button_snap, "Grid snap." );
    m_entry_snap = manage( new Entry());
    m_entry_snap->set_width_chars(5);
    m_entry_snap->set_editable( false );
    
    m_hbox2->pack_start( *m_button_snap , false, false );
    m_hbox2->pack_start( *m_entry_snap , false, false );


    /* note_length */
    m_button_note_length = manage( new Button());
    m_button_note_length->add( *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( note_length_xpm  ))));
    m_button_note_length->signal_clicked().connect(  SigC::bind<Menu *>( mem_fun( *this, &seqedit::popup_menu), m_menu_note_length  ));
    m_tooltips->set_tip( *m_button_note_length, "Note Length." );
    m_entry_note_length = manage( new Entry());
    m_entry_note_length->set_width_chars(5);
    m_entry_note_length->set_editable( false );
    
    m_hbox2->pack_start( *m_button_note_length , false, false );
    m_hbox2->pack_start( *m_entry_note_length , false, false );


    /* zoom */
    m_button_zoom = manage( new Button());
    m_button_zoom->add( *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( zoom_xpm  ))));
    m_button_zoom->signal_clicked().connect(  SigC::bind<Menu *>( mem_fun( *this, &seqedit::popup_menu), m_menu_zoom  ));
    m_tooltips->set_tip( *m_button_zoom, "Zoom. Pixels to Ticks" );
    m_entry_zoom = manage( new Entry());
    m_entry_zoom->set_width_chars(4);
    m_entry_zoom->set_editable( false );

    m_hbox2->pack_start( *m_button_zoom , false, false );
    m_hbox2->pack_start( *m_entry_zoom , false, false );

  
    m_hbox2->pack_start( *(manage(new VSeparator( ))), false, false, 4);

    /* key */
    m_button_key = manage( new Button());
    m_button_key->add( *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( key_xpm  ))));
    m_button_key->signal_clicked().connect(  SigC::bind<Menu *>( mem_fun( *this, &seqedit::popup_menu), m_menu_key  ));
    m_tooltips->set_tip( *m_button_key, "Key of Sequence" );
    m_entry_key = manage( new Entry());
    m_entry_key->set_width_chars(5);
    m_entry_key->set_editable( false );
    
    m_hbox2->pack_start( *m_button_key , false, false );
    m_hbox2->pack_start( *m_entry_key , false, false );

    /* music scale */
    m_button_scale = manage( new Button());
    m_button_scale->add( *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( scale_xpm  ))));
    m_button_scale->signal_clicked().connect(  SigC::bind<Menu *>( mem_fun( *this, &seqedit::popup_menu), m_menu_scale  ));
    m_tooltips->set_tip( *m_button_scale, "Musical Scale" );
    m_entry_scale = manage( new Entry());
    m_entry_scale->set_width_chars(5);
    m_entry_scale->set_editable( false );

    m_hbox2->pack_start( *m_button_scale , false, false );
    m_hbox2->pack_start( *m_entry_scale , false, false );

    m_hbox2->pack_start( *(manage(new VSeparator( ))), false, false, 4);

    /* background sequence */
    m_button_sequence = manage( new Button());
    m_button_sequence->add( *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( sequences_xpm  ))));
    m_button_sequence->signal_clicked().connect(  mem_fun( *this, &seqedit::popup_sequence_menu));
    m_tooltips->set_tip( *m_button_sequence, "Background Sequence" );
    m_entry_sequence = manage( new Entry());
    m_entry_sequence->set_width_chars(14);
    m_entry_sequence->set_editable( false );

    m_hbox2->pack_start( *m_button_sequence , false, false );
    m_hbox2->pack_start( *m_entry_sequence , true, true );



#if 0
    /* Select */
    m_radio_select = manage( new RadioButton( "Sel", true ));
    m_radio_select->signal_clicked().connect( SigC::bind(mem_fun( *this, &seqedit::mouse_action ), e_action_select ));
    m_hbox3->pack_start( *m_radio_select, false, false );
    
    /* Draw */
    m_radio_draw = manage( new RadioButton( "Draw" ));
    m_radio_draw->signal_clicked().connect( SigC::bind(mem_fun( *this, &seqedit::mouse_action ), e_action_draw ));
    m_hbox3->pack_start( *m_radio_draw, false, false );
     
    /* Grow */
    m_radio_grow = manage( new RadioButton( "Grow" ));
    m_radio_grow->signal_clicked().connect( SigC::bind(mem_fun( *this, &seqedit::mouse_action ), e_action_grow ));
    m_hbox3->pack_start( *m_radio_grow, false, false );
     
    /* Stretch */

    Gtk::RadioButton::Group g = m_radio_select->get_group();
    m_radio_draw->set_group(g);
    m_radio_grow->set_group(g);
#endif 
}

#if 0
void 
seqedit::mouse_action( mouse_action_e a_action )
{
    if ( a_action == e_action_select && m_radio_select->get_active())
        printf( "mouse_action() select [%d]\n", a_action );
    
    if ( a_action == e_action_draw && m_radio_draw->get_active())
        printf( "mouse_action() draw [%d]\n", a_action );

    if ( a_action == e_action_grow && m_radio_grow->get_active())
        printf( "mouse_action() grow [%d]\n", a_action );
}
#endif

void
seqedit::popup_menu(Menu *a_menu)
{
    a_menu->popup(0,0);
}


void
seqedit::popup_midibus_menu( void )
{
    using namespace Menu_Helpers;

    m_menu_midibus = manage( new Menu());

    /* midi buses */
    /*mastermidibus *masterbus = m_mainperf->get_master_midi_bus();
    for ( int i=0; i< masterbus->get_num_out_buses(); i++ ){
        m_menu_midibus->items().push_back(MenuElem(masterbus->get_midi_out_bus_name(i),
                                                   SigC::bind(mem_fun(*this,&seqedit::set_midi_bus), i)));
    }
*/
    m_menu_midibus->popup(0,0);


}



void
seqedit::popup_sequence_menu( void )
{
    using namespace Menu_Helpers;


    m_menu_sequences = manage( new Menu());

    m_menu_sequences->items().push_back(MenuElem("Off",
                                                 SigC::bind(mem_fun(*this, &seqedit::set_background_sequence), -1)));
    m_menu_sequences->items().push_back( SeparatorElem( ));

    for ( int ss=0; ss<c_max_sets; ++ss ){

        Menu *menu_ss = NULL;  
        bool inserted = false;
        
        for ( int seq=0; seq<  c_seqs_in_set; seq++ ){

            int i = ss * c_seqs_in_set + seq;

            char name[30];
            
            /*if ( m_mainperf->is_active( i )){

                if ( !inserted ){
                    inserted = true;
                    sprintf( name, "[%d]", ss );
                    menu_ss = manage( new Menu());
                    m_menu_sequences->items().push_back(MenuElem(name,*menu_ss));
                    
                }
                
                sequence *seq = m_mainperf->get_sequence( i );                
                sprintf( name, "[%d] %.13s", i, seq->get_name() );

                menu_ss->items().push_back(MenuElem(name,
                                                    SigC::bind(mem_fun(*this,&seqedit::set_background_sequence), i)));
                
            }*/
        }
    }
    
    m_menu_sequences->popup(0,0);
}

void
seqedit::set_background_sequence( int a_seq )
{
    char name[30];

    m_initial_sequence = m_sequence = a_seq;
/*
    if ( a_seq == -1 || !m_mainperf->is_active( a_seq )){
        m_entry_sequence->set_text("Off");
         m_seqroll_wid->set_background_sequence( false, 0 );
    }
    
    if ( m_mainperf->is_active( a_seq )){

        sequence *seq = m_mainperf->get_sequence( a_seq );                
        sprintf( name, "[%d] %.13s", a_seq, seq->get_name() );
        m_entry_sequence->set_text(name);

        m_seqroll_wid->set_background_sequence( true, a_seq );

    }
	*/
}

Gtk::Image*
seqedit::create_menu_image( bool a_state )
{
    if ( a_state )
        return manage( new Image(Gdk::Pixbuf::create_from_xpm_data( menu_full_xpm  )));
    else
        return manage( new Image(Gdk::Pixbuf::create_from_xpm_data( menu_empty_xpm  )));
}


void
seqedit::popup_event_menu( void )
{
    using namespace Menu_Helpers;

    /* temp */
    char b[20];

    bool note_on = false;
    bool note_off = false;
    bool aftertouch = false;
    bool program_change = false;
    bool channel_pressure = false;
    bool pitch_wheel = false;
    bool ccs[128]; memset( ccs, false, sizeof(bool) * 128 );

    unsigned char status, cc;
    m_seq->reset_draw_marker();
    while ( m_seq->get_next_event( &status, &cc ) == true ){

        switch( status ){
            
        	case EVENT_NOTE_OFF: note_off = true; break;	  
            case EVENT_NOTE_ON: note_on = true; break;
            case EVENT_AFTERTOUCH: aftertouch = true; break;
            case EVENT_CONTROL_CHANGE: ccs[cc] = true; break;
            case EVENT_PITCH_WHEEL: pitch_wheel = true; break;
		    
                /* one data item */
            case EVENT_PROGRAM_CHANGE: program_change = true; break;
            case EVENT_CHANNEL_PRESSURE: channel_pressure = true; break;
        }
    }

    m_menu_data = manage( new Menu());

    m_menu_data->items().push_back( ImageMenuElem( "Note On Velocity",
                                                   *create_menu_image( note_on ),
                                                   SigC::bind(mem_fun(*this,&seqedit::set_data_type), (unsigned char) EVENT_NOTE_ON, 0 )));

    m_menu_data->items().push_back( SeparatorElem( )); 

    m_menu_data->items().push_back( ImageMenuElem( "Note Off Velocity",
                                                   *create_menu_image( note_off ),
                                                   SigC::bind(mem_fun(*this,&seqedit::set_data_type), (unsigned char) EVENT_NOTE_OFF, 0 )));

    m_menu_data->items().push_back( ImageMenuElem( "AfterTouch",
                                                   *create_menu_image( aftertouch ),
                                                   SigC::bind(mem_fun(*this,&seqedit::set_data_type), (unsigned char) EVENT_AFTERTOUCH, 0 )));

    m_menu_data->items().push_back( ImageMenuElem( "Program Change",
                                                   *create_menu_image( program_change ),
                                                   SigC::bind(mem_fun(*this,&seqedit::set_data_type), (unsigned char) EVENT_PROGRAM_CHANGE, 0 )));

    m_menu_data->items().push_back( ImageMenuElem( "Channel Pressure",
                                                   *create_menu_image( channel_pressure ),
                                                   SigC::bind(mem_fun(*this,&seqedit::set_data_type), (unsigned char) EVENT_CHANNEL_PRESSURE, 0 )));

    m_menu_data->items().push_back( ImageMenuElem( "Pitch Wheel",
                                                   *create_menu_image( pitch_wheel ),
                                                   SigC::bind(mem_fun(*this,&seqedit::set_data_type), (unsigned char) EVENT_PITCH_WHEEL , 0 )));

    m_menu_data->items().push_back( SeparatorElem( )); 

    /* create control change */
    for ( int i=0; i<8; i++ ){
        
        sprintf( b, "Controls %d-%d", (i*16), (i*16)+15 );
        Menu *menu_cc = manage( new Menu() ); 
        
        for( int j=0; j<16; j++ ){
			/*
            menu_cc->items().push_back( ImageMenuElem( c_controller_names[i*16+j],
                                                       *create_menu_image( ccs[i*16+j]),
                                                       SigC::bind(mem_fun(*this,&seqedit::set_data_type), 
                                                       (unsigned char) EVENT_CONTROL_CHANGE, i*16+j)));
													   */
        }
        m_menu_data->items().push_back( MenuElem( string(b), *menu_cc ));
    }

    m_menu_data->popup(0,0);
}


    //m_option_midich->set_history( m_seq->getMidiChannel() );
    //m_option_midibus->set_history( m_seq->getMidiBus()->getID() );

void 
seqedit::set_midi_channel( int a_midichannel  )
{
    char b[10];
    sprintf( b, "%d", a_midichannel+1 );
    m_entry_channel->set_text(b);
    m_seq->set_midi_channel( a_midichannel );
    // m_mainwid->update_sequence_on_window( m_pos );
}


void 
seqedit::set_midi_bus( int a_midibus )
{
    m_seq->set_midi_bus( a_midibus );
	//mastermidibus *mmb =  m_mainperf->get_master_midi_bus();
    //m_entry_bus->set_text( mmb->get_midi_out_bus_name( a_midibus ));
    // m_mainwid->update_sequence_on_window( m_pos );
}


void 
seqedit::set_zoom( int a_zoom  )
{
    char b[10];
    sprintf( b, "1:%d", a_zoom );
    m_entry_zoom->set_text(b);

    m_zoom = a_zoom;
    m_initial_zoom = a_zoom;
    m_seqroll_wid->set_zoom( m_zoom );
    m_seqtime_wid->set_zoom( m_zoom );
    m_seqdata_wid->set_zoom( m_zoom ); 
    m_seqevent_wid->set_zoom( m_zoom );
}


void 
seqedit::set_snap( int a_snap  )
{
    char b[10];
    sprintf( b, "1/%d",   c_ppqn * 4 / a_snap );
    m_entry_snap->set_text(b);
    
    m_snap = a_snap;
    m_initial_snap = a_snap;
    m_seqroll_wid->set_snap( m_snap );
    m_seqevent_wid->set_snap( m_snap );
}



void 
seqedit::set_note_length( int a_note_length  )
{
    char b[10];
    sprintf( b, "1/%d",   c_ppqn * 4 / a_note_length );
    m_entry_note_length->set_text(b);
    
    m_note_length = a_note_length;
    m_initial_note_length = a_note_length;
    m_seqroll_wid->set_note_length( m_note_length );
}




void 
seqedit::set_scale( int a_scale )
{
  m_entry_scale->set_text( c_scales_text[a_scale] );

  m_scale = m_initial_scale = a_scale;

  m_seqroll_wid->set_scale( m_scale );
  m_seqkeys_wid->set_scale( m_scale );


}

void 
seqedit::set_key( int a_note )
{ 
  m_entry_key->set_text( c_key_text[a_note] );

  m_key = m_initial_key = a_note;
  
  m_seqroll_wid->set_key( m_key );
  m_seqkeys_wid->set_key( m_key );

}
 
void
seqedit::apply_length( int a_bpm, int a_bw, int a_measures )
{
  m_seq->set_length( a_measures * a_bpm * ((c_ppqn * 4) / a_bw) );

  m_seqroll_wid->reset();
  m_seqtime_wid->reset();
  m_seqdata_wid->reset();  
  m_seqevent_wid->reset();
    
}

long
seqedit::get_measures( void )
{
  long measures = ( m_seq->get_length() / ((m_seq->get_bpm() * (c_ppqn * 4)) /  m_seq->get_bw() ));
 
  return measures;
}

void 
seqedit::set_measures( int a_length_measures  )
{

    char b[10];
    sprintf( b, "%d", a_length_measures );
    m_entry_length->set_text(b);

    m_measures = a_length_measures;
    apply_length( m_seq->get_bpm(), m_seq->get_bw(), a_length_measures );
    
}



void 
seqedit::set_bpm( int a_beats_per_measure )
{
  char b[4];
  sprintf( b, "%d", a_beats_per_measure );
  m_entry_bpm->set_text(b);

  if ( a_beats_per_measure != m_seq->get_bpm() ){
    
    long length = get_measures();
    m_seq->set_bpm( a_beats_per_measure );
    apply_length( a_beats_per_measure, m_seq->get_bw(), length );
  }
}



void 
seqedit::set_bw( int a_beat_width  )
{
  char b[4];
  sprintf( b, "%d",   a_beat_width );
  m_entry_bw->set_text(b);
  
  if ( a_beat_width != m_seq->get_bw()){
    
    long length = get_measures();
    m_seq->set_bw( a_beat_width );
    apply_length( m_seq->get_bpm(), a_beat_width, length );
  }
}




void 
seqedit::name_change_callback( void )
{
    m_seq->set_name( m_entry_name->get_text());
    // m_mainwid->update_sequence_on_window( m_pos );
}


void 
seqedit::play_change_callback( void )
{
    m_seq->set_playing( m_toggle_play->get_active() );
    // m_mainwid->update_sequence_on_window( m_pos );
}


void 
seqedit::record_change_callback( void )
{
    //m_mainperf->get_master_midi_bus()->set_sequence_input( true, m_seq );
    m_seq->set_recording( m_toggle_record->get_active() );
}


void 
seqedit::undo_callback( void )
{
	m_seq->pop_undo( );
 	
	m_seqroll_wid->redraw();
	m_seqtime_wid->redraw();
	m_seqdata_wid->redraw();  
	m_seqevent_wid->redraw();
}


void 
seqedit::thru_change_callback( void )
{
    //m_mainperf->get_master_midi_bus()->set_sequence_input( true, m_seq );
    m_seq->set_thru( m_toggle_thru->get_active() );
}

void 
seqedit::set_data_type( unsigned char a_status, unsigned char a_control  )
{
    m_editing_status = a_status;
    m_editing_cc = a_control;

    m_seqevent_wid->set_data_type( a_status, a_control );
    m_seqdata_wid->set_data_type( a_status, a_control );
	m_seqroll_wid->set_data_type( a_status, a_control );

    char text[100];
    char hex[20];
    char type[80];

    sprintf( hex, "[0x%02X]", a_status );  
    
    if ( a_status ==  EVENT_NOTE_OFF )         
	sprintf( type, "Note Off" );
    if ( a_status ==  EVENT_NOTE_ON )          
	sprintf( type, "Note On" );  
    if ( a_status ==  EVENT_AFTERTOUCH )       
	sprintf( type, "Aftertouch" );           
    if ( a_status ==  EVENT_CONTROL_CHANGE )   
	sprintf( type, "Control Change - %d", a_control );  
    if ( a_status ==    EVENT_PROGRAM_CHANGE )   
	sprintf( type, "Program Change" );       
    if ( a_status ==    EVENT_CHANNEL_PRESSURE ) 
	sprintf( type, "Channel Pressure" );     
    if ( a_status ==  EVENT_PITCH_WHEEL )      
	sprintf( type, "Pitch Wheel" );          

    sprintf( text, "%s %s", hex, type );

    m_entry_data->set_text( text );

}


void 
seqedit::on_realize()
{
    // we need to do the default realize
    Gtk::Window::on_realize();

    Glib::signal_timeout().connect(mem_fun(*this,&seqedit::timeout ), c_redraw_ms);
 
}

bool
seqedit::timeout( void )
{
    if (m_seq->is_dirty_edit() ){
		m_seqroll_wid->idle_redraw();
		m_seqevent_wid->idle_redraw();
		m_seqdata_wid->idle_redraw();
    }

    m_seqroll_wid->draw_progress_on_window();
    
    return true;
}

seqedit::~seqedit()
{
    //m_seq->set_editing( false );
}



bool
seqedit::on_delete_event(GdkEventAny *a_event)
{
    //printf( "seqedit::on_delete_event()\n" );
    m_seq->set_recording( false );
    //m_mainperf->get_master_midi_bus()->set_sequence_input( false, NULL );
    m_seq->set_editing( false );

    delete this;
    
    return false;
}
