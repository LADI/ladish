#! /usr/bin/env python
# encoding: utf-8

import os
import Options
import Utils

APPNAME='ladish'
VERSION='1'

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
    opt.add_option('--enable-pkg-config-dbus-service-dir', action='store_true', default=False, help='force D-Bus service install dir to be one returned by pkg-config')

def configure(conf):
    conf.check_tool('compiler_cc')

    conf.check_cfg(
        package = 'dbus-1',
        atleast_version = '1.0.0',
        mandatory = True,
        errmsg = "not installed, see http://dbus.freedesktop.org/",
        args='--cflags --libs')

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
        args='--cflags --libs')

    conf.check_cfg(
        package = 'libxml-2.0',
        atleast_version = '2.0.0',
        mandatory = True,
        errmsg = "not installed, see http://xmlsoft.org/",
        args='--cflags --libs')

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
    daemon.env.append_value("LINKFLAGS", ["-lutil"])
    daemon.source = []
    for source in [
        'main.c',
        'server.c',
        'loader.c',
        'log.c',
        'sigsegv.c',
        'proctitle.c',
        'project.c',
        'appdb.c',
        'client.c',
        'store.c',
        'procfs.c',
        'jack_patch.c',
        'file.c',
        'dbus_service.c',
        'jackdbus_mgr.c',
        'dbus_iface_control.c',
        'dbus_iface_server.c',
        'client_dependency.c',
        'jack_mgr_client.c',
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
        ]:
        daemon.source.append(os.path.join("dbus", source))

    daemon.source.append(os.path.join("common", "safety.c"))

    # process org.jackaudio.service.in -> org.jackaudio.service
    #import misc
    #obj = bld.new_task_gen('subst')
    #obj.source = 'org.jackaudio.service.in'
    #obj.target = 'org.jackaudio.service'
    #obj.dict = {'BINDIR': bld.env['PREFIX'] + '/bin'}
    #obj.install_path = '${DBUS_SERVICES_DIR}/'
    #obj.fun = misc.subst_func

    liblash = bld.new_task_gen('cc', 'shlib')
    liblash.includes = "build/default" # XXX config.h version.h and other generated files
    liblash.uselib = 'DBUS-1 LIBXML-2.0 UUID'
    liblash.target = 'lash'
    liblash.defines = 'LASH_OLD_API'
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
    liblash.source.append(os.path.join("daemon", "file.c"))

    # pkgpyexec_LTLIBRARIES = _lash.la
    # INCLUDES = $(PYTHON_INCLUDES)
    # _lash_la_LDFLAGS = -module -avoid-version ../liblash/liblash.la
    # _lash_la_SOURCES = lash.c lash.h lash_wrap.c
    # pkgpyexec_SCRIPTS = lash.py
    # CLEANFILES = lash_wrap.c lash.py lash.pyc zynjacku.defs
    # EXTRA_DIST = test.py lash.i lash.py
    # lash_wrap.c lash.py: lash.i lash.h
    # 	swig -o lash_wrap.c -I$(top_srcdir) -python $(top_srcdir)/$(subdir)/lash.i

def dist_hook():
    pass
