#!/usr/bin/env python
# Waf utilities for easily building standard unixey packages/libraries
# Coyright (C) 2008 Dave Robillard (see COPYING file for details)
# Copyright (C) 2008 Nedko Arnaudov (FIXME: verify license)

import os
import misc
import Params
import Configure

Configure.g_maxlen = 55
g_is_child = False

# Only run autowaf hooks once (even if sub projects call several times)
global g_step
g_step = 0

def set_options(opt):
	global g_step
	if g_step > 0:
		return
	opt.add_option('--build-docs', action='store_true', default=False, dest='build_docs',
			help="Build documentation - requires doxygen [Default: False]")
	opt.add_option('--debug', action='store_true', default=False, dest='debug',
			help="Build debuggable binaries [Default: False]")
	opt.add_option('--strict', action='store_true', default=False, dest='strict',
			help="Use strict compiler flags and show all warnings [Default: False]")
	opt.tool_options('compiler_cxx')
	g_step = 1

def configure(conf):
	global g_step
	if g_step > 1:
		return
	e = conf.env
	def append_cxx_flags(val):
		e.append_value('CCFLAGS', val)
		e.append_value('CXXFLAGS', val)
	conf.check_tool('misc')
	conf.env['BUILD_DOCS'] = Params.g_options.build_docs
	conf.env['DEBUG'] = Params.g_options.debug
	if Params.g_options.debug:
		e['CCFLAGS'] = '-O0 -g -std=c99'
		e['CXXFLAGS'] = '-O0 -g'
	if Params.g_options.strict:
		append_cxx_flags('-Wall') # evoral currently -pedantic broken
	append_cxx_flags('-DCONFIG_H_PATH=\\\"waf-config.h\\\"')
	g_step = 2
	
def set_local_lib(conf, name):
	if not type(conf.env['AUTOWAF_LOCAL_LIBS']) == dict:
		conf.env['AUTOWAF_LOCAL_LIBS'] = {}
		
	conf.env['AUTOWAF_LOCAL_LIBS'][name.lower()] = True

def use_lib(bld, obj, libs):
	libs_list = libs.split()
	for l in libs_list:
		if l.lower() in bld.env()['AUTOWAF_LOCAL_LIBS']:
			obj.uselib_local += ' lib' + l.lower() + ' '
		else:
			obj.uselib += ' ' + l.upper() + ' '

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
	obj.dict     = {
		'prefix'           : bld.env()['PREFIX'][:-1],
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


