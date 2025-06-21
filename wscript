#! /usr/bin/env python
# encoding: utf-8

from __future__ import with_statement


parallel_debug = False

APPNAME='ladish'
VERSION='2.1'
DBUS_NAME_BASE = 'org.ladish'
RELEASE = False

# these variables are mandatory ('/' are converted automatically)
top = '.'
out = 'build'

import os, sys, re, io, optparse, shutil, tokenize
from hashlib import md5

from waflib import Errors, Utils, Options, Logs, Scripting
from waflib import Configure
from waflib import Context

def display_msg(conf, msg="", status = None, color = None):
    if status:
        conf.msg(msg, status, color)
    else:
        Logs.pprint('NORMAL', msg)

def display_raw_text(conf, text, color = 'NORMAL'):
    Logs.pprint(color, text, sep = '')

def display_line(conf, text, color = 'NORMAL'):
    Logs.pprint(color, text, sep = os.linesep)

def yesno(bool):
    if bool:
        return "yes"
    else:
        return "no"

def options(opt):
    opt.load('compiler_c')
    opt.load('compiler_cxx')
    opt.load('python')
    opt.add_option('--enable-pkg-config-dbus-service-dir', action='store_true', default=False, help='force D-Bus service install dir to be one returned by pkg-config')
    opt.add_option('--disable-ladishd', action='store_true', default=False, help='Do not build ladishd')
    opt.add_option('--disable-ladiconfd', action='store_true', default=False, help='Do not build ladiconfd')
    opt.add_option('--disable-alsapid', action='store_true', default=False, help='Do not build alsapid')
    opt.add_option('--disable-jmcore', action='store_true', default=False, help='Do not build jmcore (JACK multicore)')
    opt.add_option('--enable-gladish', action='store_true', default=False, help='Build gladish')
    opt.add_option('--enable-liblash', action='store_true', default=False, help='Build LASH compatibility library')
    opt.add_option('--debug', action='store_true', default=False, dest='debug', help="Build debuggable binaries")
    opt.add_option('--siginfo', action='store_true', default=False, dest='siginfo', help="Log backtrace on fatal signal")
    opt.add_option('--doxygen', action='store_true', default=False, help='Enable build of doxygen documentation')
    opt.add_option('--distnodeps', action='store_true', default=False, help="When creating distribution tarball, don't package git submodules")
    opt.add_option('--distname', type=str, default=None, help="Name for the distribution tarball")
    opt.add_option('--distsuffix', type=str, default="", help="String to append to the distribution tarball name")
    opt.add_option('--tagdist', action='store_true', default=False, help='Create of git tag for distname')
    opt.add_option('--libdir', type=str, default=None, help='Define lib dir')
    opt.add_option('--docdir', type=str, default=None, help="Define doc dir [default: PREFIX'/share/doc/" + APPNAME + ']')


    if parallel_debug:
        opt.load('parallel_debug')

def add_cflag(conf, flag):
    conf.env.prepend_value('CFLAGS', flag)

def add_cxxflag(conf, flag):
    conf.env.prepend_value('CXXFLAGS', flag)

def add_candcxxflag(conf, flag):
    add_cflag(conf, flag)
    add_cxxflag(conf, flag)

def add_linkflag(conf, flag):
    conf.env.prepend_value('LINKFLAGS', flag)

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
    bld(
        features     = 'subst', # the feature 'subst' overrides the source/target processing
        source       = os.path.join('daemon', 'dbus.service.in'), # list of string or nodes
        target       = target,  # list of strings or nodes
        install_path = bld.env['DBUS_SERVICES_DIR'] + os.path.sep,
        # variables to use in the substitution
        dbus_object_path = opath,
        daemon_bin_path  = os.path.join(bld.env['PREFIX'], 'bin', binary))

