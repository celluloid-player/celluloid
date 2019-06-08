#!/usr/bin/env python3

import sys

in_file = sys.argv[1]
out_file = sys.argv[2]

with open(in_file) as i, open(out_file, 'w') as o:
    o.write('#pragma once\n\n')
    o.write('#define AUTHORS {\\\n')

    for author in i:
        if not author.startswith('#'):
            o.write('"{}",\\\n'.format(author.rstrip()))

    o.write('NULL}\n')

