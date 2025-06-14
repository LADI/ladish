gui_deps = [
  dependency('dbus-glib-1'),
  dbus_dep,
  cdbus_dep,
]

subdir('flowcanvas')

gladish_sources = [
    'about.c',
    'action.c',
    'ask_dialog.c',
    'canvas.cpp',
    'control.c',
    'create_room_dialog.c',
    'dbus.c',
    'dialogs.c',
    'dynmenu.c',
    'graph_canvas.c',
    'graph_view.c',
    'gtk_builder.c',
    'jack.c',
    'load_project_dialog.c',
    'main.c',
    'menu.c',
    'pixbuf.c',
    'project_properties.c',
    'room.c',
    'save_project_dialog.c',
    'settings.c',
    'statusbar.c',
    'studio.c',
    'studio_list.c',
    'toolbar.c',
    'world_tree.c',
    'zoom.c',
    '../string_constants.c',
]

gladish = executable('gladish', gladish_sources,
                     dependencies : [gui_deps, flowcanvas_deps],
                     include_directories : [flowcanvas_inc, inc],
                     c_args : c_args + [
                       '-DGLIB_DISABLE_DEPRECATION_WARNINGS'
                     ],
                     cpp_args : [
                       '-std=c++23',
                       '-DGLIB_DISABLE_DEPRECATION_WARNINGS'
                     ],
                     link_with : [flowcanvaslib, proxieslib, commonlib],
                     install : true)

install_data('gladish.ui', install_dir : data_dir)
install_data('gladish.desktop', install_dir : get_option('datadir') / 'applications' )