def configure(conf):
    conf.load('compiler_c')
    conf.load('compiler_cxx')
    conf.load('python')
    conf.load('intltool')
    if parallel_debug:
        conf.load('parallel_debug')

    # dladdr() is used by daemon/siginfo.c
    # dlvsym() is used by the alsapid library
    conf.check_cc(msg="Checking for libdl", lib=['dl'], uselib_store='DL')

    # forkpty() is used by ladishd
    conf.check_cc(msg="Checking for libutil", lib=['util'], uselib_store='UTIL')

    conf.check_cfg(
        package = 'jack',
        mandatory = True,
        errmsg = "not installed, see http://jackaudio.org/",
        args = '--cflags --libs')

    conf.check_cfg(
        package = 'alsa',
        mandatory = True,
        errmsg = "not installed, see http://www.alsa-project.org/",
        args = '--cflags')

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
        package = 'cdbus-1',
        atleast_version = '1.0.0',
        mandatory = True,
        errmsg = "not installed, see https://github.com/LADI/cdbus",
        args = '--cflags --libs')

    if Options.options.libdir:
        conf.env['LIBDIR'] = Options.options.libdir
    else:
        conf.env['LIBDIR'] = os.path.join(os.path.normpath(conf.env['PREFIX']), 'lib')

    if Options.options.docdir:
        conf.env['DOCDIR'] = Options.options.docdir
    else:
        conf.env['DOCDIR'] = os.path.join(os.path.normpath(conf.env['PREFIX']), 'share', 'doc', APPNAME)

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

    if Options.options.enable_gladish:
        conf.check_cfg(
            package = 'glib-2.0',
            errmsg = "not installed, see http://www.gtk.org/",
            args = '--cflags --libs')
        conf.check_cfg(
            package = 'dbus-glib-1',
            errmsg = "not installed, see http://dbus.freedesktop.org/",
            args = '--cflags --libs')
        conf.check_cfg(
            package = 'gtk+-2.0',
            atleast_version = '2.20.0',
            errmsg = "not installed, see http://www.gtk.org/",
            args = '--cflags --libs')
        conf.check_cfg(
            package = 'gtkmm-2.4',
            atleast_version = '2.10.0',
            errmsg = "not installed, see http://www.gtkmm.org",
            args = '--cflags --libs')

        conf.check_cfg(
            package = 'libgnomecanvasmm-2.6',
            atleast_version = '2.6.0',
            errmsg = "not installed, see http://www.gtkmm.org",
            args = '--cflags --libs')

        #autowaf.check_pkg(conf, 'libgvc', uselib_store='AGRAPH', atleast_version='2.8', mandatory=False)

        # The boost headers package (e.g. libboost-dev) is needed
        conf.multicheck(
            {'header_name':  "boost/shared_ptr.hpp"},
            {'header_name':  "boost/weak_ptr.hpp"},
            {'header_name':  "boost/enable_shared_from_this.hpp"},
            {'header_name':  "boost/utility.hpp"},
            msg = "Checking for boost headers",
            mandatory = False)
        display_msg(conf, 'Found boost/shared_ptr.hpp',
                    yesno(conf.env['HAVE_BOOST_SHARED_PTR_HPP']))
        display_msg(conf, 'Found boost/weak_ptr.hpp',
                    yesno(conf.env['HAVE_BOOST_WEAK_PTR_HPP']))
        display_msg(conf, 'Found boost/enable_shared_from_this.hpp',
                    yesno(conf.env['HAVE_BOOST_ENABLE_SHARED_FROM_THIS_HPP']))
        display_msg(conf, 'Found boost/utility.hpp',
                    yesno(conf.env['HAVE_BOOST_UTILITY_HPP']))
        if not (conf.env['HAVE_BOOST_SHARED_PTR_HPP'] and \
                conf.env['HAVE_BOOST_WEAK_PTR_HPP'] and \
                conf.env['HAVE_BOOST_ENABLE_SHARED_FROM_THIS_HPP'] and \
                conf.env['HAVE_BOOST_UTILITY_HPP']):
            display_line(conf, "One or more boost headers not found, see http://boost.org/")
            sys.exit(1)

    conf.env['BUILD_LADISHD'] = not Options.options.disable_ladishd
    conf.env['BUILD_LADICONFD'] = not Options.options.disable_ladiconfd
    conf.env['BUILD_ALSAPID'] = not Options.options.disable_alsapid
    conf.env['BUILD_JMCORE'] = not Options.options.disable_jmcore
    conf.env['BUILD_GLADISH'] = Options.options.enable_gladish
    conf.env['BUILD_LIBLASH'] = Options.options.enable_liblash
    conf.env['BUILD_SIGINFO'] =  Options.options.siginfo

    add_cflag(conf, '-std=c23')
    add_cxxflag(conf, '-std=c++23')

    add_candcxxflag(conf, '-D_DEFAULT_SOURCE')
