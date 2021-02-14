project('ladish', 'c', 'cpp',
  version : '0.3',
  license : 'MIT')

data_dir = get_option('prefix') / 'share' / 'ladish'
locale_dir = get_option('prefix') / 'share' / 'locale'
dbus_name_base = 'org.ladish'
dbus_base_path = '/org/ladish'

git_version = run_command(['git', 'rev-parse', '--short', 'HEAD']).stdout().strip()

run_command(['touch', 'build/version.h', 'build/config.h'])

configure_file(output : 'version.h',
    configuration : {
        'GIT_VERSION' : '"@0@"'.format(git_version)
    }
)

configure_file(output : 'config.h',
  configuration : {
    'PACKAGE_VERSION' : '"@0@"'.format(meson.project_version()),
    'BASE_NAME' : '"@0@"'.format(meson.project_name()),
    'DATA_DIR' : '"@0@"'.format(data_dir),
    'LOCALE_DIR' : '"@0@"'.format(locale_dir),
    'DBUS_NAME_BASE' : '"@0@"'.format(dbus_name_base),
    'DBUS_BASE_PATH' : '"@0@"'.format(dbus_base_path),
    '_GNU_SOURCE' : 1,
  }
)

cc = meson.get_compiler('c')

inc = include_directories('build')
lash_inc = include_directories('lash_compat/liblash')

dbus_dep = dependency('dbus-1')
jack_dep = dependency('jack')

deps = [
  cc.find_library('dl'),
  cc.find_library('util'),
  dbus_dep,
  jack_dep,
  dependency('alsa'),
  dependency('uuid'),
  dependency('expat')
]

pkg_mod = import('pkgconfig')

subdir('common')
subdir('proxies')
subdir('cdbus')
subdir('alsapid')

# ladishd
subdir('daemon')

if get_option('liblash').enabled()
  subdir('lash_compat/liblash')
endif

if get_option('pylash').enabled()
  subdir('lash_compat/pylash')
endif

if get_option('gladish').enabled()
  subdir('gui')
  subdir('art')
endif

install_data(['AUTHORS', 'README', 'NEWS'], install_dir : data_dir)

install_data('ladish_control',
             install_dir : get_option('prefix') / 'bin')

jmcore = executable('jmcore', 'jmcore.c',
                    dependencies : deps,
                    link_with : [commonlib, cdbuslib],
                    install : true)

ladiconfd = executable('ladiconfd', 'conf.c',
                       dependencies : deps,
                       link_with : [commonlib, cdbuslib],
                       install : true)
