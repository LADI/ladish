common_sources = [
  'catdup.c',
  'dirhelpers.c',
  'file.c',
  'log.c',
  'time.c',
]

commonlib = static_library('common', common_sources,
                           dependencies : [dbus_dep, cdbus_dep],
                           include_directories : inc,
                           c_args : c_args,
                           install : false)
