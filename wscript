#! /usr/bin/env python
# encoding: utf-8
# waf build system from LADI patchage courtesy Nedko Arnaudov

import os
import Params
import commands
from Configure import g_maxlen
#g_maxlen = 40
import shutil

VERSION='0.4.3'
APPNAME='patchage'
APP_HUMAN_NAME='Patchage'

# these variables are mandatory ('/' are converted automatically)
srcdir = '.'
blddir = 'build'

def display_msg(msg, status = None, color = None):
    sr = msg
    global g_maxlen
    g_maxlen = max(g_maxlen, len(msg))
    if status:
        print "%s :" % msg.ljust(g_maxlen),
        Params.pprint(color, status)
    else:
        print "%s" % msg.ljust(g_maxlen)

def display_feature(msg, build):
    if build:
        display_msg(msg, "yes", 'GREEN')
    else:
        display_msg(msg, "no", 'YELLOW')

def set_options(opt):
    opt.tool_options('compiler_cxx')

    opt.add_option('--install-name', type='string', default=APPNAME, dest='app_install_name', help="Install name. [Default: '" + APPNAME + "']")
    opt.add_option('--app-human-name', type='string', default=APP_HUMAN_NAME, dest='app_human_name', help="Human name for app. [Default: '" + APP_HUMAN_NAME + "']")

def configure(conf):
    conf.check_tool('compiler_cxx')

    conf.check_tool('misc')             # subst tool

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

    conf.env['APP_INSTALL_NAME'] = Params.g_options.app_install_name
    conf.env['APP_HUMAN_NAME'] = Params.g_options.app_human_name

    conf.define('DATA_DIR', os.path.normpath(conf.env['PREFIX'] + '/share/' + conf.env['APP_INSTALL_NAME']))
    conf.write_config_header('config.h')

    print
    display_msg("Install prefix", conf.env['PREFIX'], 'CYAN')
    display_msg("Install name", "'" + conf.env['APP_INSTALL_NAME'] + "'", 'CYAN')
    display_msg("App human name", "'" + conf.env['APP_HUMAN_NAME'] + "'", 'CYAN')
    print

def build(bld):
    prog = bld.create_obj('cpp', 'program')
    prog.defines = [
	'CONFIG_H_PATH=\\"config.h\\"',
	]
    prog.source = [
	'src/main.cpp',
	'src/Patchage.cpp',
	'src/PatchageCanvas.cpp',
	'src/StateManager.cpp',
	'src/JackDriver.cpp',
	'src/JackDbusDriver.cpp',
	'src/LashClient.cpp',
	'src/LashProxy.cpp',
	'src/Project.cpp',
	'src/ProjectList.cpp',
	'src/ProjectPropertiesDialog.cpp',
	'src/Session.cpp'
    ]
    prog.includes = 'src' # make waf dependency tracking work
    prog.target = bld.env()['APP_INSTALL_NAME']
    prog.uselib = 'DBUS-1 LIBGNOMECANVASMM-2.6 LIBGLADEMM-2.4 FLOWCANVAS DBUS-GLIB-1'

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

    install_files('PREFIX', '/share/' + bld.env()['APP_INSTALL_NAME'], 'src/patchage.glade')

    # process patchage.desktop.in -> patchage.desktop
    #install_files('PREFIX', '/share/applications/', 'patchage.desktop')
    obj = bld.create_obj('subst')
    obj.source = 'patchage.desktop.in'
    obj.target = 'patchage.desktop'
    obj.dict = {
	'BINDIR': bld.env()['PREFIX'] + '/bin',
	'APP_INSTALL_NAME': bld.env()['APP_INSTALL_NAME'],
	'APP_HUMAN_NAME': bld.env()['APP_HUMAN_NAME'],
	}
    #obj.inst_var = os.path.normpath(bld.env()['PREFIX'] + '/share/applications/')
    #obj.inst_dir = '/'
    install_as(os.path.normpath(bld.env()['PREFIX'] + '/share/applications/'), bld.env()['APP_INSTALL_NAME'] + '.desktop', 'build/default/patchage.desktop')

    install_as(os.path.normpath(bld.env()['PREFIX'] + '/share/icons/hicolor/scalable/apps/'), bld.env()['APP_INSTALL_NAME'] + '.svg', 'icons/scalable/patchage.svg')

    icon_sizes = ['16x16', '22x22', '24x24', '32x32', '48x48']
    for icon_size in icon_sizes:
	install_as(os.path.normpath(bld.env()['PREFIX'] + '/share/icons/hicolor/' + icon_size + '/apps/'), bld.env()['APP_INSTALL_NAME'] + '.png', 'icons/' + icon_size + '/patchage.png')

    # TODO: ask some packagers for best policy about updating icon cache here
    # gtk-update-icon-cache -f -t $(datadir)/icons/hicolor
