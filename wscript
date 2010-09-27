#! /usr/bin/env python
# encoding: utf-8

import os
import Options
import Utils
import shutil
import re
import misc

APPNAME='ladish'
VERSION='0.3-rc'
DBUS_NAME_BASE = 'org.ladish'
RELEASE = False

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
    opt.add_option('--debug', action='store_true', default=False, dest='debug', help="Build debuggable binaries")
    opt.add_option('--doxygen', action='store_true', default=False, help='Enable build of doxygen documentation')
    opt.add_option('--distnodeps', action='store_true', default=False, help="When creating distribution tarball, don't package git submodules")

def add_cflag(conf, flag):
    conf.env.append_unique('CXXFLAGS', flag)
    conf.env.append_unique('CCFLAGS', flag)

def add_linkflag(conf, flag):
    conf.env.append_unique('LINKFLAGS', flag)

def check_gcc_optimizations_enabled(flags):
    gcc_optimizations_enabled = False
    for flag in flags:
        if len(flag) < 2 or flag[0] != '-' or flag[1] != 'O':
            continue
        if len(flag) == 2:
            gcc_optimizations_enabled = True;
        else:
            gcc_optimizations_enabled = flag[2] != '0';
    return gcc_optimizations_enabled

def create_service_taskgen(bld, target, opath, binary):
    obj = bld.new_task_gen('subst')
    obj.source = os.path.join('daemon', 'dbus.service.in')
    obj.target = target
    obj.dict = {'dbus_object_path': opath,
                'daemon_bin_path': os.path.join(bld.env['PREFIX'], 'bin', binary)}
    obj.install_path = bld.env['DBUS_SERVICES_DIR'] + os.path.sep
    obj.fun = misc.subst_func

def configure(conf):
    conf.check_tool('compiler_cc')
    conf.check_tool('compiler_cxx')
    conf.check_tool('boost')
    #conf.check_tool('ParallelDebug')

    conf.check_cfg(
        package = 'jack',
        mandatory = True,
        errmsg = "not installed, see http://jackaudio.org/",
        args = '--cflags --libs')

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

    conf.env['LIBDIR'] = os.path.join(os.path.normpath(conf.env['PREFIX']), 'lib')

    conf.env['BUILD_LIBLASH'] = Options.options.enable_liblash
    conf.env['BUILD_DOXYGEN_DOCS'] = Options.options.doxygen

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
        atleast_version = '2.20.0',
        errmsg = "not installed, see http://www.gtk.org/",
        args = '--cflags --libs'):
        build_gui = False

    if build_gui and not conf.check_cfg(
        package = 'flowcanvas',
        mandatory = False,
        atleast_version = '0.6.4',
        errmsg = "not installed, see http://drobilla.net/software/flowcanvas/",
        args = '--cflags --libs'):
        build_gui = False

    if build_gui:
        # We need the boost headers package (e.g. libboost-dev)
        # shared_ptr.hpp and weak_ptr.hpp
        build_gui = conf.check_boost(errmsg="not found, see http://boost.org/")

    conf.env['BUILD_GLADISH'] = build_gui

    conf.env['BUILD_WERROR'] = not RELEASE
    if conf.env['BUILD_WERROR']:
        add_cflag(conf, '-Wall')
        add_cflag(conf, '-Werror')
        # for pre gcc-4.4, enable optimizations so use of uninitialized variables gets detected
        try:
            is_gcc = conf.env['CC_NAME'] == 'gcc'
            if is_gcc:
                gcc_ver = []
                for n in conf.env['CC_VERSION']:
                    gcc_ver.append(int(n))
                if gcc_ver[0] < 4 or gcc_ver[1] < 4:
                    #print "optimize force enable is required"
                    if not check_gcc_optimizations_enabled(conf.env['CCFLAGS']):
                        if Options.options.debug:
                            print "C optimization must be forced in order to enable -Wuninitialized"
                            print "However this will not be made because debug compilation is enabled"
                        else:
                            print "C optimization forced in order to enable -Wuninitialized"
                            conf.env.append_unique('CCFLAGS', "-O")
        except:
            pass

    conf.env['BUILD_DEBUG'] = Options.options.debug
    if conf.env['BUILD_DEBUG']:
        add_cflag(conf, '-g')
        add_cflag(conf, '-O0')
        add_linkflag(conf, '-g')

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
    display_msg(conf, 'Treat warnings as errors', yesno(conf.env['BUILD_WERROR']))
    display_msg(conf, 'Debuggable binaries', yesno(conf.env['BUILD_DEBUG']))
    display_msg(conf, 'Build doxygen documentation', yesno(conf.env['BUILD_DOXYGEN_DOCS']))

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

    display_msg(conf, 'C compiler flags', repr(conf.env['CCFLAGS']))
    display_msg(conf, 'C++ compiler flags', repr(conf.env['CXXFLAGS']))

    if not conf.env['BUILD_GLADISH']:
        display_msg(conf)
        display_line(conf,     "WARNING: The GUI frontend will not built", 'RED')

    display_msg(conf)

