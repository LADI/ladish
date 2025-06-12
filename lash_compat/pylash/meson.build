#project('lash-bindings', 'c',
#  version : '1.1',
#  license : 'GPL-or-later')

py = import('python')
py = py.find_installation('python2')

libpylash = py.extension_module(
  '_lash',
  [
    'lash.c',
    'lash_wrap.c'
  ],
  c_args : [
    '-std=c99',
    '-includelash/lash.h'
  ],
  dependencies : [
    py.dependency(),
    dependency('lash-1.0')
  ],
  install : true)

py.install_sources('lash.py')