#    add_candcxxflag(conf, '-D_GNU_SOURCE')
#    add_candcxxflag(conf, '-D_POSIX_C_SOURCE=200112L')
    add_candcxxflag(conf, '-D_POSIX_C_SOURCE=200809L')
#    add_candcxxflag(conf, '-D_XOPEN_SOURCE=500')

    add_candcxxflag(conf, '-fvisibility=hidden')

    conf.env['BUILD_WERROR'] = False #not RELEASE
    add_cflag(conf, '-Wall')

    if conf.env['BUILD_GLADISH']:
        add_cxxflag(conf, '-Wno-deprecated-copy')

    add_candcxxflag(conf, '-Wimplicit-fallthrough=2')

    if conf.env['BUILD_WERROR']:
        add_candcxxflag(conf, '-Werror')
        # for pre gcc-4.4, enable optimizations so use of uninitialized variables gets detected
        try:
            is_gcc = conf.env['CC_NAME'] == 'gcc'
            if is_gcc:
                gcc_ver = []
                for n in conf.env['CC_VERSION']:
                    gcc_ver.append(int(n))
                if gcc_ver[0] < 4 or gcc_ver[1] < 4:
                    #print "optimize force enable is required"
                    if not check_gcc_optimizations_enabled(conf.env['CFLAGS']):
                        if Options.options.debug:
                            print ("C optimization must be forced in order to enable -Wuninitialized")
                            print ("However this will not be made because debug compilation is enabled")
                        else:
                            print ("C optimization forced in order to enable -Wuninitialized")
                            conf.env.append_unique('CFLAGS', "-O")
        except:
            pass

    conf.env['BUILD_DEBUG'] = Options.options.debug
    if conf.env['BUILD_DEBUG']:
        add_candcxxflag(conf, '-g')
        add_candcxxflag(conf, '-O0')
        add_linkflag(conf, '-g')

    conf.env['DATA_DIR'] = os.path.normpath(os.path.join(conf.env['PREFIX'], 'share', APPNAME))
    conf.env['LOCALE_DIR'] = os.path.normpath(os.path.join(conf.env['PREFIX'], 'share', 'locale'))

    # write some parts of the configure environment to the config.h file
    conf.define('DATA_DIR', conf.env['DATA_DIR'])
    conf.define('LOCALE_DIR', conf.env['LOCALE_DIR'])
    conf.define('PACKAGE_VERSION', VERSION)
    conf.define('DBUS_NAME_BASE', DBUS_NAME_BASE)
    conf.define('DBUS_BASE_PATH', '/' + DBUS_NAME_BASE.replace('.', '/'))
    conf.define('BASE_NAME', APPNAME)
    conf.define('SIGINFO_ENABLED', conf.env['BUILD_SIGINFO'])
    conf.define('_GNU_SOURCE', 1)
    conf.write_config_header('config.h')

    display_msg(conf)

    display_msg(conf, "==================")
    version_msg = APPNAME + "-" + VERSION

    if os.access('version.h', os.R_OK):
        data = open('version.h').read()
        m = re.match(r'^#define GIT_VERSION "([^"]*)"$', data)
        if m != None:
            version_msg += " exported from " + m.group(1)
    elif os.access('.git', os.R_OK):
        version_msg += " git revision will checked and eventually updated during build"

    display_msg(conf, version_msg)

    display_msg(conf)
    display_msg(conf, "Install prefix", conf.env['PREFIX'], 'CYAN')

    display_msg(conf, 'Build ladishd', yesno(conf.env['BUILD_LADISHD']))
    display_msg(conf, 'Build ladiconfd', yesno(conf.env['BUILD_LADICONFD']))
    display_msg(conf, 'Build alsapid', yesno(conf.env['BUILD_ALSAPID']))
    display_msg(conf, 'Build jmcore', yesno(conf.env['BUILD_JMCORE']))
    display_msg(conf, 'Build gladish', yesno(conf.env['BUILD_GLADISH']))
    display_msg(conf, 'Build liblash', yesno(Options.options.enable_liblash))
    display_msg(conf, 'Build with siginfo', yesno(conf.env['BUILD_SIGINFO']))
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

    display_msg(conf, 'C compiler flags', repr(conf.env['CFLAGS']))
    display_msg(conf, 'C++ compiler flags', repr(conf.env['CXXFLAGS']))
    display_msg(conf, 'Linker flags', repr(conf.env['LINKFLAGS']))

    if not conf.env['BUILD_GLADISH']:
        display_msg(conf)
        display_line(conf,     "WARNING: The GUI frontend will not built", 'RED')

    display_msg(conf)

