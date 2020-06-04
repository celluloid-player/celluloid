exe = executable(
  'test-option-parser',
  ['..' / 'src' / 'celluloid-option-parser.c', 'test-option-parser.c'],
  include_directories: include_directories('..' / 'src'),
  dependencies: libgtk
)

test('test-option-parser', exe)
