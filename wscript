#! /usr/bin/env python
# encoding: utf-8

import os
import Options
import Utils
import shutil
import re

APPNAME='ladish'
VERSION='0.2'
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

def yesno(bool):
    if bool:
        return "yes"
    else:
        return "no"

def set_options(opt):
    opt.tool_options('compiler_cc')
    opt.tool_options('compiler_cxx')
    opt.tool_options('boost')
    opt.add_option('--enable-pkg-config-dbus-service-dir', action='store_true', default=False, help='force D-Bus service install dir to be one returned by pkg-config')
    opt.add_option('--enable-liblash', action='store_true', default=False, help='Build LASH compatibility library')

def add_cflag(conf, flag):
    conf.env.append_unique('CXXFLAGS', flag)
    conf.env.append_unique('CCFLAGS', flag)

def configure(conf):
    conf.check_tool('compiler_cc')
    conf.check_tool('compiler_cxx')
    conf.check_tool('boost')

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

    conf.env['BUILD_LIBLASH'] = Options.options.enable_liblash

    conf.check_cfg(
        package = 'uuid',
        mandatory = True,
        errmsg = "not installed, see http://e2fsprogs.sourceforge.net/",
        args = '--cflags --libs')

    conf.check(
        header_name='expat.h',
        mandatory = True,
        errmsg = "not installed, see http://expat.sourceforge.net/")

    conf.env['LIB_EXPAT'] = ['expat']

    build_gui = True

    if build_gui and not conf.check_cfg(
        package = 'glib-2.0',
        mandatory = False,
        errmsg = "not installed, see http://www.gtk.org/",
        args = '--cflags --libs'):
        build_gui = False

    if build_gui and not conf.check_cfg(
        package = 'dbus-glib-1',
        mandatory = False,
        errmsg = "not installed, see http://dbus.freedesktop.org/",
        args = '--cflags --libs'):
        build_gui = False

    if build_gui and not conf.check_cfg(
        package = 'gtk+-2.0',
        mandatory = False,
        errmsg = "not installed, see http://www.gtk.org/",
        args = '--cflags --libs'):
        build_gui = False

    if build_gui and not conf.check_cfg(
        package = 'libglade-2.0',
        mandatory = False,
        errmsg = "not installed, see http://ftp.gnome.org/pub/GNOME/sources/libglade/",
        args = '--cflags --libs'):
        build_gui = False

    if build_gui and not conf.check_cfg(
        package = 'flowcanvas',
        mandatory = False,
        atleast_version = '0.5.3',
        errmsg = "not installed, see http://drobilla.net/software/flowcanvas/",
        args = '--cflags --libs'):
        build_gui = False

    if build_gui:
        # We need the boost headers package (e.g. libboost-dev)
        # shared_ptr.hpp and weak_ptr.hpp
        build_gui = conf.check_boost(errmsg="not found, see http://boost.org/")

    conf.env['BUILD_GLADISH'] = build_gui

    add_cflag(conf, '-g')
    add_cflag(conf, '-Wall')
    add_cflag(conf, '-Werror')

    conf.define('DATA_DIR', os.path.normpath(os.path.join(conf.env['PREFIX'], 'share', APPNAME)))
    conf.define('PACKAGE_VERSION', VERSION)
    conf.define('DBUS_NAME_BASE', DBUS_NAME_BASE)
    conf.define('DBUS_BASE_PATH', '/' + DBUS_NAME_BASE.replace('.', '/'))
    conf.define('BASE_NAME', APPNAME)
    conf.define('_GNU_SOURCE', 1)
    conf.write_config_header('config.h')

    display_msg(conf)

    display_msg(conf, "==================")
    version_msg = APPNAME + "-" + VERSION

    if os.access('version.h', os.R_OK):
        data = file('version.h').read()
        m = re.match(r'^#define GIT_VERSION "([^"]*)"$', data)
        if m != None:
            version_msg += " exported from " + m.group(1)
    elif os.access('.git', os.R_OK):
        version_msg += " git revision will checked and eventually updated during build"

    display_msg(conf, version_msg)

    display_msg(conf)
    display_msg(conf, "Install prefix", conf.env['PREFIX'], 'CYAN')

    display_msg(conf, 'Build gladish', yesno(conf.env['BUILD_GLADISH']))
    display_msg(conf, 'Build liblash', yesno(Options.options.enable_liblash))

    if conf.env['DBUS_SERVICES_DIR'] != conf.env['DBUS_SERVICES_DIR_REAL']:
        display_msg(conf)
        display_line(conf,     "WARNING: D-Bus session services directory as reported by pkg-config is", 'RED')
        display_raw_text(conf, "WARNING:", 'RED')
        display_line(conf,      conf.env['DBUS_SERVICES_DIR_REAL'], 'CYAN')
        display_line(conf,     'WARNING: but service file will be installed in', 'RED')
        display_raw_text(conf, "WARNING:", 'RED')
        display_line(conf,      conf.env['DBUS_SERVICES_DIR'], 'CYAN')
        display_line(conf,     'WARNING: You may need to adjust your D-Bus configuration after installing ladish', 'RED')
        display_line(conf,     'WARNING: You can override dbus service install directory', 'RED')
        display_line(conf,     'WARNING: with --enable-pkg-config-dbus-service-dir option to this script', 'RED')

    display_msg(conf)

