flowcanvas_inc = include_directories('.')

flowcanvas_deps = [
  # dependency('libgvc', version : '>=2.8'),
  dependency('gtkmm-2.4', version : '>=2.10.0', required : true),
  dependency('libgnomecanvasmm-2.6', version : '>=2.6.0', required : true),
  # dependency('boost', required : true)
]

flowcanvas_sources = [
  'Canvas.cpp',
  'Connectable.cpp',
  'Connection.cpp',
  'Ellipse.cpp',
  'Item.cpp',
  'Module.cpp',
  'Port.cpp'
]

flowcanvaslib = static_library('flowcanvas', flowcanvas_sources,
                               include_directories : [flowcanvas_inc, inc],
                               dependencies : flowcanvas_deps,
                               install : false)
