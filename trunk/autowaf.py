#!/usr/bin/env python
# Waf utilities for easily building standard unixey packages/libraries
# Coyright (C) 2008 Dave Robillard (see COPYING file for details)

import os
import misc

def configure(conf):
	conf.check_tool('misc')

def link_flags(env, lib):
	return ' '.join(map(lambda x: env['LIB_ST'] % x, env['LIB_' + lib]))

def compile_flags(env, lib):
	return ' '.join(map(lambda x: env['CPPPATH_ST'] % x, env['CPPPATH_' + lib]))

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
	obj = bld.create_obj('subst')
	obj.source = 'doc/reference.doxygen.in'
	obj.target = 'doc/reference.doxygen'
	doc_dir = blddir + '/default/' + name.lower() + '/doc'
	obj.dict = {
		name + '_VERSION' : version,
		name + '_SRCDIR'  : os.path.abspath(srcdir) + '/' + name.lower(),
		name + '_DOC_DIR' : os.path.abspath(doc_dir)
	}
	obj.inst_var = 0
	out1 = bld.create_obj('command-output')
	out1.stdout = '/doc/doxygen.out'
	out1.stdin = '/doc/reference.doxygen' # whatever..
	out1.command = 'doxygen'
	#out1.argv = [os.path.abspath(blddir) + '/default/doc/reference.doxygen']
	out1.argv = [os.path.abspath(doc_dir) + '/reference.doxygen']
	out1.command_is_external = True


