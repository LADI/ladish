#!/usr/bin/env python
import Params
import autowaf

# Version of this package (even if built as a child)
RAUL_VERSION = '0.5.1'

# Variables for 'waf dist'
APPNAME = 'raul'
VERSION = RAUL_VERSION

# Mandatory variables
srcdir = '.'
blddir = 'build'

def set_options(opt):
	opt.tool_options('compiler_cxx')

def configure(conf):
	if not conf.env['CXX']:
		conf.check_tool('compiler_cxx')
	if not conf.env['HAVE_GLIBMM']:
		conf.check_pkg('glibmm-2.4', destvar='GLIBMM', vnum='2.16.0', mandatory=True)
	if not conf.env['HAVE_GTHREAD']:
		conf.check_pkg('gthread-2.0', destvar='GTHREAD', vnum='2.16.0', mandatory=True)
	if not conf.env['HAVE_JACK']:
		conf.check_pkg('jack', destvar='JACK', vnum='0.107.0', mandatory=True)

def build(bld):
	# Headers
	install_files('PREFIX', 'include/raul', 'raul/*.hpp')
	install_files('PREFIX', 'include/raul', 'raul/*.h')
	
	# Pkgconfig file
	autowaf.build_pc(bld, 'RAUL', RAUL_VERSION, 'GLIBMM GTHREAD JACK')
	
	# Library
	obj = bld.create_obj('cpp', 'shlib')
	obj.source = '''
		src/Maid.cpp
		src/Path.cpp
		src/SMFReader.cpp
		src/SMFWriter.cpp
		src/Symbol.cpp
		src/Thread.cpp
	'''
	obj.includes = '..'
	obj.name     = 'libraul'
	obj.target   = 'raul'
	obj.uselib   = 'GLIBMM GTHREAD'
	obj.vnum     = '1.0.0'
	
	# Unit tests
	bld.add_subdirs('tests')
