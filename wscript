#!/usr/bin/env python
# Licensed under the GNU GPL v2 or later, see COPYING file for details.
# Copyright (C) 2008 Dave Robillard
# Copyright (C) 2008 Nedko Arnaudov
import os
import Params
import autowaf

# Version of this package (even if built as a child)
PATCHAGE_VERSION = '0.4.2'

# Variables for 'waf dist'
APPNAME = 'patchage'
VERSION = PATCHAGE_VERSION
APP_HUMAN_NAME = 'Patchage'

# Mandatory variables
srcdir = '.'
blddir = 'build'

def set_options(opt):
	autowaf.set_options(opt)
	opt.add_option('--install-name', type='string', default=APPNAME, dest='app_install_name',
			help="Install name. [Default: '" + APPNAME + "']")
	opt.add_option('--app-human-name', type='string', default=APP_HUMAN_NAME, dest='app_human_name',
			help="Human name for app. [Default: '" + APP_HUMAN_NAME + "']")
	opt.add_option('--jack-dbus', action='store_true', default=False, dest='jack_dbus',
			help="Use Jack via D-Bus [Default: False (use libjack)]")
	opt.add_option('--no-lash', action='store_true', default=False, dest='no_lash',
			help="Do not build Lash support")
	opt.add_option('--no-alsa', action='store_true', default=False, dest='no_alsa',
			help="Do not build Alsa Sequencer support")

def configure(conf):
	autowaf.configure(conf)
	autowaf.check_tool(conf, 'compiler_cxx')
	autowaf.check_pkg(conf, 'dbus-1', destvar='DBUS', mandatory=False)
	autowaf.check_pkg(conf, 'dbus-glib-1', destvar='DBUS_GLIB', mandatory=False)
	autowaf.check_pkg(conf, 'flowcanvas', destvar='FLOWCANVAS', vnum='0.5.1', mandatory=True)
	autowaf.check_pkg(conf, 'libglademm-2.4', destvar='GLADEMM', vnum='2.6.0', mandatory=True)
	autowaf.check_pkg(conf, 'glibmm-2.4', destvar='GLIBMM', vnum='2.14.0', mandatory=True)
	autowaf.check_pkg(conf, 'libgnomecanvasmm-2.6', destvar='GNOMECANVASMM', mandatory=True)
	autowaf.check_pkg(conf, 'gthread-2.0', destvar='GTHREAD', vnum='2.14.0', mandatory=True)
	autowaf.check_pkg(conf, 'gtkmm-2.4', destvar='GTKMM', vnum='2.11.12', mandatory=True)
	autowaf.check_pkg(conf, 'raul', destvar='RAUL', vnum='0.5.1', mandatory=True)
	
	# Use Jack D-Bus if requested (only one jack driver is allowed)
	conf.env['HAVE_JACK_DBUS'] = conf.env['HAVE_DBUS'] == 1 and Params.g_options.jack_dbus

	if conf.env['HAVE_JACK_DBUS']:
		conf.define('HAVE_JACK_DBUS', conf.env['HAVE_JACK_DBUS'])
	else:
		autowaf.check_pkg(conf, 'jack', destvar='JACK', vnum='0.107.0', mandatory=False)
		conf.define('USE_LIBJACK', conf.env['HAVE_JACK'] == 1)
	
	conf.define('HAVE_JACK_MIDI', conf.env['HAVE_JACK'] == 1 or conf.env['HAVE_JACK_DBUS'] == 1)

	# Use Alsa if present unless --no-alsa
	if not Params.g_options.no_alsa and conf.env['HAVE_ALSA'] != 1:
		conf.check_pkg('alsa', destvar='ALSA', mandatory=False)
	
	# Use LASH if we have DBUS unless --no-lash
	if not Params.g_options.no_lash and conf.env['HAVE_DBUS_GLIB']:
		conf.env['HAVE_LASH'] = 1
		conf.define('HAVE_LASH', 1)
	else:
		conf.env['HAVE_LASH'] = False
	
	conf.check_tool('misc') # subst tool
	
	# Boost headers (e.g. libboost-dev)
	autowaf.check_header(conf, 'boost/shared_ptr.hpp', mandatory=True)
	autowaf.check_header(conf, 'boost/weak_ptr.hpp', mandatory=True)
	
	conf.env['PATCHAGE_VERSION'] = PATCHAGE_VERSION

	conf.env['APP_INSTALL_NAME'] = Params.g_options.app_install_name
	conf.env['APP_HUMAN_NAME'] = Params.g_options.app_human_name
	conf.define('PATCHAGE_DATA_DIR', os.path.normpath(
			conf.env['DATADIR'] + conf.env['APP_INSTALL_NAME']))
	
	conf.write_config_header('config.h')
	
	autowaf.print_summary(conf)
	autowaf.display_header('Patchage Configuration')
	autowaf.display_msg("Install name", "'" + conf.env['APP_INSTALL_NAME'] + "'", 'CYAN')
	autowaf.display_msg("App human name", "'" + conf.env['APP_HUMAN_NAME'] + "'", 'CYAN')
	autowaf.display_msg("Jack (D-Bus)", str(conf.env['HAVE_JACK_DBUS']), 'YELLOW')
	autowaf.display_msg("LASH (D-Bus)", str(conf.env['HAVE_LASH'] == 1), 'YELLOW')
	autowaf.display_msg("Jack (libjack)", str(conf.env['USE_LIBJACK']), 'YELLOW')
	autowaf.display_msg("Alsa Sequencer", str(conf.env['HAVE_ALSA'] == 1), 'YELLOW')
	print

def build(bld):
	# Program
	prog = bld.create_obj('cpp', 'program')
	prog.includes = 'src' # make waf dependency tracking work
	prog.target = bld.env()['APP_INSTALL_NAME']
	prog.inst_dir = bld.env()['BINDIRNAME']
	autowaf.use_lib(bld, prog, 'DBUS FLOWCANVAS GLADEMM DBUS_GLIB GNOMECANVASMM GTHREAD RAUL')
	prog.source = '''
		src/LashClient.cpp
		src/Patchage.cpp
		src/PatchageCanvas.cpp
		src/PatchageEvent.cpp
		src/StateManager.cpp
		src/main.cpp
	'''
	if bld.env()['HAVE_JACK_DBUS']:
		prog.source += '''
			src/JackDbusDriver.cpp
		'''
	if bld.env()['HAVE_LASH']:
		prog.source += '''
			src/LashProxy.cpp
			src/LoadProjectDialog.cpp
			src/Project.cpp
			src/ProjectList.cpp
			src/ProjectPropertiesDialog.cpp
			src/Session.cpp
		'''
	if bld.env()['HAVE_LASH'] or bld.env()['HAVE_JACK_DBUS']:
		prog.source += ' src/DBus.cpp '
	if bld.env()['USE_LIBJACK']:
		prog.source += ' src/JackDriver.cpp '
		prog.uselib += ' JACK '
	if bld.env()['HAVE_ALSA'] == 1:
		prog.source += ' src/AlsaDriver.cpp '
		prog.uselib += ' ALSA '
	
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
	obj.inst_var = 'PREFIX'
	obj.inst_dir = bld.env()['DATADIRNAME'] + 'applications'
	
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
	icon_sizes = ['16x16', '22x22', '24x24', '32x32', '48x48']
	for s in icon_sizes:
		install_as(
			os.path.normpath(bld.env()['DATADIR'] + '/icons/hicolor/' + s + '/apps/'),
			bld.env()['APP_INSTALL_NAME'] + '.png',
			'icons/' + s + '/patchage.png')

def shutdown():
	autowaf.shutdown()

