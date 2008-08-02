/* This file is part of Patchage.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
 * 
 * Patchage is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Patchage is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <string.h>
#include <cmath>
#include <sstream>
#include <fstream>
#include <libgnomecanvasmm.h>
#include <libglademm/xml.h>
#include <gtk/gtkwindow.h>
#include <boost/format.hpp>

#include CONFIG_H_PATH
#include "common.hpp"
#include "jack_proxy.hpp"
#include "JackSettingsDialog.hpp"
#include "Patchage.hpp"
#include "PatchageCanvas.hpp"
#include "StateManager.hpp"
#include "lash_proxy.hpp"
#include "project_list.hpp"
#include "session.hpp"
#include "globals.hpp"
#include "a2j_proxy.hpp"
#include "PatchageModule.hpp"

Patchage * g_app;

//#define LOG_TO_STD
#define LOG_TO_STATUS

#define DBUS_CALL_DEFAULT_TIMEOUT 1000 // in milliseconds

using namespace std;

/* Gtk helpers (resize combo boxes) */

static void
gtkmm_get_ink_pixel_size (Glib::RefPtr<Pango::Layout> layout,
			       int& width,
			       int& height)
{
	Pango::Rectangle ink_rect = layout->get_ink_extents ();
	
	width = (ink_rect.get_width() + PANGO_SCALE / 2) / PANGO_SCALE;
	height = (ink_rect.get_height() + PANGO_SCALE / 2) / PANGO_SCALE;
}

static void
gtkmm_set_width_for_given_text (Gtk::Widget &w, const gchar *text,
						   gint hpadding/*, gint vpadding*/)
	
{
	int old_width, old_height;
	w.get_size_request(old_width, old_height);

	int width, height;
	w.ensure_style ();
	
	gtkmm_get_ink_pixel_size (w.create_pango_layout (text), width, height);
	w.set_size_request(width + hpadding, old_height);//height + vpadding);
}

/* end Gtk helpers */


#define INIT_WIDGET(x) x(g_xml, ((const char*)#x) + 1)

