# -*- coding: utf-8 -*-
#
# Copyright (c) 2019 Thomas Heller
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

import re
import os

api_ref_header = '''
..
    Copyright (C) 2019 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

-------------------------------------------------------------------------------
{}
-------------------------------------------------------------------------------
'''

api_ref_file = '''
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
`#include <{0}>`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. autodoxygenfile:: {0}
   :project: {1}
'''

def generate_breathe_projects(hpx_source_dir):
    hpx_libs_dir = hpx_source_dir + '/libs'

    # create symlink to the libs directory if it doesn't already exist...
    doc_libs_path = os.path.abspath('.') + '/libs'
    if not os.path.exists(doc_libs_path):
        os.symlink(hpx_libs_dir, doc_libs_path)

    breathe_projects_source = {'hpx': (hpx_source_dir, [])}

    # Scan the libs directory for source files to be included in
    # the documentation
    file_regex = re.compile('.*\.(h|hxx|hpp|ipp)$')
    for subdir_full, dirs, files in os.walk(hpx_libs_dir):
        # strip the prefix of the directory...
        subdir = subdir_full[len(hpx_libs_dir) + 1:]

        # If we are in the root of the libs dir, we add all
        # subdirectories as a new breathe project
        if len(subdir) == 0:
            for dir in dirs:
                breathe_projects_source[dir] = (subdir_full + '/' + dir, [])
        # We are inside a library and need to add the files
        # to the breathe project
        else:
            # We only scan the include subdir...
            if not '/include' in subdir:
                continue
            # ... but omit anything in detail directories
            if '/detail' in subdir:
                continue

            # strip the name of the library... the base path is already
            # set
            subdirs = subdir.split('/')
            lib = subdirs[0]
            subdir = '/'.join(subdirs[1:])
            # And now add all remaining files
            # which are headers or source files
            for f in files:
                if not file_regex.match(f) is None:
                    skip = False
                    for line in file(subdir_full + '/' + f):
                        if '// sphinx:undocumented' in line:
                            skip = True
                            break
                    if skip:
                        continue
                    breathe_projects_source[lib][1].append(subdir + '/' + f)

    # Generate the rst files for the API documentation
    main_refs = ''
    api_refs = ''

    # Collect and generate header API documentation
    for lib in breathe_projects_source:
        lib_sources = breathe_projects_source[lib]
        if len(lib_sources[1]) == 0:
            continue
        print(lib)
        basedir = os.path.abspath('.') + '/libs/' + lib
        if not os.path.exists(basedir):
            os.makedirs(basedir)
        lib_api_ref = open(basedir + '/api.rst', 'w')
        lib_name = lib.upper() if len(lib) == 2 else lib.capitalize()
        lib_api_ref.write(api_ref_header.format(lib_name))
        for source in lib_sources[1]:
            header = source[len('include/'):]
            lib_api_ref.write(api_ref_file.format(header, lib))
        api_refs += '   /libs/' + lib + '/api.rst\n'
        print('Generated ' + basedir + '/api.rst')

    for header in open(os.path.abspath('.') + '/headers.txt').readlines():
        header = header.strip()
        breathe_projects_source['hpx'][1].append(header)
        main_refs += api_ref_file.format(header, 'hpx')

    api_rst = open(os.path.abspath('.') + '/api.rst', 'w')
    for line in open(hpx_source_dir + '/docs/sphinx/_templates/api.rst').readlines():
        api_rst.write(line.format(lib_refs=api_refs, main_refs=main_refs))
    api_rst.close()
    print('Generated ' + os.path.abspath('.') + '/api.rst')

    return breathe_projects_source
