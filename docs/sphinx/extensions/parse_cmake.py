# -*- coding: utf-8 -*-
#
# Copyright (c) 2019 Thomas Heller
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

import os
import re

toolchain_template = '''
{toolchain_name}
{section_sep}

.. literalinclude:: {toolchain_file}
    :language: cmake
'''

def parse_cmake(hpx_source_dir, cmake_file, options):
    try:
        print('Processing ' + hpx_source_dir + cmake_file)
        func_call_re = re.compile('^\s*([a-z_]+)\s*\((.*)', re.IGNORECASE)
        option_category_re = re.compile('.*CATEGORY\s+"([^"]+)".*')
        strings_re = re.compile('.*STRINGS\s+"([^"]+)".*')

        content = ''
        function_name = None
        for line in open(hpx_source_dir + cmake_file).readlines():
            line = line.strip()
            # ignore comments
            line = re.sub('#.*$','', line)
            if line == '':
                continue

            match = func_call_re.match(line)
            if match:
                name, remainder = match.groups()
                if function_name is None:
                    function_name = name
                    content += ' ' + remainder
                else:
                    args = content[:-1] if content[-1] == ')' else content
                    if function_name == 'include':
                        include_file = '/cmake/' + args
                        if os.path.exists(hpx_source_dir + include_file):
                            parse_cmake(hpx_source_dir, include_file, options)
                        elif os.path.exists(hpx_source_dir + include_file + '.cmake'):
                            parse_cmake(hpx_source_dir, include_file + '.cmake', options)
                    if function_name == 'add_subdirectory':
                        subdir = hpx_source_dir + '/' + args
                        if os.path.exists(subdir + '/CMakeLists.txt'):
                            parse_cmake(subdir, '/CMakeLists.txt', options)
                    if function_name == 'hpx_option':
                        args_list = args.split(' ')
                        var, var_type = args_list[0:2]

                        advanced = 'ADVANCED' in args
                        category = 'General'
                        category_match = option_category_re.match(args)
                        if category_match:
                            category = category_match.group(1)
                        values = ''
                        strings_match = strings_re.match(args)
                        if strings_match:
                            values = strings_match.group(1)

                        docstring = args_list[2][1:]
                        for arg in args_list[3:]:
                            docstring += ' ' + arg
                            if len(arg) != 0 and arg[-1] == '"':
                            #if arg[-1] == '"':
                                break
                        if not category in options:
                            options[category] = []
                        options[category].append((var, var_type, docstring, values, advanced))
                        print('option: %s(%s), category=%s %s' % (var, var_type, category, docstring))
                    function_name = name
                    content = remainder
            else:
                content += ' ' + line

    except Exception, e:
        print('error: %s' % e)


def generate_cmake_doc(hpx_source_dir):
    # Create the documentation for the toolchain files.
    toolchains = ''
    # Scan the toolchain directory...
    toolchain_dir = hpx_source_dir + '/cmake/toolchains'
    for toolchain in os.listdir(toolchain_dir):
        toolchain_name = os.path.splitext(toolchain)[0]
        section_sep = '-' * len(toolchain_name)
        toolchains += toolchain_template.format(
            toolchain_name=toolchain_name,
            section_sep=section_sep,
            toolchain_file= '../../../cmake/toolchains/' + toolchain)

    # Write out toolchain documentation
    cmake_toolchains_rst = open(os.path.abspath('.') + '/manual/cmake_toolchains.rst', 'w')
    for line in open(hpx_source_dir + '/docs/sphinx/_templates/cmake_toolchains.rst').readlines():
        cmake_toolchains_rst.write(line.format(toolchains))
    cmake_toolchains_rst.close()
    print('Generated ' + os.path.abspath('.') + '/manual/cmake_toolchains.rst')
    print('yay')

    return (u'1.3.0', u'1.3.0-dev')

if __name__ == '__main__':
    options =  {}
    parse_cmake('/home/heller/programming/hpx', '/CMakeLists.txt', options)
    for option in options.keys():
        print(option)
        for var in options[option]:
            print('    %s' % str(var))
