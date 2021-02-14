install_headers([
    'lash/client_interface.h',
    'lash/config.h',
    'lash/event.h',
    'lash/lash.h',
    'lash/protocol.h',
    'lash/types.h',
], subdir : 'lash-1.0/lash')

liblash = library('lash', 'lash.c',
                  include_directories : [lash_inc, inc],
                  dependencies : [dbus_dep],
                  link_with : [cdbuslib, commonlib],
                  install : true)

pkg_mod.generate(libraries : liblash,
                 version : '1.1.1',
                 name : 'liblash',
                 requires : dbus_dep,
                 description : 'lash compatibility library to ladish',
                 filebase : 'liblash')