Patchage::Patchage(int argc, char** argv)
	: _dbus_connection(0)
	, INIT_WIDGET(_menu_open_session)
	, INIT_WIDGET(_menu_save_session)
	, INIT_WIDGET(_menu_save_session_as)
	, INIT_WIDGET(_menu_close_session)
	, _state_manager(NULL)
	, _max_dsp_load(0.0)
	, INIT_WIDGET(_about_win)
	, INIT_WIDGET(_buffer_size_combo)
	, INIT_WIDGET(_clear_load_but)
	, INIT_WIDGET(_main_scrolledwin)
	, INIT_WIDGET(_main_win)
	, INIT_WIDGET(_main_xrun_progress)
	, INIT_WIDGET(_menu_file_quit)
	, INIT_WIDGET(_menu_help_about)
	, INIT_WIDGET(_menu_jack_start)
	, INIT_WIDGET(_menu_jack_stop)
	, INIT_WIDGET(_menu_store_positions)
	, INIT_WIDGET(_menu_view_arrange)
	, INIT_WIDGET(_menu_view_messages)
	, INIT_WIDGET(_menu_view_refresh)
	, INIT_WIDGET(_menu_view_toolbar)
	, INIT_WIDGET(_messages_win)
	, INIT_WIDGET(_messages_clear_but)
	, INIT_WIDGET(_messages_close_but)
	, INIT_WIDGET(_sample_rate_label)
	, INIT_WIDGET(_status_text)
	, INIT_WIDGET(_toolbar)
	, INIT_WIDGET(_zoom_full_but)
	, INIT_WIDGET(_zoom_normal_but)
{
	g_app = this;

	_settings_filename = getenv("HOME");
	_settings_filename += "/.patchagerc";
	_state_manager = new StateManager();
	_canvas = boost::shared_ptr<PatchageCanvas>(new PatchageCanvas(this, 1600*2, 1200*2));

	while (argc > 0) {
		if (!strcmp(*argv, "--help")) {
			cout << "Usage: patchage [OPTIONS]\nOptions: --no-alsa" << endl;
			exit(0);
		}

		argv++;
		argc--;
	}

	dbus_error_init(&_dbus_error);

	// Connect to the bus
	_dbus_connection = dbus_bus_get(DBUS_BUS_SESSION, &_dbus_error);
	if (dbus_error_is_set(&_dbus_error)) {
		error_msg("dbus_bus_get() failed");
		error_msg(_dbus_error.message);
		dbus_error_free(&_dbus_error);
		return;
	}

	dbus_connection_setup_with_g_main(_dbus_connection, NULL);

	Glib::set_application_name("Patchage");
	_about_win->property_program_name() = "Patchage";
	_about_win->property_logo_icon_name() = "patchage";
	gtk_window_set_default_icon_name("patchage");
	
	gtkmm_set_width_for_given_text(*_buffer_size_combo, "4096 frames", 40);

	_main_scrolledwin->add(*_canvas);
	_canvas->scroll_to(static_cast<int>(_canvas->width()/2 - 320),
	                   static_cast<int>(_canvas->height()/2 - 240)); // FIXME: hardcoded

	_main_scrolledwin->property_hadjustment().get_value()->set_step_increment(10);
	_main_scrolledwin->property_vadjustment().get_value()->set_step_increment(10);

	_main_scrolledwin->signal_scroll_event().connect(
			sigc::mem_fun(this, &Patchage::on_scroll));

	_buffer_size_combo->signal_changed().connect(
			sigc::mem_fun(this, &Patchage::buffer_size_changed));
	_clear_load_but->signal_clicked().connect(
			sigc::mem_fun(this, &Patchage::clear_load));
	_zoom_normal_but->signal_clicked().connect(sigc::bind(
			sigc::mem_fun(this, &Patchage::zoom), 1.0));
	_zoom_full_but->signal_clicked().connect(
			sigc::mem_fun(_canvas.get(), &PatchageCanvas::zoom_full));

	_menu_open_session->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::load_project_ask));
	_menu_save_session->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::save_all_projects));
	_menu_close_session->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::close_all_projects));

	_menu_store_positions->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::on_store_positions));
	_menu_file_quit->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::on_quit));
	_menu_view_refresh->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::refresh));
	_menu_view_arrange->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::on_arrange));
	_menu_view_toolbar->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::on_view_toolbar));
	_menu_view_messages->signal_toggled().connect(
			sigc::mem_fun(this, &Patchage::on_show_messages));
	_menu_help_about->signal_activate().connect(
			sigc::mem_fun(this, &Patchage::on_help_about));
	
	_messages_clear_but->signal_clicked().connect(
			sigc::mem_fun(this, &Patchage::on_messages_clear));
	_messages_close_but->signal_clicked().connect(
			sigc::mem_fun(this, &Patchage::on_messages_close));
	_messages_win->signal_delete_event().connect(
			sigc::mem_fun(this, &Patchage::on_messages_delete));

	_canvas->show();
	_main_win->present();
	
	_state_manager->load(_settings_filename);
	
	_main_win->resize(
		static_cast<int>(_state_manager->get_window_size().x),
		static_cast<int>(_state_manager->get_window_size().y));

	_main_win->move(
		static_cast<int>(_state_manager->get_window_location().x),
		static_cast<int>(_state_manager->get_window_location().y));
	
	_about_win->set_transient_for(*_main_win);

	_a2j = new a2j_proxy;

	//info_msg(str(boost::format("a2j jack client name is '%s'") % _a2j->get_jack_client_name()));

	_session = new session();

	_project_list = new project_list(this, _session);

	_lash = new lash_proxy(_session);

	_jack = new jack_proxy(this);

 	_menu_jack_start->signal_activate().connect(
 		sigc::mem_fun(_jack, &jack_proxy::start_server));
 	_menu_jack_stop->signal_activate().connect(
 		sigc::mem_fun(_jack, &jack_proxy::stop_server));

	jack_status_changed(_jack->is_started());

	connect_widgets();
	update_state();

	_canvas->grab_focus();

	// Idle callback, check if we need to refresh
	Glib::signal_timeout().connect(
			sigc::mem_fun(this, &Patchage::idle_callback), 100);
}

