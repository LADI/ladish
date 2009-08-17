#! /usr/bin/env python
# encoding: utf-8

import os
import Options
import Utils

APPNAME='ladish'
VERSION='1'
DBUS_NAME_BASE = 'org.ladish'

# these variables are mandatory ('/' are converted automatically)
srcdir = '.'
blddir = 'build'

def display_msg(conf, msg="", status = None, color = None):
    if status:
        conf.check_message_1(msg)
        conf.check_message_2(status, color)
    else:
        Utils.pprint('NORMAL', msg)

def display_raw_text(conf, text, color = 'NORMAL'):
    Utils.pprint(color, text, sep = '')

def display_line(conf, text, color = 'NORMAL'):
    Utils.pprint(color, text, sep = os.linesep)

def set_options(opt):
    opt.tool_options('compiler_cc')
    opt.tool_options('compiler_cxx')
    opt.add_option('--enable-pkg-config-dbus-service-dir', action='store_true', default=False, help='force D-Bus service install dir to be one returned by pkg-config')

def add_cflag(conf, flag):
    conf.env.append_unique('CXXFLAGS', flag)
    conf.env.append_unique('CCFLAGS', flag)

def configure(conf):
    conf.check_tool('compiler_cc')
    conf.check_tool('compiler_cxx')

    conf.check_cfg(
        package = 'dbus-1',
        atleast_version = '1.0.0',
        mandatory = True,
        errmsg = "not installed, see http://dbus.freedesktop.org/",
        args = '--cflags --libs')

    dbus_dir = conf.check_cfg(package='dbus-1', args='--variable=session_bus_services_dir', msg="Retrieving D-Bus services dir")
    if not dbus_dir:
        return

    dbus_dir = dbus_dir.strip()
    conf.env['DBUS_SERVICES_DIR_REAL'] = dbus_dir

    if Options.options.enable_pkg_config_dbus_service_dir:
        conf.env['DBUS_SERVICES_DIR'] = dbus_dir
    else:
        conf.env['DBUS_SERVICES_DIR'] = os.path.join(os.path.normpath(conf.env['PREFIX']), 'share', 'dbus-1', 'services')

    conf.check_cfg(
        package = 'uuid',
        mandatory = True,
        errmsg = "not installed, see http://e2fsprogs.sourceforge.net/",
        args = '--cflags --libs')

    conf.check(header_name='expat.h', define_name="HAVE_EXPAT")

    if conf.is_defined('HAVE_EXPAT'):
        conf.env['LIB_EXPAT'] = ['expat']

    conf.check_cfg(
        package = 'dbus-glib-1',
        mandatory = True,
        errmsg = "not installed, see http://dbus.freedesktop.org/",
        args = '--cflags --libs')

    conf.check_cfg(
        package = 'glibmm-2.4',
        mandatory = True,
        errmsg = "not installed, see http://www.gtkmm.org/",
        args = '--cflags --libs')

    conf.check_cfg(
        package = 'gtkmm-2.4',
        mandatory = True,
        atleast_version = '2.11.12',
        errmsg = "not installed, see http://www.gtkmm.org/",
        args = '--cflags --libs')

    conf.check_cfg(
        package = 'libgnomecanvasmm-2.6',
        mandatory = True,
        errmsg = "not installed, see http://www.gtkmm.org/",
        args = '--cflags --libs')

    conf.check_cfg(
        package = 'libglademm-2.4',
        mandatory = True,
        errmsg = "not installed, see http://www.gtkmm.org/",
        args = '--cflags --libs')

    conf.check_cfg(
        package = 'flowcanvas',
        mandatory = True,
        atleast_version = '0.4.0',
        errmsg = "not installed, see http://drobilla.net/software/flowcanvas/",
        args = '--cflags --libs')

    # We need the boost headers package (e.g. libboost-dev)
    # shared_ptr.hpp and weak_ptr.hpp
    conf.check_tool('boost')
    conf.check_boost()

    add_cflag(conf, '-g')
    add_cflag(conf, '-Wall')
    add_cflag(conf, '-Werror')

    conf.define('DATA_DIR', os.path.normpath(os.path.join(conf.env['PREFIX'], 'share', APPNAME)))
    conf.define('DEFAULT_PROJECT_DIR', "audio-projects")
    conf.define('PACKAGE_VERSION', VERSION)
    conf.define('DBUS_NAME_BASE', DBUS_NAME_BASE)
    conf.define('DBUS_BASE_PATH', '/' + DBUS_NAME_BASE.replace('.', '/'))
    conf.define('_GNU_SOURCE', 1)
    conf.write_config_header('config.h')

    display_msg(conf)

    display_msg(conf, "==================")
    version_msg = APPNAME + "-" + VERSION

    #if svnrev:
    #    version_msg += " exported from r" + rev
    #else:
    #    version_msg += " git revision will checked and eventually updated during build"

    display_msg(conf, version_msg)

    display_msg(conf)
    display_msg(conf, "Install prefix", conf.env['PREFIX'], 'CYAN')

    display_msg(conf, 'D-Bus service install directory', conf.env['DBUS_SERVICES_DIR'], 'CYAN')

    if conf.env['DBUS_SERVICES_DIR'] != conf.env['DBUS_SERVICES_DIR_REAL']:
        display_msg(conf)
        display_line(conf,     "WARNING: D-Bus session services directory as reported by pkg-config is", 'RED')
        display_raw_text(conf, "WARNING:", 'RED')
        display_line(conf,      conf.env['DBUS_SERVICES_DIR_REAL'], 'CYAN')
        display_line(conf,     'WARNING: but service file will be installed in', 'RED')
        display_raw_text(conf, "WARNING:", 'RED')
        display_line(conf,      conf.env['DBUS_SERVICES_DIR'], 'CYAN')
        display_line(conf,     'WARNING: You may need to adjust your D-Bus configuration after installing jackdbus', 'RED')
        display_line(conf,     'WARNING: You can override dbus service install directory', 'RED')
        display_line(conf,     'WARNING: with --enable-pkg-config-dbus-service-dir option to this script', 'RED')

    display_msg(conf)

