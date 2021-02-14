py = import('python')
py = py.find_installation('python2')

libpylash = py.extension_module('pylash', ['lash.c', 'lash_wrap.c'],
                  include_directories : [lash_inc, inc],
                  dependencies : [py.dependency()],
                  install : true)

py.install_sources('lash.py')
