#!/usr/bin/env python
# Licensed under the GNU GPL v2 or later, see COPYING file for details.
# Copyright (C) 2008 Dave Robillard
# Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
import os
import Params
import autowaf

# Version of this package (even if built as a child)
PATCHAGE_VERSION = '5'

# Variables for 'waf dist'
APPNAME = 'lpatchage'
VERSION = PATCHAGE_VERSION
APP_HUMAN_NAME = 'LADI Patchage'

# Mandatory variables
srcdir = '.'
blddir = 'build'

def set_options(opt):
	autowaf.set_options(opt, False, False, False)
	opt.add_option('--install-name', type='string', default=APPNAME, dest='app_install_name', help="Install name. [Default: '" + APPNAME + "']")
	opt.add_option('--app-human-name', type='string', default=APP_HUMAN_NAME, dest='app_human_name', help="Human name for app. [Default: '" + APP_HUMAN_NAME + "']")

def configure(conf):
	autowaf.configure(conf)
	autowaf.check_tool(conf, 'compiler_cxx')
	autowaf.check_tool(conf, 'compiler_cc')

	conf.check_tool('misc') # subst tool

	conf.check_pkg('dbus-1', mandatory=True)
	conf.check_pkg('dbus-glib-1', mandatory=True)
	conf.check_pkg('glibmm-2.4', mandatory=True)
	conf.check_pkg('gtkmm-2.4', mandatory=True, vnum='2.11.12')
	conf.check_pkg('libgnomecanvasmm-2.6', mandatory=True)
	conf.check_pkg('libglademm-2.4', mandatory=True)

	# You need the boost headers package (e.g. libboost-dev)
	conf.check_header('boost/shared_ptr.hpp', mandatory=True)
	conf.check_header('boost/weak_ptr.hpp', mandatory=True)

	conf.check_pkg('flowcanvas', mandatory=True, vnum='0.4.0')

	conf.env['PATCHAGE_VERSION'] = PATCHAGE_VERSION
	conf.env['APP_INSTALL_NAME'] = Params.g_options.app_install_name
	conf.env['APP_HUMAN_NAME'] = Params.g_options.app_human_name
	if conf.env['BUNDLE']:
		conf.define('PATCHAGE_DATA_DIR', os.path.normpath(
				conf.env['DATADIRNAME'] + conf.env['APP_INSTALL_NAME']))
	else:
		conf.define('PATCHAGE_DATA_DIR', os.path.normpath(
				conf.env['DATADIR'] + conf.env['APP_INSTALL_NAME']))
	
	conf.write_config_header('config.h')

	print
	autowaf.display_msg("Install prefix", conf.env['PREFIX'], 'CYAN')
	autowaf.display_msg("Install name", "'" + conf.env['APP_INSTALL_NAME'] + "'", 'CYAN')
	autowaf.display_msg("App human name", "'" + conf.env['APP_HUMAN_NAME'] + "'", 'CYAN')
	print

def build(bld):
	# Program
	prog = bld.create_obj('cpp', 'program')
	prog.includes = 'src' # make waf dependency tracking work
	prog.target = bld.env()['APP_INSTALL_NAME']
	prog.inst_dir = bld.env()['BINDIRNAME']
	prog.uselib = 'DBUS-1 LIBGNOMECANVASMM-2.6 LIBGLADEMM-2.4 FLOWCANVAS DBUS-GLIB-1'
	prog.source = [
	    'src/main.cpp',
	    'src/Patchage.cpp',
	    'src/PatchageCanvas.cpp',
	    'src/StateManager.cpp',
	    'src/jack_proxy.cpp',
	    'src/lash_client.cpp',
	    'src/lash_proxy.cpp',
	    'src/load_projects_dialog.cpp',
	    'src/project.cpp',
	    'src/project_list.cpp',
	    'src/project_properties.cpp',
	    'src/session.cpp',
	    'src/a2j_proxy.cpp',
	    'src/dbus_helpers.c'
	    ]
	
	# Executable wrapper script (if building a bundle)
	autowaf.build_wrapper(bld, 'patchage.in', prog)

	# Glade UI definitions (XML)
	install_files('DATADIR', bld.env()['APP_INSTALL_NAME'], 'src/patchage.glade')
	
	# 'Desktop' file (menu entry, icon, etc)
	obj = bld.create_obj('subst')
	obj.source = 'patchage.desktop.in'
	obj.target = 'patchage.desktop'
	obj.dict = {
		'BINDIR'           : bld.env()['BINDIR'],
		'APP_INSTALL_NAME' : bld.env()['APP_INSTALL_NAME'],
		'APP_HUMAN_NAME'   : bld.env()['APP_HUMAN_NAME'],
	}
	install_as(os.path.normpath(bld.env()['DATADIR'] + 'applications/'), bld.env()['APP_INSTALL_NAME'] + '.desktop', 'build/default/patchage.desktop')

	# Icons
	#
	# Installation layout (with /usr prefix)
	# /usr/bin/patchage
	# /usr/share/applications/patchage.desktop
	# /usr/share/icons/hicolor/16x16/apps/patchage.png
	# /usr/share/icons/hicolor/22x22/apps/patchage.png
	# /usr/share/icons/hicolor/24x24/apps/patchage.png
	# /usr/share/icons/hicolor/32x32/apps/patchage.png
	# /usr/share/icons/hicolor/48x48/apps/patchage.png
	# /usr/share/icons/hicolor/scalable/apps/patchage.svg
	# /usr/share/patchage/patchage.glade
	#
	# icon cache is updated using:
	# gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

	# Dave disabled this, ask why before removing this
	#install_as(os.path.normpath(bld.env()['PREFIX'] + '/share/icons/hicolor/scalable/apps/'), bld.env()['APP_INSTALL_NAME'] + '.svg', 'icons/scalable/patchage.svg')

	icon_sizes = ['16x16', '22x22', '24x24', '32x32', '48x48']
	for icon_size in icon_sizes:
		install_as(os.path.normpath(bld.env()['DATADIR'] + '/icons/hicolor/' + icon_size + '/apps/'), bld.env()['APP_INSTALL_NAME'] + '.png', 'icons/' + icon_size + '/patchage.png')

def shutdown():
	autowaf.shutdown()