def build(bld):
    daemon = bld.new_task_gen('cc', 'program')
    daemon.target = 'ladishd'
    daemon.includes = "build/default" # XXX config.h version.h and other generated files
    daemon.uselib = 'DBUS-1 LIBXML-2.0 UUID'
    daemon.ver_header = 'version.h'
    daemon.env.append_value("LINKFLAGS", ["-lutil", "-ldl"])

    daemon.source = [
        'jack_proxy.c',
        ]

    for source in [
        'main.c',
        'loader.c',
        'log.c',
        'sigsegv.c',
        'proctitle.c',
        'appdb.c',
        'procfs.c',
        'dbus_iface_control.c',
        'jack.c',
        'studio.c',
        'catdup.c'
        ]:
        daemon.source.append(os.path.join("daemon", source))

    for source in [
        'service.c',
        'signal.c',
        'method.c',
        'error.c',
        'object_path.c',
        'introspection.c',
        'interface.c',
        'helpers.c',
        ]:
        daemon.source.append(os.path.join("dbus", source))

    daemon.source.append(os.path.join("common", "safety.c"))

    # process name.arnaudov.nedko.ladish.service.in -> name.arnaudov.nedko.ladish.service
    import misc
    obj = bld.new_task_gen('subst')
    obj.source = os.path.join('daemon', 'dbus.service.in')
    obj.target = DBUS_NAME_BASE + '.service'
    obj.dict = {'dbus_object_path': DBUS_NAME_BASE,
                'daemon_bin_path': os.path.join(bld.env['PREFIX'], 'bin', daemon.target)}
    obj.install_path = bld.env['DBUS_SERVICES_DIR'] + os.path.sep
    obj.fun = misc.subst_func

    liblash = bld.new_task_gen('cc', 'shlib')
    liblash.includes = "build/default" # XXX config.h version.h and other generated files
    liblash.uselib = 'DBUS-1 LIBXML-2.0 UUID'
    liblash.target = 'lash'
    liblash.vnum = "1.1.1"
    liblash.defines = ['LASH_OLD_API', 'DEBUG_OUTPUT_TERMINAL']
    liblash.source = []

    for source in [
        'lash.c',
        'lash_config.c',
        'client.c',
        'dbus_service.c',
        'dbus_iface_client.c',
        'protocol.c',
        'event.c',
        'args.c',
        ]:
        liblash.source.append(os.path.join("lash_compat", "liblash", source))

    for source in [
        'service.c',
        'signal.c',
        'method.c',
        'error.c',
        'object_path.c',
        'introspection.c',
        'interface.c',
        ]:
        liblash.source.append(os.path.join("dbus", source))

    liblash.source.append(os.path.join("common", "safety.c"))
    liblash.source.append(os.path.join("daemon", "legacy", "file.c"))

    # pkgpyexec_LTLIBRARIES = _lash.la
    # INCLUDES = $(PYTHON_INCLUDES)
    # _lash_la_LDFLAGS = -module -avoid-version ../liblash/liblash.la
    # _lash_la_SOURCES = lash.c lash.h lash_wrap.c
    # pkgpyexec_SCRIPTS = lash.py
    # CLEANFILES = lash_wrap.c lash.py lash.pyc zynjacku.defs
    # EXTRA_DIST = test.py lash.i lash.py
    # lash_wrap.c lash.py: lash.i lash.h
    # 	swig -o lash_wrap.c -I$(top_srcdir) -python $(top_srcdir)/$(subdir)/lash.i

    #####################################################
    # gladish
    gladish = bld.new_task_gen('cxx', 'program')
    gladish.features.append('cc')
    gladish.target = 'gladish'
    gladish.defines = ['DEBUG_OUTPUT_TERMINAL']
    gladish.includes = "build/default" # XXX config.h version.h and other generated files
    gladish.uselib = 'DBUS-1 LIBGNOMECANVASMM-2.6 LIBGLADEMM-2.4 FLOWCANVAS DBUS-GLIB-1'

    gladish.source = [
        'jack_proxy.c',
        'graph_proxy.c',
        ]

    for source in [
        'main.cpp',
        'Patchage.cpp',
        #'PatchageCanvas.cpp',
        #'StateManager.cpp',
        #'lash_client.cpp',
        #'lash_proxy.cpp',
        #'load_projects_dialog.cpp',
        #'project.cpp',
        #'project_list.cpp',
        #'project_properties.cpp',
        #'session.cpp',
        #'a2j_proxy.cpp',
        'dbus_helpers.c',
        'canvas.cpp',
        'graph_canvas.c',
        ]:
        gladish.source.append(os.path.join("gui", source))

    for source in [
        'method.c',
        'helpers.c',
        ]:
        gladish.source.append(os.path.join("dbus", source))

    # Glade UI definitions (XML)
    bld.install_files(bld.env['DATA_DIR'], 'gui/gui.glade')
    
    # 'Desktop' file (menu entry, icon, etc)
    #obj = bld.create_obj('subst')
    #obj.source = 'patchage.desktop.in'
    #obj.target = 'patchage.desktop'
    #obj.dict = {
    #    'BINDIR'           : bld.env()['BINDIR'],
    #    'APP_INSTALL_NAME' : bld.env()['APP_INSTALL_NAME'],
    #    'APP_HUMAN_NAME'   : bld.env()['APP_HUMAN_NAME'],
    #}
    #install_as(os.path.normpath(bld.env()['DATADIR'] + 'applications/'), bld.env()['APP_INSTALL_NAME'] + '.desktop', 'build/default/patchage.desktop')

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

    #icon_sizes = ['16x16', '22x22', '24x24', '32x32', '48x48']
    #for icon_size in icon_sizes:
    #    install_as(os.path.normpath(bld.env()['DATADIR'] + '/icons/hicolor/' + icon_size + '/apps/'), bld.env()['APP_INSTALL_NAME'] + '.png', 'icons/' + icon_size + '/patchage.png')

def dist_hook():
    pass

import commands
from Constants import RUN_ME
from TaskGen import feature, after
import Task, Utils

@feature('cc')
@after('apply_core')
def process_git(self):
    if getattr(self, 'ver_header', None):
        tsk = self.create_task('git_ver')
        tsk.set_outputs(self.path.find_or_declare(self.ver_header))

def git_ver(self):
    self.ver = "unknown"
    self.ver = commands.getoutput("LANG= git rev-parse HEAD").splitlines()[0]
    if commands.getoutput("LANG= git diff-index --name-only HEAD").splitlines():
        self.ver += "-dirty"

    Utils.pprint('BLUE', "git revision " + self.ver)

    fi = open(self.outputs[0].abspath(self.env), 'w')
    fi.write('#define GIT_VERSION "%s"\n' % self.ver)
    fi.close()

cls = Task.task_type_from_func('git_ver', vars=[], func=git_ver, color='BLUE', before='cc')

def always(self):
	return RUN_ME
cls.runnable_status = always

def post_run(self):
	sg = Utils.h_list(self.ver)
	node = self.outputs[0]
	variant = node.variant(self.env)
	self.generator.bld.node_sigs[variant][node.id] = sg
cls.post_run = post_run
