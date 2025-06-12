proxies_sources = [
  'a2j_proxy.c',
  'app_supervisor_proxy.c',
  'conf_proxy.c',
  'control_proxy.c',
  'graph_proxy.c',
  'jack_proxy.c',
  'jmcore_proxy.c',
  'lash_client_proxy.c',
  'notify_proxy.c',
  'room_proxy.c',
  'studio_proxy.c',
]

proxieslib = static_library('proxies', proxies_sources,
                            c_args : c_args,
                            include_directories : inc,
                            dependencies : dbus_dep,
                            install : false)