Patchage::~Patchage() 
{
	delete _jack;
	delete _lash;
	delete _project_list;
	delete _session;
	delete _state_manager;
	delete _a2j;

	_about_win.destroy();
	_messages_win.destroy();
	_main_win.destroy();
}


bool
Patchage::idle_callback() 
{
	update_load();

	return true;
}


void
Patchage::update_toolbar()
{
	bool started;

	started = _jack->is_started();

	_buffer_size_combo->set_sensitive(started);

	if (started)
	{
		_buffer_size_combo->set_active((int)log2f(_jack->buffer_size()) - 5);
	}
}


void
Patchage::update_load()
{
	if (!_jack->is_started())
	{
		_main_xrun_progress->set_text("JACK stopped");
		return;
	}

	char tmp_buf[8];
	snprintf(tmp_buf, 8, "%zd", _jack->xruns());

	_main_xrun_progress->set_text(string(tmp_buf) + " Dropouts");

	float load = _jack->get_dsp_load();

	load /= 100.0;								// dbus returns it in percents, we use 0..1

	if (load > _max_dsp_load)
	{
		_max_dsp_load = load;
		_main_xrun_progress->set_fraction(load);
	}
}


void
Patchage::zoom(double z)
{
	_state_manager->set_zoom(z);
	_canvas->set_zoom(z);
}


void
Patchage::refresh() 
{
	assert(_canvas);

	_canvas->destroy();

	if (_jack)
		_jack->refresh();

	for (ItemList::iterator i = _canvas->items().begin(); i != _canvas->items().end(); ++i) {
		(*i)->resize();
	}
}


/** Update the stored window location and size in the StateManager (in memory).
 */
void
Patchage::store_window_location()
{
	int loc_x, loc_y, size_x, size_y;
	_main_win->get_position(loc_x, loc_y);
	_main_win->get_size(size_x, size_y);
	Coord window_location;
	window_location.x = loc_x;
	window_location.y = loc_y;
	Coord window_size;
	window_size.x = size_x;
	window_size.y = size_y;
	_state_manager->set_window_location(window_location);
	_state_manager->set_window_size(window_size);
}


void
Patchage::clear_load()
{
	_main_xrun_progress->set_fraction(0.0);
	_jack->reset_xruns();
	_max_dsp_load = 0.0;
}


void
Patchage::status_message(const string& msg) 
{
	if (_status_text->get_buffer()->size() > 0)
		_status_text->get_buffer()->insert(_status_text->get_buffer()->end(), "\n");

	_status_text->get_buffer()->insert(_status_text->get_buffer()->end(), msg);
	_status_text->scroll_to_mark(_status_text->get_buffer()->get_insert(), 0);
}


void
Patchage::update_state()
{
	for (ItemList::iterator i = _canvas->items().begin(); i != _canvas->items().end(); ++i) {
		shared_ptr<Module> module = dynamic_pointer_cast<Module>(*i);
		if (module) 
			module->load_location();
	}
}


/** Update the sensitivity status of menus to reflect the present.
 *
 * (eg. disable "Connect to Jack" when Patchage is already connected to Jack)
 */
