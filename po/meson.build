i18n.gettext(meson.project_name(),
  preset: 'glib'
)

msgfmt = find_program('msgfmt', required: false)
if msgfmt.found()
  linguas = run_command('cat', files('LINGUAS')).stdout().strip().split('\n')
  foreach lingua : linguas
    po_file = '.'.join([lingua, 'po'])
    test('Validate @0@'.format(po_file), msgfmt,
      args: [
        '--output-file=/dev/null',
        '--check-accelerator=_',
        '-c',
        '-v',
        join_paths(meson.current_source_dir(), po_file)
      ]
    )
  endforeach
endif
