project(
  'ladish', 'c', 'cpp',
  version : '2.1',
  license : 'GPL-2.0-only')

data_dir = get_option('prefix') / 'share' / 'ladish'
locale_dir = get_option('prefix') / 'share' / 'locale'
dbus_name_base = 'org.ladish'
dbus_base_path = '/org/ladish'

fs = import('fs')
if fs.is_dir('.git')
  # This block handles building from git source tree
  # version.h is generated during build
  git_version = run_command(
    ['git', 'rev-parse', '--short', 'HEAD'],
    check : true,
    capture : true,
  ).stdout().strip()

  configure_file(output : 'version.h',
                 configuration : {
                                   'GIT_VERSION' : '"@0@"'.format(git_version)
                 }
  )
else
  # Tarballs already have version.h,
  # Just copy the file in this case
  configure_file(output : 'version.h',
                 input : 'version.h',
                 copy: true
  )
endif

alsapid_enabled = 0
if get_option('alsapid').enabled()
  alsapid_enabled = 1
endif

configure_file(output : 'config.h',
  configuration : {
    'PACKAGE_VERSION' : '"@0@"'.format(meson.project_version()),
    'BASE_NAME' : '"@0@"'.format(meson.project_name()),
    'DATA_DIR' : '"@0@"'.format(data_dir),
    'LOCALE_DIR' : '"@0@"'.format(locale_dir),
    'DBUS_NAME_BASE' : '"@0@"'.format(dbus_name_base),
    'DBUS_BASE_PATH' : '"@0@"'.format(dbus_base_path),
#    '_DEFAULT_SOURCE' : 1,
    '_GNU_SOURCE' : 1,
#    '_POSIX_C_SOURCE' : '200809L',
    'BUILD_ALSAPID' : alsapid_enabled,
  }
)

cc = meson.get_compiler('c')

c_args = [
  '-std=gnu99',
#  '-D_GNU_SOURCE',
#  '-D_DEFAULT_SOURCE',
#  '-D_POSIX_C_SOURCE=200112L',
]

inc = include_directories(meson.global_build_root().split('/')[-1])
lash_inc = include_directories('lash_compat/liblash')

jack_dep = dependency('jack')
dbus_dep = dependency('dbus-1')
cdbus_dep = dependency('cdbus-1')

deps = [
  cc.find_library('dl'),
  cc.find_library('util'),
  dbus_dep,
  cdbus_dep,
  jack_dep,
  dependency('alsa'),
  dependency('uuid'),
  dependency('expat')
]

pkg_mod = import('pkgconfig')

subdir('common')
subdir('proxies')
if get_option('alsapid').enabled()
  subdir('alsapid')
endif

# ladishd
if get_option('ladishd').enabled()
  subdir('daemon')
endif

if get_option('liblash').enabled()
  subdir('lash_compat/liblash')
endif

if get_option('gladish').enabled()
  subdir('gui')
  subdir('art')
endif

install_data(['AUTHORS', 'README.adoc', 'NEWS'], install_dir : data_dir)

install_data('ladish_control',
             install_dir : get_option('prefix') / 'bin')

if get_option('jmcore').enabled()
jmcore = executable('jmcore', 'jmcore.c',
                    dependencies : deps,
                    link_with : [commonlib],
                    install : true)
endif

if get_option('ladiconfd').enabled()
ladiconfd = executable('ladiconfd', 'conf.c',
                       dependencies : deps,
                       link_with : [commonlib],
                       install : true)
endif

summary(
  {
    'ladishd': get_option('ladishd').enabled(),
    'ladiconfd': get_option('ladiconfd').enabled(),
    'jmcore': get_option('jmcore').enabled(),
    'alsapid': get_option('alsapid').enabled(),
    'liblash': get_option('liblash').enabled(),
    'pylash': get_option('pylash').enabled(),
    'gladish': get_option('gladish').enabled(),
  },
  bool_yn: true,
  section: 'Configuration',
)