void
Patchage::connect_widgets()
{
#if 0
	_lash_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(*_menu_lash_connect, &Gtk::MenuItem::set_sensitive), false));
	_lash_driver->signal_attached.connect(sigc::bind(
			sigc::mem_fun(*_menu_lash_disconnect, &Gtk::MenuItem::set_sensitive), true));
	
	_lash_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(*_menu_lash_connect, &Gtk::MenuItem::set_sensitive), true));
	_lash_driver->signal_detached.connect(sigc::bind(
			sigc::mem_fun(*_menu_lash_disconnect, &Gtk::MenuItem::set_sensitive), false));
#endif

	_jack->signal_started.connect(
		sigc::bind(sigc::mem_fun(this, &Patchage::jack_status_changed), true));
	
	_jack->signal_stopped.connect(
		sigc::bind(sigc::mem_fun(this, &Patchage::jack_status_changed), false));
}

void
Patchage::jack_status_changed(
	bool started)
{
	update_toolbar();

	_menu_jack_start->set_sensitive(!started);
	_menu_jack_stop->set_sensitive(started);
	_clear_load_but->set_sensitive(started);
	if (!started)
	{
		_main_xrun_progress->set_fraction(0.0);
	}
}

void
Patchage::on_arrange() 
{
	assert(_canvas);
	
	_canvas->arrange();
}

	
void
Patchage::on_help_about() 
{
	_about_win->run();
	_about_win->hide();
}


void
Patchage::on_messages_clear()
{
	_status_text->get_buffer()->erase(
			_status_text->get_buffer()->begin(),
			_status_text->get_buffer()->end());
}

	
void
Patchage::on_messages_close() 
{
	_menu_view_messages->set_active(false);
}

	
bool
Patchage::on_messages_delete(GdkEventAny*) 
{
	_menu_view_messages->set_active(false);
	return true;
}


void
Patchage::on_quit() 
{
	_main_win->hide();
}


void
Patchage::on_show_messages()
{
	if (_menu_view_messages->get_active())
		_messages_win->present();
	else
		_messages_win->hide();
}

	
void
Patchage::on_store_positions() 
{
	store_window_location();
	_state_manager->save(_settings_filename);
}


void
Patchage::on_view_toolbar() 
{
	if (_menu_view_toolbar->get_active())
		_toolbar->show();
	else
		_toolbar->hide();
}

	
bool
Patchage::on_scroll(GdkEventScroll* ev) 
{
	cout << "ON SCROLL" << endl;
	return false;
}


void
Patchage::buffer_size_changed()
{
	const int selected = _buffer_size_combo->get_active_row_number();

	if (selected == -1) {
		update_toolbar();
	} else {
		jack_nframes_t buffer_size = 1 << (selected+5);
	
		// this check is temporal workaround for jack bug
		// we skip setting buffer size if it same as acutal one
		// proper place for such check is in jack
		if (_jack->buffer_size() != buffer_size)
		{
			if (!_jack->set_buffer_size(buffer_size))
			{
				update_toolbar(); // reset combo box to actual value
			}
		}
	}
}

void
Patchage::error_msg(const std::string& msg)
{
#if defined(LOG_TO_STATUS)
	status_message(msg);
#endif

#if defined(LOG_TO_STD)
	cerr << msg << endl;
#endif
}

void
Patchage::info_msg(const std::string& msg)
{
#if defined(LOG_TO_STATUS)
	status_message(msg);
#endif

#if defined(LOG_TO_STD)
	cerr << msg << endl;
#endif
}

