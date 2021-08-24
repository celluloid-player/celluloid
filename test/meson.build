test_option_parser = executable(
  'test-option-parser',
  ['..' / 'src' / 'celluloid-option-parser.c', 'test-option-parser.c'],
  include_directories: include_directories('..' / 'src'),
  dependencies: libgtk
)

test_playlist_model = executable(
  'test-playlist-model',
  generated_marshal_sources +
  [ '..' / 'src' / 'celluloid-playlist-model.c',
    '..' / 'src' / 'celluloid-playlist-item.c',
    'test-playlist-model.c'],
  include_directories: include_directories('..' / 'src'),
  dependencies: libgtk
)

test('test-option-parser', test_option_parser)
test('test-playlist-model', test_playlist_model)
