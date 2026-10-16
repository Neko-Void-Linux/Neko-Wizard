project('neko-store', 'c',
  version : '0.1.0',
  default_options : ['warning_level=2'])

gtkdep = dependency('gtk4')

src = [
  'src/main.c',
  'src/apps_manager.c',
  'src/installer.c',
  'src/window.c',
  'src/app_card.c',
  'src/mirror_manager.c',
  'src/privileged.c',
]

executable('neko-store',
  src,
  dependencies : [gtkdep],
  install : true
)