bool
Patchage::dbus_call(
		bool response_expected,
		const char * service,
		const char * object,
		const char * iface,
		const char * method,
		DBusMessage ** reply_ptr_ptr,
		int in_type,
		va_list ap)
{
	DBusMessage* request_ptr;
	DBusMessage* reply_ptr;

	request_ptr = dbus_message_new_method_call(
		service,
		object,
		iface,
		method);
	if (!request_ptr) {
		throw std::runtime_error("dbus_message_new_method_call() returned 0");
	}

	dbus_message_append_args_valist(request_ptr, in_type, ap);

	// send message and get a handle for a reply
	reply_ptr = dbus_connection_send_with_reply_and_block(
		_dbus_connection,
		request_ptr,
		DBUS_CALL_DEFAULT_TIMEOUT,
		&_dbus_error);

	dbus_message_unref(request_ptr);

	if (!reply_ptr) {
		if (response_expected) {
			error_msg(str(boost::format("no reply from server when calling method '%s'"
					", error is '%s'") % method % _dbus_error.message));
		}
		dbus_error_free(&_dbus_error);
	} else {
		*reply_ptr_ptr = reply_ptr;
	}

	return reply_ptr;
}

bool
Patchage::dbus_call(
		bool response_expected,
		const char * serivce,
		const char * object,
		const char * iface,
		const char * method,
		DBusMessage ** reply_ptr_ptr,
		int in_type,
		...)
{
	bool ret;
	va_list ap;

	va_start(ap, in_type);

	ret = dbus_call(
		response_expected,
		serivce,
		object,
		iface,
		method,
		reply_ptr_ptr,
		in_type,
		ap);

	va_end(ap);

	return ap;
}

void
Patchage::set_lash_availability(
	bool lash_active)
{
	_project_list->set_lash_availability(lash_active);
	if (!lash_active)
	{
		_session->clear();
	}
}

struct loadable_project_list_column_record : public Gtk::TreeModel::ColumnRecord
{
	Gtk::TreeModelColumn<Glib::ustring> name;
	Gtk::TreeModelColumn<Glib::ustring> modified;
	Gtk::TreeModelColumn<Glib::ustring> description;
};

static
void
convert_timestamp_to_string(
	const time_t timestamp,
	std::string& timestamp_string)
{
	GDate mtime, now;
	gint days_diff;
	struct tm tm_mtime;
	time_t time_now;
	const gchar *format;
	gchar buf[256];

	if (timestamp == 0)
	{
		timestamp_string = "Unknown";
		return;
	}

	localtime_r(&timestamp, &tm_mtime);

	g_date_set_time_t(&mtime, timestamp);
	time_now = time(NULL);
	g_date_set_time_t(&now, time_now);

	days_diff = g_date_get_julian(&now) - g_date_get_julian(&mtime);

	if (days_diff == 0)
	{
		format = "Today at %H:%M";
	}
	else if (days_diff == 1)
	{
		format = "Yesterday at %H:%M";
	}
	else
	{
		if (days_diff > 1 && days_diff < 7)
		{
			format = "%A"; /* Days from last week */
		}
		else
		{
			format = "%x"; /* Any other date */
		}
	}

	if (strftime(buf, sizeof(buf), format, &tm_mtime) != 0)
	{
		timestamp_string = buf;
	}
	else
	{
		timestamp_string = "Unknown";
	}
}

class load_project_dialog
{
public:
	load_project_dialog(
		Patchage *app)
		: _app(app)
	{
		_dialog.init(g_xml, "load_project_dialog");
		_widget.init(g_xml, "loadable_projects_list");

		_columns.add(_columns.name);
		_columns.add(_columns.modified);
		_columns.add(_columns.description);

		_model = Gtk::ListStore::create(_columns);
		_widget->set_model(_model);

		_widget->remove_all_columns();
		_widget->append_column("Project Name", _columns.name);
		_widget->append_column("Modified", _columns.modified);
		_widget->append_column("Description", _columns.description);
	}

