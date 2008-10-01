#!/usr/bin/env python
# Parts from LADI Patchage by Nedko Arnaudov
import os
import Params
from Configure import g_maxlen

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
	opt.tool_options('compiler_cxx')
	opt.add_option('--install-name', type='string', default=APPNAME, dest='app_install_name',
			help="Install name. [Default: '" + APPNAME + "']")
	opt.add_option('--app-human-name', type='string', default=APP_HUMAN_NAME, dest='app_human_name',
			help="Human name for app. [Default: '" + APP_HUMAN_NAME + "']")
	opt.add_option('--no-jack-dbus', action='store_true', default=False, dest='no_jack_dbus',
			help="Do not build Jack via D-Bus support")
	opt.add_option('--no-lash', action='store_true', default=False, dest='no_lash',
			help="Do not build Lash support")
	opt.add_option('--no-alsa', action='store_true', default=False, dest='no_alsa',
			help="Do not build Alsa Sequencer support")

def display_msg(msg, status = None, color = None):
	global g_maxlen
	g_maxlen = max(g_maxlen, len(msg))
	if status:
		print "%s :" % msg.ljust(g_maxlen),
		Params.pprint(color, status)
	else:
		print "%s" % msg.ljust(g_maxlen)

def configure(conf):
	if not conf.env['CXX']:
		conf.check_tool('compiler_cxx')
	if not conf.env['HAVE_DBUS']:
		conf.check_pkg('dbus-1', destvar='DBUS', mandatory=False)
	if not conf.env['HAVE_DBUS_GLIB']:
		conf.check_pkg('dbus-glib-1', destvar='DBUS_GLIB', mandatory=False)
	if not conf.env['HAVE_FLOWCANVAS']:
		conf.check_pkg('flowcanvas', destvar='FLOWCANVAS', vnum='0.5.1', mandatory=True)
	if not conf.env['HAVE_GLADEMM']:
		conf.check_pkg('libglademm-2.4', destvar='GLADEMM', vnum='2.6.0', mandatory=True)
	if not conf.env['HAVE_GLIBMM']:
		conf.check_pkg('glibmm-2.4', destvar='GLIBMM', vnum='2.16.0', mandatory=True)
	if not conf.env['HAVE_GNOMECANVASMM']:
		conf.check_pkg('libgnomecanvasmm-2.6', destvar='GNOMECANVASMM', mandatory=True)
	if not conf.env['HAVE_GTHREAD']:
		conf.check_pkg('gthread-2.0', destvar='GTHREAD', vnum='2.16.0', mandatory=True)
	if not conf.env['HAVE_GTKMM']:
		conf.check_pkg('gtkmm-2.4', destvar='GTKMM', vnum='2.11.12', mandatory=True)
	if not conf.env['HAVE_RAUL']:
		conf.check_pkg('raul', destvar='RAUL', vnum='0.5.1', mandatory=True)
	
	# Use Jack D-Bus unless --no-jack-dbus (only one jack driver is allowed)
	conf.env['HAVE_JACK_DBUS'] = conf.env['HAVE_DBUS'] and not Params.g_options.no_jack_dbus
	if conf.env['HAVE_JACK_DBUS']:
		conf.define('HAVE_JACK_DBUS', True)
	elif not conf.env['HAVE_JACK']:
		conf.check_pkg('jack', destvar='JACK', vnum='0.107.0', mandatory=False)
		conf.define('USE_LIBJACK', conf.env['HAVE_JACK'])

	# Use Alsa if present unless --no-alsa
	if not Params.g_options.no_alsa and not conf.env['HAVE_ALSA']:
		conf.check_pkg('alsa', destvar='ALSA', mandatory=False)
	
	# Use LASH if we have DBUS unless --no-lash
	if not Params.g_options.no_lash and conf.env['HAVE_DBUS_GLIB']:
		conf.env['HAVE_LASH'] = True
		conf.define('HAVE_LASH', True)
	else:
		conf.env['HAVE_LASH'] = False
	
	conf.check_tool('misc') # subst tool
	
	# Boost headers (e.g. libboost-dev)
	conf.check_header('boost/shared_ptr.hpp', mandatory=True)
	conf.check_header('boost/weak_ptr.hpp', mandatory=True)
	
	conf.env['PATCHAGE_VERSION'] = PATCHAGE_VERSION
	conf.env.append_value('CCFLAGS', '-DCONFIG_H_PATH=\\\"waf-config.h\\\"')
	conf.env.append_value('CXXFLAGS', '-DCONFIG_H_PATH=\\\"waf-config.h\\\"')

	conf.env['APP_INSTALL_NAME'] = Params.g_options.app_install_name
	conf.env['APP_HUMAN_NAME'] = Params.g_options.app_human_name
	conf.define('DATA_DIR', os.path.normpath(
			conf.env['PREFIX'] + '/share/' + conf.env['APP_INSTALL_NAME']))
	
	conf.write_config_header('waf-config.h')
	
	print
	display_msg("Install prefix", conf.env['PREFIX'], 'CYAN')
	display_msg("Install name", "'" + conf.env['APP_INSTALL_NAME'] + "'", 'CYAN')
	display_msg("App human name", "'" + conf.env['APP_HUMAN_NAME'] + "'", 'CYAN')
	display_msg("Jack (D-Bus)", str(bool(conf.env['HAVE_JACK_DBUS'])), 'YELLOW')
	display_msg("LASH (D-Bus)", str(bool(conf.env['HAVE_LASH'])), 'YELLOW')
	display_msg("Jack (libjack)", str(bool(conf.env['USE_LIBJACK'])), 'YELLOW')
	display_msg("Alsa Sequencer", str(bool(conf.env['HAVE_ALSA'])), 'YELLOW')
	print

def build(bld):
	# Program
	prog = bld.create_obj('cpp', 'program')
	prog.includes = 'src' # make waf dependency tracking work
	prog.target = bld.env()['APP_INSTALL_NAME']
	prog.uselib = 'DBUS FLOWCANVAS GLADEMM DBUS_GLIB GNOMECANVASMM GTHREAD RAUL'
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
	if bld.env()['HAVE_ALSA']:
		prog.source += ' src/AlsaDriver.cpp '
		prog.uselib += ' ALSA '
	
	# Glade UI definitions (XML)
	install_files('PREFIX', '/share/' + bld.env()['APP_INSTALL_NAME'], 'src/patchage.glade')
	
	# 'Desktop' file (menu entry, icon, etc)
	obj = bld.create_obj('subst')
	obj.source = 'patchage.desktop.in'
	obj.target = 'patchage.desktop'
	obj.dict = {
		'BINDIR'           : bld.env()['PREFIX'] + '/bin',
		'APP_INSTALL_NAME' : bld.env()['APP_INSTALL_NAME'],
		'APP_HUMAN_NAME'   : bld.env()['APP_HUMAN_NAME'],
	}
	obj.inst_var = 'PREFIX'
	obj.inst_dir = '/share/applications'
	
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
			os.path.normpath(bld.env()['PREFIX'] + '/share/icons/hicolor/' + s + '/apps/'),
			bld.env()['APP_INSTALL_NAME'] + '.png',
			'icons/' + s + '/patchage.png')

