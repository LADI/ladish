#! /usr/bin/env python
# encoding: utf-8

APPNAME='ladish'
VERSION='1'

# these variables are mandatory ('/' are converted automatically)
srcdir = '.'
blddir = 'build'

def set_options(opt):
    opt.tool_options('compiler_cc')

def configure(conf):
    conf.check_tool('compiler_cc')

def build(bld):
    pass

def dist_hook():
    pass