	void
	run(
		std::list<lash_project_info>& projects)
	{
		Gtk::TreeModel::Row row;
		int result;

		for (std::list<lash_project_info>::iterator iter = projects.begin(); iter != projects.end(); iter++)
		{
			std::string str;
			row = *(_model->append());
			row[_columns.name] = iter->name;
			convert_timestamp_to_string(iter->modification_time, str);
			row[_columns.modified] = str;
			row[_columns.description] = iter->description;
		}

		_widget->signal_button_press_event().connect(sigc::mem_fun(*this, &load_project_dialog::on_load_project_dialog_button_press_event), false);
		_widget->signal_key_press_event().connect(sigc::mem_fun(*this, &load_project_dialog::on_load_project_dialog_key_press_event), false);

	loop:
		result = _dialog->run();

		if (result == 2)
		{
			Glib::RefPtr<Gtk::TreeView::Selection> selection = _widget->get_selection();
			Gtk::TreeIter iter = selection->get_selected();
			if (!iter)
			{
				goto loop;
			}

			Glib::ustring project_name = (*iter)[_columns.name];
			_app->load_project(project_name);
		}

		_dialog->hide();
	}

private:
	void
	load_selected_project()
	{
			Glib::RefPtr<Gtk::TreeView::Selection> selection = _widget->get_selection();
			Glib::ustring name = (*selection->get_selected())[_columns.name];
			_app->load_project(name);
			_dialog->hide();
	}

	bool
	on_load_project_dialog_button_press_event(GdkEventButton * event_ptr)
	{
		if (event_ptr->type == GDK_2BUTTON_PRESS && event_ptr->button == 1)
		{
			load_selected_project();
			return true;
		}

		return false;
	}

	bool
	on_load_project_dialog_key_press_event(GdkEventKey * event_ptr)
	{
		if (event_ptr->type == GDK_KEY_PRESS &&
				(event_ptr->keyval == GDK_Return ||
				 event_ptr->keyval == GDK_KP_Enter))
		{
			load_selected_project();
			return true;
		}

		return false;
	}

	Patchage *_app;
	Widget<Gtk::Dialog> _dialog;
	Widget<Gtk::TreeView> _widget;
	loadable_project_list_column_record _columns;
	Glib::RefPtr<Gtk::ListStore> _model;
};

void
Patchage::load_project_ask()
{
	std::list<lash_project_info> projects;
	load_project_dialog dialog(this);

	_lash->get_available_projects(projects);

	dialog.run(projects);
}

void
Patchage::load_project(
	const std::string& project_name)
{
	_lash->load_project(project_name);
}

void
Patchage::save_all_projects()
{
	_lash->save_all_projects();
}

void
Patchage::save_project(
	const std::string& project_name)
{
	_lash->save_project(project_name);
}

void
Patchage::close_project(
	const std::string& project_name)
{
	_lash->close_project(project_name);
}

void
Patchage::close_all_projects()
{
	_lash->close_all_projects();
}

bool
Patchage::maybe_map_j2a(
	const char * jack_client_name,
	const char * jack_port_name,
	string& canvas_client_name,
	string& canvas_port_name,
	bool& is_a2j_mapped)
{
	is_a2j_mapped = strcmp(_a2j->get_jack_client_name(), jack_client_name) == 0;

	if (!is_a2j_mapped)
	{
		canvas_client_name = jack_client_name;
		canvas_port_name = jack_port_name;
		return true;
	}

	if (!_a2j->map_jack_port(jack_port_name, canvas_client_name, canvas_port_name))
	{
		is_a2j_mapped = false;
		return false;
	}

	return true;
}

void
Patchage::on_port_added(
	const char * jack_client_name,
	const char * jack_port_name,
	PortType port_type,
	bool is_input,
	bool is_terminal)
{
	bool is_a2j_mapped;
	string canvas_client_name;
	string canvas_port_name;

	if (!maybe_map_j2a(
				jack_client_name,
				jack_port_name,
				canvas_client_name,
				canvas_port_name,
				is_a2j_mapped))
	{
		return;
	}

	ModuleType module_type = InputOutput;
	if (_state_manager->get_module_split(canvas_client_name, is_terminal)) {
		if (is_input) {
			module_type = Input;
		} else {
			module_type = Output;
		}
	}

	boost::shared_ptr<PatchageModule> module = _canvas->find_module(canvas_client_name, module_type);

	if (!module) {
		module = boost::shared_ptr<PatchageModule>(new PatchageModule(this, canvas_client_name, module_type));
		module->load_location();
		_canvas->add_item(module);
	}

	if (module->get_port(canvas_port_name)) {
		return;
	}

	boost::shared_ptr<PatchagePort> port = boost::shared_ptr<PatchagePort>(
		new PatchagePort(
			module,
			canvas_port_name,
			is_input,
			_state_manager->get_port_color(port_type)));

	port->type = port_type;
	port->is_a2j_mapped = is_a2j_mapped;
	if (is_a2j_mapped)
	{
		port->a2j_jack_port_name = jack_port_name;
	}

	module->add_port(port);

	module->resize();
}

