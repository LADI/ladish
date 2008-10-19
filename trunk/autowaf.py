#!/usr/bin/env python
# Waf utilities for easily building standard unixey packages/libraries
# Licensed under the GNU GPL v2 or later, see COPYING file for details.
# Copyright (C) 2008 Dave Robillard
# Copyright (C) 2008 Nedko Arnaudov

import os
import misc
import Params
import Configure
import Utils

Configure.g_maxlen = 55
g_is_child = False

# Only run autowaf hooks once (even if sub projects call several times)
global g_step
g_step = 0

def set_options(opt):
	"Add standard autowaf options if they havn't been added yet"
	global g_step
	if g_step > 0:
		return
	opt.tool_options('compiler_cxx')
	opt.add_option('--debug', action='store_true', default=False, dest='debug',
			help="Build debuggable binaries [Default: False]")
	opt.add_option('--strict', action='store_true', default=False, dest='strict',
			help="Use strict compiler flags and show all warnings [Default: False]")
	opt.add_option('--build-docs', action='store_true', default=False, dest='build_docs',
			help="Build documentation - requires doxygen [Default: False]")
	opt.add_option('--bindir', type='string', help="Executable programs [Default: PREFIX/bin]")
	opt.add_option('--libdir', type='string', help="Libraries [Default: PREFIX/lib]")
	opt.add_option('--includedir', type='string', help="Header files [Default: PREFIX/include]")
	opt.add_option('--datadir', type='string', help="Shared data [Default: PREFIX/share]")
	opt.add_option('--mandir', type='string', help="Manual pages [Default: DATADIR/man]")
	opt.add_option('--htmldir', type='string', help="HTML documentation [Default: DATADIR/doc/PACKAGE]")
	g_step = 1

def check_header(conf, name, define='', **args):
	"Check for a header iff it hasn't been checked for yet"
	if type(conf.env['AUTOWAF_HEADERS']) != dict:
		conf.env['AUTOWAF_HEADERS'] = {}

	checked = conf.env['AUTOWAF_HEADERS']
	if not name in checked:
		conf.check_header(name, define, **args)
		checked[name] = True

def check_tool(conf, name):
	"Check for a tool iff it hasn't been checked for yet"
	if type(conf.env['AUTOWAF_TOOLS']) != dict:
		conf.env['AUTOWAF_TOOLS'] = {}

	checked = conf.env['AUTOWAF_TOOLS']
	if not name in checked:
		conf.check_tool(name)
		checked[name] = True

def check_pkg(conf, name, **args):
	"Check for a package iff it hasn't been checked for yet"
	if not 'HAVE_' + args['destvar'] in conf.env:
		if not conf.check_pkg(name, **args):
			conf.env['HAVE_' + args['destvar']] = False
		else:
			conf.env['HAVE_' + args['destvar']] = True

def configure(conf):
	global g_step
	if g_step > 1:
		return
	def append_cxx_flags(val):
		conf.env.append_value('CCFLAGS', val)
		conf.env.append_value('CXXFLAGS', val)
	check_tool(conf, 'misc')
	check_tool(conf, 'compiler_cc')
	check_tool(conf, 'compiler_cxx')
	conf.env['BUILD_DOCS'] = Params.g_options.build_docs
	conf.env['DEBUG'] = Params.g_options.debug
	if Params.g_options.bindir:
		conf.env['BINDIR'] = Params.g_options.bindir
	else:
		conf.env['BINDIR'] = conf.env['PREFIX'] + 'bin/'
	if Params.g_options.libdir:
		conf.env['LIBDIR'] = Params.g_options.libdir
	else:
		conf.env['LIBDIR'] = conf.env['PREFIX'] + 'lib/'
	if Params.g_options.datadir:
		conf.env['DATADIR'] = Params.g_options.datadir
	else:
		conf.env['DATADIR'] = conf.env['PREFIX'] + 'share/'
	if Params.g_options.htmldir:
		conf.env['HTMLDIR'] = Params.g_options.htmldir
	else:
		conf.env['HTMLDIR'] = conf.env['DATADIR'] + 'doc/' + Utils.g_module.APPNAME + '/'
	if Params.g_options.mandir:
		conf.env['MANDIR'] = Params.g_options.mandir
	else:
		conf.env['MANDIR'] = conf.env['DATADIR'] + 'man/'
	if Params.g_options.debug:
		conf.env['CCFLAGS'] = '-O0 -g -std=c99'
		conf.env['CXXFLAGS'] = '-O0 -g -ansi'
	if Params.g_options.strict:
		conf.env['CCFLAGS'] = '-O0 -g -std=c99 -pedantic'
		append_cxx_flags('-Wall -Wextra -Wno-unused-parameter')
	append_cxx_flags('-fPIC -DPIC')
	g_step = 2
	