def build(bld):
    daemon = bld.new_task_gen('cc', 'program')
    daemon.target = 'ladishd'
    daemon.includes = "build/default" # XXX config.h version.h and other generated files
    daemon.uselib = 'DBUS-1 UUID EXPAT'
    daemon.ver_header = 'version.h'
    daemon.env.append_value("LINKFLAGS", ["-lutil", "-ldl", "-Wl,-E"])

    daemon.source = [
        'jack_proxy.c',
        'graph_proxy.c',
        'catdup.c',
        ]

    for source in [
        'main.c',
        'loader.c',
        'log.c',
        'dirhelpers.c',
        'sigsegv.c',
        'proctitle.c',
        'appdb.c',
        'procfs.c',
        'control.c',
        'studio.c',
        'graph.c',
        'client.c',
        'port.c',
        'virtualizer.c',
        'dict.c',
        'graph_dict.c',
        'escape.c',
        'studio_jack_conf.c',
        'cmd_load_studio.c',
        'cmd_new_studio.c',
        'cmd_rename_studio.c',
        'cmd_save_studio.c',
        'cmd_start_studio.c',
        'cmd_stop_studio.c',
        'cmd_unload_studio.c',
        'cmd_exit.c',
        'cqueue.c',
        'app_supervisor.c',
        ]:
        daemon.source.append(os.path.join("daemon", source))

    for source in [
        'signal.c',
        'method.c',
        'error.c',
        'object_path.c',
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

    if bld.env['BUILD_LIBLASH']:
        liblash = bld.new_task_gen('cc', 'shlib')
        liblash.includes = "build/default" # XXX config.h version.h and other generated files
        liblash.uselib = 'DBUS-1'
        liblash.target = 'lash'
        liblash.vnum = "1.1.1"
        liblash.defines = ['LOG_OUTPUT_STDOUT']
        liblash.source = [os.path.join("lash_compat", "liblash", 'lash.c')]

    # pkgpyexec_LTLIBRARIES = _lash.la
    # INCLUDES = $(PYTHON_INCLUDES)
    # _lash_la_LDFLAGS = -module -avoid-version ../liblash/liblash.la
    # _lash_la_SOURCES = lash.c lash.h lash_wrap.c
    # pkgpyexec_SCRIPTS = lash.py
    # CLEANFILES = lash_wrap.c lash.py lash.pyc zynjacku.defs
    # EXTRA_DIST = test.py lash.i lash.py
    # lash_wrap.c lash.py: lash.i lash.h
    #   swig -o lash_wrap.c -I$(top_srcdir) -python $(top_srcdir)/$(subdir)/lash.i

    #####################################################
    # gladish
    if bld.env['BUILD_GLADISH']:
        gladish = bld.new_task_gen('cxx', 'program')
        gladish.features.append('cc')
        gladish.target = 'gladish'
        gladish.defines = ['LOG_OUTPUT_STDOUT']
        gladish.includes = "build/default" # XXX config.h version.h and other generated files
        gladish.uselib = 'DBUS-1 DBUS-GLIB-1 LIBGLADE-2.0 FLOWCANVAS'

        gladish.source = [
            'jack_proxy.c',
            'graph_proxy.c',
            'studio_proxy.c',
            'catdup.c',
            ]

        for source in [
            'main.c',
            #'lash_client.cpp',
            #'lash_proxy.cpp',
            #'load_projects_dialog.cpp',
            #'project.cpp',
            'world_tree.c',
            'graph_view.c',
            #'project_properties.cpp',
            #'session.cpp',
            'a2j_proxy.c',
            'dbus_helpers.c',
            'canvas.cpp',
            'graph_canvas.c',
            'glade.c',
            'control_proxy.c',
            'app_supervisor_proxy.c',
            'ask_dialog.c',
            ]:
            gladish.source.append(os.path.join("gui", source))

        for source in [
            'method.c',
            'helpers.c',
            ]:
            gladish.source.append(os.path.join("dbus", source))

        # Glade UI definitions (XML)
        bld.install_files(bld.env['DATA_DIR'], 'gui/gui.glade')
    
    bld.install_files('${PREFIX}/bin', 'ladish_control', chmod=0755)

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
    shutil.copy('../build/default/version.h', "./")

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
    header = self.outputs[0].abspath(self.env)
    if os.access('../version.h', os.R_OK):
        shutil.copy('../version.h', header)
        data = file(header).read()
        m = re.match(r'^#define GIT_VERSION "([^"]*)"$', data)
        if m != None:
            self.ver = m.group(1)
            Utils.pprint('BLUE', "tarball from git revision " + self.ver)
        else:
            self.ver = "tarball"
        return

    if os.access('../.git', os.R_OK):
        self.ver = commands.getoutput("LANG= git rev-parse HEAD").splitlines()[0]
        if commands.getoutput("LANG= git diff-index --name-only HEAD").splitlines():
            self.ver += "-dirty"

        Utils.pprint('BLUE', "git revision " + self.ver)
    else:
        self.ver = "unknown"

    fi = open(header, 'w')
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