def git_ver(self):
    bld = self.generator.bld
    header = self.outputs[0].abspath()
    if os.access('./version.h', os.R_OK):
        header = os.path.join(os.getcwd(), out, "version.h")
        shutil.copy('./version.h', header)
        data = open(header).read()
        m = re.match(r'^#define GIT_VERSION "([^"]*)"$', data)
        if m != None:
            self.ver = m.group(1)
            Logs.pprint('BLUE', "tarball from git revision " + self.ver)
        else:
            self.ver = "tarball"
        return

    if bld.srcnode.find_node('.git'):
        self.ver = bld.cmd_and_log("LANG= git rev-parse HEAD", quiet=Context.BOTH).splitlines()[0]
        if bld.cmd_and_log("LANG= git diff-index --name-only HEAD", quiet=Context.BOTH).splitlines():
            self.ver += "-dirty"

        Logs.pprint('BLUE', "git revision " + self.ver)
    else:
        self.ver = "unknown"

    fi = open(header, 'w')
    fi.write('#define GIT_VERSION "%s"\n' % self.ver)
    fi.close()

def build(bld):
    if not bld.env['DATA_DIR']:
        raise "DATA_DIR is emtpy"

    bld(rule=git_ver, target='version.h', update_outputs=True, always=True, ext_out=['.h'])

    if bld.env['BUILD_LADISHD']:
        daemon = bld.program(source = [], features = 'c cprogram', includes = [bld.path.get_bld()])
        daemon.target = 'ladishd'
        daemon.uselib = 'DBUS-1 CDBUS-1 UUID EXPAT DL UTIL'
        daemon.ver_header = 'version.h'
        # Make backtrace function lookup to work for functions in the executable itself
        daemon.env.append_value("LINKFLAGS", ["-Wl,-E"])
        daemon.defines = ["HAVE_CONFIG_H"]

        daemon.source = ["string_constants.c"]

        for source in [
                'main.c',
                'loader.c',
                'proctitle.c',
                'procfs.c',
                'control.c',
                'studio.c',
                'graph.c',
                'graph_manager.c',
                'client.c',
                'port.c',
                'virtualizer.c',
                'dict.c',
                'graph_dict.c',
                'escape.c',
                'studio_jack_conf.c',
                'studio_list.c',
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
                'recent_store.c',
                'recent_projects.c',
                'check_integrity.c',
                'lash_server.c',
                'jack_session.c',
        ]: daemon.source.append(os.path.join("daemon", source))

        if Options.options.siginfo:
            daemon.source.append(os.path.join("daemon", 'siginfo.c'))

        for source in [
                'jack_proxy.c',
                'graph_proxy.c',
                'a2j_proxy.c',
                "jmcore_proxy.c",
                "notify_proxy.c",
                "conf_proxy.c",
                "lash_client_proxy.c",
        ]: daemon.source.append(os.path.join("proxies", source))

        for source in [
                'log.c',
                'time.c',
                'dirhelpers.c',
                'catdup.c',
        ]: daemon.source.append(os.path.join("common", source))

        daemon.source.append(os.path.join("alsapid", "helper.c"))

        # process dbus.service.in -> ladish.service
        create_service_taskgen(bld, DBUS_NAME_BASE + '.service', DBUS_NAME_BASE, daemon.target)

    #####################################################
    # jmcore
    if bld.env['BUILD_JMCORE']:
        jmcore = bld.program(source = [], features = 'c cprogram', includes = [bld.path.get_bld()])
        jmcore.target = 'jmcore'
        jmcore.uselib = 'DBUS-1 CDBUS-1 JACK'
        jmcore.defines = ['LOG_OUTPUT_STDOUT']
        jmcore.source = ['jmcore.c']

        for source in [
                'log.c',
        ]: jmcore.source.append(os.path.join("common", source))

        create_service_taskgen(bld, DBUS_NAME_BASE + '.jmcore.service', DBUS_NAME_BASE + ".jmcore", jmcore.target)

    #####################################################
    # conf
    if bld.env['BUILD_LADICONFD']:
        ladiconfd = bld.program(source = [], features = 'c cprogram', includes = [bld.path.get_bld()])
        ladiconfd.target = 'ladiconfd'
        ladiconfd.uselib = 'DBUS-1 CDBUS-1'
        ladiconfd.defines = ['LOG_OUTPUT_STDOUT']
        ladiconfd.source = ['conf.c']

        for source in [
                'log.c',
                'dirhelpers.c',
                'catdup.c',
        ]: ladiconfd.source.append(os.path.join("common", source))

        create_service_taskgen(bld, DBUS_NAME_BASE + '.conf.service', DBUS_NAME_BASE + ".conf", ladiconfd.target)

    #####################################################
    # alsapid
    if bld.env['BUILD_ALSAPID']:
        alsapid = bld.shlib(source = [], features = 'c cshlib', includes = [bld.path.get_bld()])
        alsapid.uselib = 'DL'
        alsapid.target = 'alsapid'
        for source in [
                'lib.c',
                'helper.c',
        ]: alsapid.source.append(os.path.join("alsapid", source))

    #####################################################
    # liblash
    if bld.env['BUILD_LIBLASH']:
        liblash = bld.shlib(source = [], features = 'c cshlib', includes = [bld.path.get_bld()])
        liblash.uselib = 'DBUS-1 CDBUS-1'
        liblash.target = 'lash'
        liblash.vnum = "1.1.1"
        liblash.defines = ['LOG_OUTPUT_STDOUT']
        liblash.source = [os.path.join("lash_compat", "liblash", 'lash.c')]

        for source in [
            'dirhelpers.c',
            'catdup.c',
            'file.c',
            'log.c',
            ]:
            liblash.source.append(os.path.join("common", source))

        bld.install_files('${PREFIX}/include/lash-1.0/lash', bld.path.ant_glob('lash_compat/liblash/lash/*.h'))

        # process lash-1.0.pc.in -> lash-1.0.pc
        bld(
            features     = 'subst', # the feature 'subst' overrides the source/target processing
            source       = os.path.join("lash_compat", 'lash-1.0.pc.in'), # list of string or nodes
            target       = 'lash-1.0.pc', # list of strings or nodes
            install_path = '${LIBDIR}/pkgconfig/',
            # variables to use in the substitution
            prefix       = bld.env['PREFIX'],
            exec_prefix  = bld.env['PREFIX'],
            libdir       = bld.env['LIBDIR'],
            includedir   = os.path.normpath(bld.env['PREFIX'] + '/include'))

    #####################################################
    # gladish
    if bld.env['BUILD_GLADISH']:
        gladish = bld.program(source = [], features = 'c cxx cxxprogram', includes = [bld.path.get_bld()])
        gladish.target = 'gladish'
        gladish.defines = [
            'LOG_OUTPUT_STDOUT',
            # for gtk2 using code, issuing deprecate warnings for unuported upstream gtk2
            # is non-sense at least and may be harmful
            # by making sensible warnings lost in in the noise.
            # glib warnings about gtk2 being deprecated is in same category.
            'GLIB_DISABLE_DEPRECATION_WARNINGS',
#            'GTK_DISABLE_DEPRECATION_WARNINGS',
        ]
        gladish.uselib = 'DBUS-1 CDBUS-1 DBUS-GLIB-1 GTKMM-2.4 LIBGNOMECANVASMM-2.6 GTK+-2.0'

        gladish.source = ["string_constants.c"]

        for source in [
            'main.c',
            'load_project_dialog.c',
            'save_project_dialog.c',
            'project_properties.c',
            'world_tree.c',
            'graph_view.c',
            'canvas.cpp',
            'graph_canvas.c',
            'gtk_builder.c',
            'ask_dialog.c',
            'create_room_dialog.c',
            'menu.c',
            'dynmenu.c',
            'toolbar.c',
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
            'zoom.c',
            ]:
            gladish.source.append(os.path.join("gui", source))

        for source in [
            'Module.cpp',
            'Item.cpp',
            'Port.cpp',
            'Connection.cpp',
            'Ellipse.cpp',
            'Canvas.cpp',
            'Connectable.cpp',
            ]:
            gladish.source.append(os.path.join("gui", "flowcanvas", source))

        for source in [
            'jack_proxy.c',
            'a2j_proxy.c',
            'graph_proxy.c',
            'studio_proxy.c',
            'control_proxy.c',
            'app_supervisor_proxy.c',
            "room_proxy.c",
            "conf_proxy.c",
            ]:
            gladish.source.append(os.path.join("proxies", source))

        for source in [
            'log.c',
            'catdup.c',
            'file.c',
            ]:
            gladish.source.append(os.path.join("common", source))

        # GtkBuilder UI definitions (XML)
        bld.install_files('${DATA_DIR}', 'gui/gladish.ui')

        # 'Desktop' file (menu entry, icon, etc)
        bld.install_files('${PREFIX}/share/applications/', 'gui/gladish.desktop', chmod=0o0644)

        # Icons
        icon_sizes = ['16x16', '22x22', '24x24', '32x32', '48x48', '256x256']
        for icon_size in icon_sizes:
            bld.path.ant_glob('art/' + icon_size + '/apps/*.png')
            bld.install_files('${PREFIX}/share/icons/hicolor/' + icon_size + '/apps/', 'art/' + icon_size + '/apps/gladish.png')

        status_images = []
        for status in ["down", "unloaded", "started", "stopped", "warning", "error"]:
            status_images.append("art/status_" + status + ".png")

        bld.install_files('${DATA_DIR}', status_images)
        bld.install_files('${DATA_DIR}', "art/ladish-logo-128x128.png")

        bld(features='intltool_po', appname=APPNAME, podir='po', install_path="${LOCALE_DIR}")

    bld.install_files('${PREFIX}/bin', 'ladish_control', chmod=0o0755)

    bld.install_files('${DOCDIR}', ["AUTHORS", "README.adoc", "NEWS"])
    bld.install_as('${DATA_DIR}/COPYING', "gpl2.txt")

    if bld.env['BUILD_DOXYGEN_DOCS'] == True:
        html_docs_source_dir = "build/default/html"
        if bld.cmd == 'clean':
            if os.access(html_docs_source_dir, os.R_OK):
                Logs.pprint('CYAN', "Removing doxygen generated documentation...")
                shutil.rmtree(html_docs_source_dir)
                Logs.pprint('CYAN', "Removing doxygen generated documentation done.")
        elif bld.cmd == 'build':
            if not os.access(html_docs_source_dir, os.R_OK):
                os.popen("doxygen").read()
            else:
                Logs.pprint('CYAN', "doxygen documentation already built.")

