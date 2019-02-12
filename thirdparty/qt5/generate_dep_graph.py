#!/usr/bin/env python3

import argparse
import os
import sys
import tarfile


def get_lib_name(filename):
    '''
    Strip 'lib' and 'Qt5' prefixes and file extension.
    '''
    name = os.path.splitext(os.path.basename(filename))[0]

    if name.startswith('lib'):
        name = name[3:]

    return name


def deps_from_prl_file(stream):
    '''
    Get list of Qt dependencies from a file stream for a qmake .prl file.
    '''
    prefix = 'QMAKE_PRL_LIBS = '
    deps = []

    for line in stream:
        line = line.decode('UTF-8')

        if not line.startswith(prefix):
            continue

        items = line[len(prefix):].strip().split()
        for item in items:
            if item.startswith('-L'):
                continue
            elif item.startswith('-l'):
                item = item[2:]

            item = get_lib_name(item)

            if 'qt' not in item.lower():
                continue

            deps.append(item)

    return deps


def generate_dependency_graph(pkg_file):
    cmake_libs = set()
    targets = {}

    with tarfile.open(pkg_file) as tar:
        for member in tar:
            parent = os.path.dirname(member.name)
            filename = os.path.basename(member.name)

            if parent == 'lib/cmake' and filename != 'Qt5':
                cmake_libs.add(filename)
            elif member.name.endswith('.a'):
                name = get_lib_name(filename)
                targets.setdefault(name, {})['path'] = member.name
            elif member.name.endswith('.prl'):
                name = get_lib_name(filename)
                with tar.extractfile(member) as f:
                    targets.setdefault(name, {})['deps'] = deps_from_prl_file(f)

    # Make sure the core libraries don't depend on each other because cmake
    # already handles that
    for name, info in targets.items():
        info['cmake_lib'] = name in cmake_libs
        if info['cmake_lib']:
            info['deps'] = list(filter(
                    lambda x: x not in cmake_libs, info['deps']))

    return targets


def get_cmake_target_name(name):
    if name.startswith('Qt5'):
        name = name[3:]

    return 'Qt5::' + name


def write_cmake_commands(targets, out):
    # Add imported targets
    for name, info in targets.items():
        if info['cmake_lib']:
            continue

        target = get_cmake_target_name(name)

        out.write('add_library("%s" UNKNOWN IMPORTED)\n' % target)
        out.write('set_target_properties("%s" PROPERTIES' % target)
        out.write(' IMPORTED_LINK_INTERFACE_LANGUAGES "CXX" IMPORTED_LOCATION')
        out.write(' "${_qt5Core_install_prefix}/%s")\n' % info['path'])

    # Add dependencies
    for name, info in targets.items():
        if not info['deps']:
            continue

        target = get_cmake_target_name(name)

        out.write('target_link_libraries("%s" INTERFACE' % target)

        for dep in info['deps']:
            out.write(' "%s"' % get_cmake_target_name(dep))

        out.write(')\n')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('file', help='pkg.tar.xz file for Qt5')

    args = parser.parse_args()

    targets = generate_dependency_graph(args.file)
    write_cmake_commands(targets, sys.stdout)


if __name__ == '__main__':
    main()