void
Patchage::on_port_removed(
	const char * jack_client_name,
	const char * jack_port_name)
{
	bool is_a2j_mapped;
	string canvas_client_name;
	string canvas_port_name;

	if (!maybe_map_j2a(
				jack_client_name,
				jack_port_name,
				canvas_client_name,
				canvas_port_name,
				is_a2j_mapped))
	{
		return;
	}

	boost::shared_ptr<PatchagePort> port = dynamic_pointer_cast<PatchagePort>(_canvas->get_port(canvas_client_name, canvas_port_name));
	if (!port) {
		error_msg(str(boost::format("Unable to remove unknown port '%s':'%s'") % canvas_client_name % canvas_port_name));
		return;
	}

	boost::shared_ptr<PatchageModule> module = dynamic_pointer_cast<PatchageModule>(port->module().lock());

	module->remove_port(port);
	port.reset();
			
	// No empty modules (for now)
	if (module->num_ports() == 0) {
		_canvas->remove_item(module);
		module.reset();
	} else {
		module->resize();
	}
}

void
Patchage::on_ports_connected(
	const char * jack_client1_name,
	const char * jack_port1_name,
	const char * jack_client2_name,
	const char * jack_port2_name)
{
	string canvas_client1_name;
	string canvas_port1_name;
	string canvas_client2_name;
	string canvas_port2_name;
	bool is_a2j_mapped;

	if (!maybe_map_j2a(
				jack_client1_name,
				jack_port1_name,
				canvas_client1_name,
				canvas_port1_name,
				is_a2j_mapped))
	{
		return;
	}

	if (!maybe_map_j2a(
				jack_client2_name,
				jack_port2_name,
				canvas_client2_name,
				canvas_port2_name,
				is_a2j_mapped))
	{
		return;
	}

	boost::shared_ptr<PatchagePort> port1 = dynamic_pointer_cast<PatchagePort>(_canvas->get_port(canvas_client1_name, canvas_port1_name));
	if (!port1) {
		error_msg((string)"Unable to connect unknown port '" + canvas_port1_name + "' of client '" + canvas_client1_name + "'");
		return;
	}

	boost::shared_ptr<PatchagePort> port2 = dynamic_pointer_cast<PatchagePort>(_canvas->get_port(canvas_client2_name, canvas_port2_name));
	if (!port2) {
		error_msg((string)"Unable to connect unknown port '" + canvas_port2_name + "' of client '" + canvas_client2_name + "'");
		return;
	}

	_canvas->add_connection(port1, port2, port1->color() + 0x22222200);
}