def set_local_lib(conf, name, has_objects):
	conf.define('HAVE_' + name.upper(), True)
	if has_objects:
		if type(conf.env['AUTOWAF_LOCAL_LIBS']) != dict:
			conf.env['AUTOWAF_LOCAL_LIBS'] = {}
		conf.env['AUTOWAF_LOCAL_LIBS'][name.lower()] = True
	else:
		if type(conf.env['AUTOWAF_LOCAL_HEADERS']) != dict:
			conf.env['AUTOWAF_LOCAL_HEADERS'] = {}
		conf.env['AUTOWAF_LOCAL_HEADERS'][name.lower()] = True

def use_lib(bld, obj, libs):
	abssrcdir = os.path.abspath('.')
	libs_list = libs.split()
	for l in libs_list:
		in_headers = l.lower() in bld.env()['AUTOWAF_LOCAL_HEADERS']
		in_libs    = l.lower() in bld.env()['AUTOWAF_LOCAL_LIBS']
		if in_libs:
			obj.uselib_local += ' lib' + l.lower() + ' '
		
		if in_headers or in_libs:
			inc_flag = '-iquote' + abssrcdir + '/' + l.lower()
			for f in ['CCFLAGS', 'CXXFLAGS']:
				if not inc_flag in bld.env()[f]:
					bld.env().prepend_value(f, inc_flag)
		else:
			obj.uselib += ' ' + l


def display_header(title):
	Params.pprint('BOLD', title)

def display_msg(msg, status = None, color = None):
	Configure.g_maxlen = max(Configure.g_maxlen, len(msg))
	if status:
		print "%s :" % msg.ljust(Configure.g_maxlen),
		Params.pprint(color, status)
	else:
		print "%s" % msg.ljust(Configure.g_maxlen)

def print_summary(conf):
	global g_step
	if g_step > 2:
		print
		return
	e = conf.env
	print
	display_header('Global configuration')
	display_msg("Install prefix", conf.env['PREFIX'], 'CYAN')
	display_msg("Debuggable build", str(conf.env['DEBUG']), 'YELLOW')
	display_msg("Build documentation", str(conf.env['BUILD_DOCS']), 'YELLOW')
	print
	g_step = 3

def link_flags(env, lib):
	return ' '.join(map(lambda x: env['LIB_ST'] % x, env['LIB_' + lib]))

def compile_flags(env, lib):
	return ' '.join(map(lambda x: env['CPPPATH_ST'] % x, env['CPPPATH_' + lib]))

def set_recursive():
	global g_is_child
	g_is_child = True

def is_child():
	global g_is_child
	return g_is_child

# Pkg-config file
def build_pc(bld, name, version, libs):
	'''Build a pkg-config file for a library.
	name    -- uppercase variable name     (e.g. 'SOMENAME')
	version -- version string              (e.g. '1.2.3')
	libs    -- string/list of dependencies (e.g. 'LIBFOO GLIB')
	'''

	obj          = bld.create_obj('subst')
	obj.source   = name.lower() + '.pc.in'
	obj.target   = name.lower() + '.pc'
	obj.inst_var = 'PREFIX'
	obj.inst_dir = 'lib/pkgconfig'
	pkg_prefix   = bld.env()['PREFIX'] 
	if pkg_prefix[-1] == '/':
		pkg_prefix = pkg_prefix[:-1]
	obj.dict     = {
		'prefix'           : pkg_prefix,
		'exec_prefix'      : '${prefix}',
		'libdir'           : '${exec_prefix}/lib',
		'includedir'       : '${prefix}/include',
		name + '_VERSION'  : version,
	}
	if type(libs) != list:
		libs = libs.split()
	for i in libs:
		obj.dict[i + '_LIBS']   = link_flags(bld.env(), i)
		obj.dict[i + '_CFLAGS'] = compile_flags(bld.env(), i)

# Doxygen API documentation
def build_dox(bld, name, version, srcdir, blddir):
	if not bld.env()['BUILD_DOCS']:
		return
	obj = bld.create_obj('subst')
	obj.source = 'doc/reference.doxygen.in'
	obj.target = 'doc/reference.doxygen'
	if is_child():
		src_dir = srcdir + '/' + name.lower()
		doc_dir = blddir + '/default/' + name.lower() + '/doc'
	else:
		src_dir = srcdir
		doc_dir = blddir + '/default/doc'
	obj.dict = {
		name + '_VERSION' : version,
		name + '_SRCDIR'  : os.path.abspath(src_dir),
		name + '_DOC_DIR' : os.path.abspath(doc_dir)
	}
	obj.inst_var = 0
	out1 = bld.create_obj('command-output')
	out1.stdout = '/doc/doxygen.out'
	out1.stdin = '/doc/reference.doxygen' # whatever..
	out1.command = 'doxygen'
	out1.argv = [os.path.abspath(doc_dir) + '/reference.doxygen']
	out1.command_is_external = True

def shutdown():
	# This isn't really correct (for packaging), but people asking is annoying
	if Params.g_commands['install']:
		try: os.popen("/sbin/ldconfig")
		except: pass

