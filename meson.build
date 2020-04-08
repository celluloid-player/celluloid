project('celluloid', 'c',
  version: '0.19',
  meson_version: '>= 0.40.0',
  default_options: [
    'warning_level=2',
    'c_std=gnu99',
  ]
)

i18n = import('i18n')
cc = meson.get_compiler('c')

test_cflags = [
  '-funsigned-char',
  '-fstack-protector-strong',
  '-fPIE',
  '-fPIC',
  '-Wconversion',
  '-Winline',
  '-Wno-padded',
  '-Wno-unused-parameter',
  '-Wstrict-prototypes',
  '-Wmissing-prototypes',
  '-Werror=implicit-function-declaration',
  '-Werror=pointer-arith',
  '-Werror=init-self',
  ['-Werror=format-security', '-Werror=format=2'], # Must be checked together
  '-Werror=missing-include-dirs',
  '-Werror=date-time'
]
global_cflags = []
foreach cflag : test_cflags
  if cc.has_multi_arguments(cflag)
    global_cflags += cflag
  endif
endforeach
if cc.has_header('execinfo.h')
  global_cflags += '-DHAVE_EXECINFO_H'
endif
add_project_arguments(global_cflags, language: 'c')

test_ldflags = [
  '-pie',
  '-Wl,-z,relro',
  '-Wl,-z,now'
]
global_ldflags = []
foreach ldflag : test_ldflags
  if cc.has_argument(ldflag)
    global_ldflags += ldflag
  endif
endforeach
add_project_link_arguments(global_ldflags, language: 'c')

if not cc.has_function('setlocale')
  error('Could not find setlocale()')
endif

subdir('src')
subdir('data')
subdir('docs')
subdir('po')

meson.add_install_script('meson-post-install.py')