void
Patchage::on_ports_disconnected(
	const char * jack_client1_name,
	const char * jack_port1_name,
	const char * jack_client2_name,
	const char * jack_port2_name)
{
	string canvas_client1_name;
	string canvas_port1_name;
	string canvas_client2_name;
	string canvas_port2_name;
	bool is_a2j_mapped;

	if (!maybe_map_j2a(
				jack_client1_name,
				jack_port1_name,
				canvas_client1_name,
				canvas_port1_name,
				is_a2j_mapped))
	{
		return;
	}

	if (!maybe_map_j2a(
				jack_client2_name,
				jack_port2_name,
				canvas_client2_name,
				canvas_port2_name,
				is_a2j_mapped))
	{
		return;
	}

	boost::shared_ptr<PatchagePort> port1 = dynamic_pointer_cast<PatchagePort>(_canvas->get_port(canvas_client1_name, canvas_port1_name));
	if (!port1) {
		error_msg((string)"Unable to disconnect unknown port '" + canvas_port1_name + "' of client '" + canvas_client1_name + "'");
		return;
	}

	boost::shared_ptr<PatchagePort> port2 = dynamic_pointer_cast<PatchagePort>(_canvas->get_port(canvas_client2_name, canvas_port2_name));
	if (!port2) {
		error_msg((string)"Unable to disconnect unknown port '" + canvas_port2_name + "' of client '" + canvas_client2_name + "'");
		return;
	}

	_canvas->remove_connection(port1, port2);
}

static
bool
port_type_match(
	boost::shared_ptr<PatchagePort> port1,
	boost::shared_ptr<PatchagePort> port2)
{
	return port1->type == port2->type;
}

void
Patchage::get_port_jack_names(
	boost::shared_ptr<PatchagePort> port,
	string& jack_client_name,
	string& jack_port_name)
{
	if (port->is_a2j_mapped)
	{
		jack_client_name = _a2j->get_jack_client_name();
		jack_port_name = port->a2j_jack_port_name;
	}
	else
	{
		jack_client_name = port->module().lock()->name();
		jack_port_name = port->name();
	}
}

void
Patchage::connect(
	boost::shared_ptr<PatchagePort> port1,
	boost::shared_ptr<PatchagePort> port2)
{
	string jack_client1_name;
	string jack_port1_name;
	string jack_client2_name;
	string jack_port2_name;

	if (port_type_match(port1, port2))
	{
		get_port_jack_names(port1, jack_client1_name, jack_port1_name);
		get_port_jack_names(port2, jack_client2_name, jack_port2_name);

		_jack->connect(
			jack_client1_name.c_str(),
			jack_port1_name.c_str(),
			jack_client2_name.c_str(),
			jack_port2_name.c_str());
	}
	else
	{
		status_message("ERROR: Attempt to connect ports with mismatched types");
	}
}

void
Patchage::disconnect(
	boost::shared_ptr<PatchagePort> port1,
	boost::shared_ptr<PatchagePort> port2)
{
	string jack_client1_name;
	string jack_port1_name;
	string jack_client2_name;
	string jack_port2_name;

	if (port_type_match(port1, port2))
	{
		get_port_jack_names(port1, jack_client1_name, jack_port1_name);
		get_port_jack_names(port2, jack_client2_name, jack_port2_name);

		_jack->disconnect(
			jack_client1_name.c_str(),
			jack_port1_name.c_str(),
			jack_client2_name.c_str(),
			jack_port2_name.c_str());
	}
	else
	{
		status_message("ERROR: Attempt to disconnect ports with mismatched types");
	}
}

/** Destroy all JACK (canvas) ports.
 */
void
Patchage::clear_canvas()
{
	ItemList modules = _canvas->items(); // copy
	for (ItemList::iterator m = modules.begin(); m != modules.end(); ++m) {
		shared_ptr<Module> module = dynamic_pointer_cast<Module>(*m);
		if (!module)
			continue;

		PortVector ports = module->ports(); // copy
		for (PortVector::iterator p = ports.begin(); p != ports.end(); ++p) {
			boost::shared_ptr<PatchagePort> port = boost::dynamic_pointer_cast<PatchagePort>(*p);
			if (port)
			{
				module->remove_port(port);
				port->hide();
			}
		}

		if (module->ports().empty())
			_canvas->remove_item(module);
		else
			module->resize();
	}
}

bool
Patchage::is_canvas_empty()
{
	return _canvas->items().empty();
}