def build(bld):
    daemon = bld.new_task_gen('cc', 'program')
    daemon.target = 'ladishd'
    daemon.includes = "build/default" # XXX config.h version.h and other generated files
    daemon.uselib = 'DBUS-1 UUID EXPAT'
    daemon.ver_header = 'version.h'
    daemon.env.append_value("LINKFLAGS", ["-lutil", "-ldl", "-Wl,-E"])

    daemon.source = []

    for source in [
        'main.c',
        'loader.c',
        'log.c',
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
        'save.c',
        'load.c',
        'cmd_load_studio.c',
        'cmd_new_studio.c',
        'cmd_rename_studio.c',
        'cmd_save_studio.c',
        'cmd_start_studio.c',
        'cmd_stop_studio.c',
        'cmd_unload_studio.c',
        'cmd_new_app.c',
        'cmd_change_app_state.c',
        'cmd_remove_app.c',
        'cmd_create_room.c',
        'cmd_delete_room.c',
        'cmd_save_project.c',
        'cmd_unload_project.c',
        'cmd_load_project.c',
        'cmd_exit.c',
        'cqueue.c',
        'app_supervisor.c',
        'room.c',
        'room_save.c',
        'room_load.c',
        ]:
        daemon.source.append(os.path.join("daemon", source))

    for source in [
        'jack_proxy.c',
        'graph_proxy.c',
        'a2j_proxy.c',
        "jmcore_proxy.c",
        "notify_proxy.c",
        "conf_proxy.c",
        ]:
        daemon.source.append(os.path.join("proxies", source))

    for source in [
        'signal.c',
        'method.c',
        'error.c',
        'object_path.c',
        'interface.c',
        'helpers.c',
        ]:
        daemon.source.append(os.path.join("dbus", source))

    for source in [
        'time.c',
        'dirhelpers.c',
        'catdup.c',
        ]:
        daemon.source.append(os.path.join("common", source))

    # process dbus.service.in -> ladish.service
    create_service_taskgen(bld, DBUS_NAME_BASE + '.service', DBUS_NAME_BASE, daemon.target)

    #####################################################
    # jmcore
    jmcore = bld.new_task_gen('cc', 'program')
    jmcore.target = 'jmcore'
    jmcore.includes = "build/default" # XXX config.h version.h and other generated files
    jmcore.uselib = 'DBUS-1 JACK'
    jmcore.defines = ['LOG_OUTPUT_STDOUT']
    jmcore.source = ['jmcore.c']

    for source in [
        #'signal.c',
        'method.c',
        'error.c',
        'object_path.c',
        'interface.c',
        'helpers.c',
        ]:
        jmcore.source.append(os.path.join("dbus", source))

    create_service_taskgen(bld, DBUS_NAME_BASE + '.jmcore.service', DBUS_NAME_BASE + ".jmcore", jmcore.target)

    #####################################################
    # conf
    ladiconfd = bld.new_task_gen('cc', 'program')
    ladiconfd.target = 'ladiconfd'
    ladiconfd.includes = "build/default" # XXX config.h version.h and other generated files
    ladiconfd.uselib = 'DBUS-1'
    ladiconfd.defines = ['LOG_OUTPUT_STDOUT']
    ladiconfd.source = ['conf.c']

    for source in [
        'dirhelpers.c',
        'catdup.c',
        ]:
        ladiconfd.source.append(os.path.join("common", source))

    for source in [
        'signal.c',
        'method.c',
        'error.c',
        'object_path.c',
        'interface.c',
        'helpers.c',
        ]:
        ladiconfd.source.append(os.path.join("dbus", source))

    create_service_taskgen(bld, DBUS_NAME_BASE + '.conf.service', DBUS_NAME_BASE + ".conf", ladiconfd.target)

    #####################################################
    # liblash
    if bld.env['BUILD_LIBLASH']:
        liblash = bld.new_task_gen('cc', 'shlib')
        liblash.includes = "build/default" # XXX config.h version.h and other generated files
        liblash.uselib = 'DBUS-1'
        liblash.target = 'lash'
        liblash.vnum = "1.1.1"
        liblash.defines = ['LOG_OUTPUT_STDOUT']
        liblash.source = [os.path.join("lash_compat", "liblash", 'lash.c')]

        bld.install_files('${PREFIX}/include/lash', 'lash_compat/liblash/lash/*.h')

        # process lash-1.0.pc.in -> lash-1.0.pc
        obj = bld.new_task_gen('subst')
        obj.source = [os.path.join("lash_compat", 'lash-1.0.pc.in')]
        obj.target = 'lash-1.0.pc'
        obj.dict = {'prefix': bld.env['PREFIX'],
                    'exec_prefix': bld.env['PREFIX'],
                    'libdir': bld.env['LIBDIR'],
                    'includedir': os.path.normpath(bld.env['PREFIX'] + '/include'),
                    }
        obj.install_path = '${LIBDIR}/pkgconfig/'
        obj.fun = misc.subst_func

    #####################################################
    # pylash

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
        gladish.uselib = 'DBUS-1 DBUS-GLIB-1 FLOWCANVAS'

        gladish.source = []

        for source in [
            'main.c',
            'load_project_dialog.c',
            'save_project_dialog.c',
            'world_tree.c',
            'graph_view.c',
            'canvas.cpp',
            'graph_canvas.c',
            'gtk_builder.c',
            'ask_dialog.c',
            'create_room_dialog.c',
            'menu.c',
            'about.c',
            'dbus.c',
            'studio.c',
            'studio_list.c',
            'dialogs.c',
            'jack.c',
            'control.c',
            'pixbuf.c',
            'room.c',
            'statusbar.c',
            'action.c',
            'settings.c',
            ]:
            gladish.source.append(os.path.join("gui", source))

        for source in [
            'jack_proxy.c',
            'graph_proxy.c',
            'studio_proxy.c',
            'control_proxy.c',
            'app_supervisor_proxy.c',
            "room_proxy.c",
            "conf_proxy.c",
            ]:
            gladish.source.append(os.path.join("proxies", source))

        for source in [
            'method.c',
            'helpers.c',
            ]:
            gladish.source.append(os.path.join("dbus", source))

        for source in [
            'catdup.c',
            ]:
            gladish.source.append(os.path.join("common", source))

        # GtkBuilder UI definitions (XML)
        bld.install_files(bld.env['DATA_DIR'], 'gui/gladish.ui')
    
    bld.install_files('${PREFIX}/bin', 'ladish_control', chmod=0755)

    # 'Desktop' file (menu entry, icon, etc)
    bld.install_files('${PREFIX}/share/applications/', 'gui/gladish.desktop', chmod=0644)

    # Icons
    icon_sizes = ['16x16', '22x22', '24x24', '32x32', '48x48', '256x256']
    for icon_size in icon_sizes:
        bld.path.ant_glob('art/' + icon_size + '/apps/*.png')
        bld.install_files('${PREFIX}/share/icons/hicolor/' + icon_size + '/apps/', 'art/' + icon_size + '/apps/gladish.png')

    status_images = []
    for status in ["down", "unloaded", "started", "stopped", "warning", "error"]:
        status_images.append("art/status_" + status + ".png")

    bld.install_files(bld.env['DATA_DIR'], status_images)
    bld.install_files(bld.env['DATA_DIR'], "art/ladish-logo-128x128.png")
    bld.install_files(bld.env['DATA_DIR'], ["COPYING", "AUTHORS", "README", "NEWS"])

    if bld.env['BUILD_DOXYGEN_DOCS'] == True:
        html_docs_source_dir = "build/default/html"
        if Options.commands['clean']:
            if os.access(html_docs_source_dir, os.R_OK):
                Utils.pprint('CYAN', "Removing doxygen generated documentation...")
                shutil.rmtree(html_docs_source_dir)
                Utils.pprint('CYAN', "Removing doxygen generated documentation done.")
        elif Options.commands['build']:
            if not os.access(html_docs_source_dir, os.R_OK):
                os.popen("doxygen").read()
            else:
                Utils.pprint('CYAN', "doxygen documentation already built.")

