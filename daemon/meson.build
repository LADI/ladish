daemon_sources = [
  'app_supervisor.c',
  'check_integrity.c',
  'client.c',
  'cmd_change_app_state.c',
  'cmd_create_room.c',
  'cmd_delete_room.c',
  'cmd_exit.c',
  'cmd_load_project.c',
  'cmd_load_studio.c',
  'cmd_new_app.c',
  'cmd_new_studio.c',
  'cmd_remove_app.c',
  'cmd_rename_studio.c',
  'cmd_save_project.c',
  'cmd_save_studio.c',
  'cmd_start_studio.c',
  'cmd_stop_studio.c',
  'cmd_unload_project.c',
  'cmd_unload_studio.c',
  'control.c',
  'cqueue.c',
  'dict.c',
  'escape.c',
  'graph.c',
  'graph_dict.c',
  'graph_manager.c',
  'jack_session.c',
  'lash_server.c',
  'load.c',
  'loader.c',
  'main.c',
  'port.c',
  'procfs.c',
  'proctitle.c',
  'recent_projects.c',
  'recent_store.c',
  'room.c',
  'room_load.c',
  'room_save.c',
  'save.c',
  'studio.c',
  'studio_jack_conf.c',
  'studio_list.c',
  'virtualizer.c',
  '../string_constants.c',
]

# define SIGINFO_ENABLED and build
#  'siginfo.c',

ladishd_libs = [commonlib, cdbuslib, proxieslib]
ladishd_libs += cdbuslib
if get_option('alsapid').enabled()
  ladishd_libs += alsapidlib
endif

ladishd = executable('ladishd', daemon_sources,
                     dependencies : deps,
                     include_directories : inc,
		     c_args : c_args,
                     link_with : ladishd_libs,
                     install : true)
