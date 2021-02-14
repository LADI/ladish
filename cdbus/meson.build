cdbus_sources = [
    'helpers.c',
    'interface.c',
    'method.c',
    'object_path.c',
    'signal.c',
]

cdbuslib = static_library('cdbus', cdbus_sources,
                          include_directories : inc,
                          dependencies : dbus_dep,
                          install : false)