def get_tags_dirs():
    source_root = os.path.dirname(Utils.g_module.root_path)
    if 'relpath' in os.path.__all__:
        source_root = os.path.relpath(source_root)
    paths = source_root
    paths += " " + os.path.join(source_root, "common")
    paths += " " + os.path.join(source_root, "dbus")
    paths += " " + os.path.join(source_root, "proxies")
    paths += " " + os.path.join(source_root, "daemon")
    paths += " " + os.path.join(source_root, "gui")
    paths += " " + os.path.join(source_root, "example-apps")
    paths += " " + os.path.join(source_root, "lib")
    paths += " " + os.path.join(source_root, "lash_compat", "liblash")
    paths += " " + os.path.join(source_root, "lash_compat", "liblash", "lash")
    return paths

def gtags(ctx):
    '''build tag files for GNU global'''
    cmd = "find %s -mindepth 1 -maxdepth 1 -name '*.[ch]' -print | gtags --statistics -f -" % get_tags_dirs()
    #print("Running: %s" % cmd)
    os.system(cmd)

def etags(ctx):
    '''build TAGS file using etags'''
    cmd = "find %s -mindepth 1 -maxdepth 1 -name '*.[ch]' -print | etags -" % get_tags_dirs()
    #print("Running: %s" % cmd)
    os.system(cmd)
    os.system("stat -c '%y' TAGS")

