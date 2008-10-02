#!/usr/bin/env python
import Params
import autowaf

# Version of this package (even if built as a child)
RAUL_VERSION = '0.5.1'

# Library version (UNIX style major, minor, micro)
# major increment <=> incompatible changes
# minor increment <=> compatible changes (additions)
# micro increment <=> no interface changes
# Version history:
#   0.4.0 = 0,0,0
#   0.5.0 = 1,0,0 (SVN r1283)
#   0.5.1 = 2,0,0
RAUL_LIB_VERSION = '2.0.0'

# Variables for 'waf dist'
APPNAME = 'raul'
VERSION = RAUL_VERSION

# Mandatory variables
srcdir = '.'
blddir = 'build'

def set_options(opt):
	autowaf.set_options(opt)
	opt.tool_options('compiler_cxx')

def configure(conf):
	autowaf.configure(conf)
	if not conf.env['CXX']:
		conf.check_tool('compiler_cxx')
	if not conf.env['HAVE_GLIBMM']:
		conf.check_pkg('glibmm-2.4', destvar='GLIBMM', vnum='2.16.0', mandatory=True)
	if not conf.env['HAVE_GTHREAD']:
		conf.check_pkg('gthread-2.0', destvar='GTHREAD', vnum='2.16.0', mandatory=True)
	if not conf.env['HAVE_JACK']:
		conf.check_pkg('jack', destvar='JACK', vnum='0.107.0', mandatory=False)
	
	autowaf.print_summary(conf)
	print '= Raul Configuration ='
	autowaf.display_msg("Jack", str(bool(conf.env['HAVE_JACK'])), 'YELLOW')
	print

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
	if bld.env()['HAVE_JACK']:
		obj.source += ' src/JackDriver.cpp '
	obj.includes = '..'
	obj.name     = 'libraul'
	obj.target   = 'raul'
	obj.uselib   = 'GLIBMM GTHREAD JACK'
	obj.vnum     = RAUL_LIB_VERSION
	
	# Unit tests
	bld.add_subdirs('tests')
	
	# Documentation
	autowaf.build_dox(bld, 'RAUL', RAUL_VERSION, srcdir, blddir)
	install_files('PREFIX', 'share/doc/raul', blddir + '/default/doc/html/*')

