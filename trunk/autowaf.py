#!/usr/bin/env python
# Waf utilities for easily building standard unixey packages/libraries
# Coyright (C) 2008 Dave Robillard (see COPYING file for details)

def link_flags(env, lib):
	return ' '.join(map(lambda x: env['LIB_ST'] % x, env['LIB_' + lib]))

def compile_flags(env, lib):
	return ' '.join(map(lambda x: env['CPPPATH_ST'] % x, env['CPPPATH_' + lib]))

def build_pc(bld, name, version, libs):
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

