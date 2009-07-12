#! /usr/bin/env python
# encoding: utf-8

import os
import Options
import Utils

APPNAME='ladish'
VERSION='1'
DBUS_NAME_BASE = 'org.nongnu.LASH'

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

    conf.define('DEFAULT_PROJECT_DIR', "audio-projects")
    conf.define('PACKAGE_VERSION', VERSION)
    conf.define('DBUS_NAME_BASE', DBUS_NAME_BASE)
    conf.define('DBUS_BASE_PATH', '/' + DBUS_NAME_BASE.replace('.', '/'))
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