def get_tags_dirs():
    source_root = os.path.dirname(Utils.g_module.root_path)
    if 'relpath' in os.path.__all__:
        source_root = os.path.relpath(source_root)
    paths = source_root
    paths += " " + os.path.join(source_root, "common")
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

class ladish_dist(Scripting.Dist):
    cmd = 'dist'
    fun = 'dist'

    def __init__(self):
        Scripting.Dist.__init__(self)
        if Options.options.distname:
            self.base_name = Options.options.distname
        else:
            try:
                sha = self.cmd_and_log("LANG= git rev-parse --short HEAD", quiet=Context.BOTH).splitlines()[0]
                self.base_name = APPNAME + '-' + VERSION + "-g" + sha
            except:
                self.base_name = APPNAME + '-' + VERSION
        self.base_name += Options.options.distsuffix

        #print self.base_name

        if Options.options.distname and Options.options.tagdist:
            ret = self.exec_command("LANG= git tag " + self.base_name)
            if ret != 0:
                raise waflib.Errors.WafError('git tag creation failed')

    def get_base_name(self):
        return self.base_name

    def get_excl(self):
        excl = Scripting.Dist.get_excl(self)

        excl += ' .gitmodules'
        excl += ' GTAGS'
        excl += ' GRTAGS'
        excl += ' GPATH'
        excl += ' GSYMS'

        if Options.options.distnodeps:
            excl += ' laditools'
            excl += ' jack2'
            excl += ' a2jmidid'

        #print repr(excl)
        return excl

    def execute(self):
        shutil.copy('./build/version.h', "./")
        try:
            super(ladish_dist, self).execute()
        finally:
            os.remove("version.h")