def dist_hook():
    #print repr(Options.options)
    if Options.options.distnodeps:
        shutil.rmtree('laditools')
        shutil.rmtree('flowcanvas')
        shutil.rmtree('jack2')
        shutil.rmtree('a2jmidid')
    nodist_files = ['.gitmodules', 'GTAGS', 'GRTAGS', 'GPATH', 'GSYMS'] # waf does not ignore these file
    for nodist_file in nodist_files:
        os.remove(nodist_file)
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

        # needs some help with implicit dependencies
        # http://code.google.com/p/waf/issues/detail?id=732
        tg = self.bld.name_to_obj('ladishd', self.env)
        if id(tg) != id(self):
            tg.post()
        for x in tg.tasks:
            if x.inputs and x.inputs[0].name == 'main.c' and x.inputs[0].parent.name == 'daemon':
                x.set_run_after(tsk)

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

cls = Task.task_type_from_func('git_ver', vars=[], func=git_ver, color='BLUE')

def always(self):
    return RUN_ME
cls.runnable_status = always

def post_run(self):
    sg = Utils.h_list(self.ver)
    node = self.outputs[0]
    variant = node.variant(self.env)
    self.generator.bld.node_sigs[variant][node.id] = sg
cls.post_run = post_run

import Runner

old_refill = Runner.Parallel.refill_task_list
def refill_task_list(self):
    old_refill(self)
    self.outstanding.sort(cmp=lambda a, b: cmp(b.__class__.__name__, a.__class__.__name__))
Runner.Parallel.refill_task_list = refill_task_list
