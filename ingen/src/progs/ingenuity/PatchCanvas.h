/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef PATCHCANVAS_H
#define PATCHCANVAS_H

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include <flowcanvas/FlowCanvas.h>
#include <flowcanvas/Module.h>
#include <raul/SharedPtr.h>
#include <raul/Path.h>
#include "ConnectionModel.h"
#include "PatchModel.h"
#include "NodeModule.h"

using std::string;
using namespace LibFlowCanvas;

using LibFlowCanvas::Port;
using Ingen::Client::ConnectionModel;
using Ingen::Client::PatchModel;
using Ingen::Client::NodeModel;
using Ingen::Client::PortModel;
using Ingen::Client::MetadataMap;

namespace Ingenuity {
	
class NodeModule;


/** Patch canvas widget.
 *
 * \ingroup Ingenuity
 */
class PatchCanvas : public LibFlowCanvas::FlowCanvas
{
public:
	PatchCanvas(SharedPtr<PatchModel> patch, int width, int height);
	
	virtual ~PatchCanvas() {}

	/*boost::shared_ptr<NodeModule> find_module(const string& name) {
		return boost::dynamic_pointer_cast<NodeModule>(
			FlowCanvas::get_item(name));
	}*/
	
	void build();
	void arrange();

	void add_node(SharedPtr<NodeModel> nm);
	void remove_node(SharedPtr<NodeModel> nm);
	void add_port(SharedPtr<PortModel> pm);
	void remove_port(SharedPtr<PortModel> pm);
	void connection(SharedPtr<ConnectionModel> cm);
	void disconnection(SharedPtr<ConnectionModel> cm);

	void get_new_module_location(double& x, double& y);

	void destroy_selection();
	void copy_selection();

	void show_menu(GdkEvent* event)
	{ _menu->popup(event->button.button, event->button.time); }
	
private:
	string generate_port_name(const string& base);
	void menu_add_port(const string& name, const string& type, bool is_output);
	void menu_load_plugin();
	void menu_new_patch();
	void menu_load_patch();
	
	MetadataMap get_initial_data();

	bool canvas_event(GdkEvent* event);
	
	SharedPtr<LibFlowCanvas::Port> get_port_view(SharedPtr<PortModel> port);

	void connect(boost::shared_ptr<LibFlowCanvas::Connectable> src,
	             boost::shared_ptr<LibFlowCanvas::Connectable> dst);

	void disconnect(boost::shared_ptr<LibFlowCanvas::Connectable> src,
	                boost::shared_ptr<LibFlowCanvas::Connectable> dst);

	SharedPtr<PatchModel> _patch;

	typedef std::map<SharedPtr<ObjectModel>, SharedPtr<LibFlowCanvas::Module> > Views;
	Views _views;

	int _last_click_x;
	int _last_click_y;
	
	Gtk::Menu*      _menu;
	Gtk::MenuItem*  _menu_add_audio_input;
	Gtk::MenuItem*  _menu_add_audio_output;
	Gtk::MenuItem*  _menu_add_control_input;
	Gtk::MenuItem*  _menu_add_control_output;
	Gtk::MenuItem*  _menu_add_midi_input;
	Gtk::MenuItem*  _menu_add_midi_output;
	Gtk::MenuItem*  _menu_load_plugin;
	Gtk::MenuItem*  _menu_load_patch;
	Gtk::MenuItem*  _menu_new_patch;
};


} // namespace Ingenuity

#endif // PATCHCANVAS_H
