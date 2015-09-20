#!/usr/bin/python3

# Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# ---

# Generate changelogs and create HTML output

import distutils.core
import jinja2
import os
import re
import subprocess
import sys

maxbuilds = 999999


class Version():
    def __init__(self, version):
        self.ver = version
        s = version.split('.')
        self.p1 = int(s[0])
        self.p2 = int(s[1])
        self.p3 = int(s[2])
        self.p4 = int(s[3][1:])
        self.commit = s[4][1:]

    def __lt__(self, other):
        if self.p1 > other.p1:
            return False
        elif self.p1 < other.p1:
            return True
        elif self.p2 > other.p2:
            return False
        elif self.p2 < other.p2:
            return True
        elif self.p3 > other.p3:
            return False
        elif self.p3 < other.p3:
            return True
        elif self.p4 > other.p4:
            return False
        elif self.p4 < other.p4:
            return True
        else:
            return False

    def __gt__(self, other):
        if self.p1 < other.p1:
            return False
        elif self.p1 > other.p1:
            return True
        elif self.p2 < other.p2:
            return False
        elif self.p2 > other.p2:
            return True
        elif self.p3 < other.p3:
            return False
        elif self.p3 > other.p3:
            return True
        elif self.p4 < other.p4:
            return False
        elif self.p4 > other.p4:
            return True
        else:
            return False

    def __le__(self, other):
        return self == other or self < other

    def __ge__(self, other):
        return self == other or self > other

    def __eq__(self, other):
        return self.p1 == other.p1 \
            and self.p2 == other.p2 \
            and self.p3 == other.p3 \
            and self.p4 == other.p4

    def __str__(self):
        return self.ver

    def __hash__(self):
        return hash(self.ver)


def quicksort(l):
    if l == []:
        return []
    else:
        pivot = l[0]
        left = quicksort([x for x in l[1:] if x >= pivot])
        right = quicksort([x for x in l[1:] if x < pivot])
        return left + [pivot] + right


# From http://code.activestate.com/recipes/577081-humanized-representation-of-a-number-of-bytes/
def humanize_bytes(numbytes, precision=1):
    abbrevs = (
        (1 << 80, 'YiB'),
        (1 << 70, 'ZiB'),
        (1 << 60, 'EiB'),
        (1 << 50, 'PiB'),
        (1 << 40, 'TiB'),
        (1 << 30, 'GiB'),
        (1 << 20, 'MiB'),
        (1 << 10, 'KiB'),
        (1,       'bytes')
    )
    if numbytes == 1:
        return '1 byte'
    for factor, suffix in abbrevs:
        if numbytes >= factor:
            break
    return '%.*f %s' % (precision, numbytes / factor, suffix)


def get_builds(filesdir):
    # List of patcher files and versions
    files = list()
    versions = list()

    for f in os.listdir(filesdir):
        r = re.search(r'^DualBootPatcher(?:Android)?-(.*)-.*\.(apk|zip|7z)', f)
        if not r:
            r = re.search(r'^DualBootUtilities-(.*)\.zip', f)
        if not r:
            print('Skipping %s ...' % f)
            continue
        files.append(f)
        versions.append(Version(r.group(1)))

    versions = quicksort(list(set(versions)))

    # Remove old builds
    if len(versions) >= maxbuilds:
        for i in range(maxbuilds, len(versions)):
            for f in files:
                if versions[i].ver in f:
                    print('Removing old build %s ...' % f)
                    os.remove(os.path.join(filesdir, f))
        del versions[maxbuilds:]

    # List of builds
    builds = list()

    # Get file list and changelog
    for i in range(0, len(versions)):
        version = versions[i]

        # Get commit log
        process = subprocess.Popen(
            ['git', 'show', '-s', '--format="%cD"', version.commit],
            stdout=subprocess.PIPE,
            universal_newlines=True
        )
        timestamp = process.communicate()[0].split('\n')[0]
        timestamp = timestamp.replace('"', '')

        if not timestamp:
            raise Exception('Failed to determine timestamp for commit: %s'
                            % version.commit)

        # Get list of files
        cur_files = dict()
        md5sums = dict()
        for f in files:
            if version.ver in f:
                if f.endswith('.md5sum'):
                    md5sums[f[:-7]] = f
                    continue

                if f.endswith('.apk'):
                    target = 'Android'
                elif f.endswith('win32.zip') or f.endswith('.7z'):
                    target = 'Win32'
                elif f.startswith('DualBootUtilities'):
                    target = 'Utilities'
                else:
                    target = 'Other'

                if target not in cur_files:
                    cur_files[target] = list()

                cur_files[target].append(f)

        build = dict()
        build['version'] = str(version)
        build['timestamp'] = timestamp
        build['files'] = dict()
        build['commits'] = list()

        # Write files list
        for target in sorted(cur_files):
            for f in cur_files[target]:
                fullpath = os.path.join(filesdir, f)
                size = humanize_bytes(os.path.getsize(fullpath))

                file_info = dict()
                file_info['build'] = f
                file_info['size'] = size

                if f in md5sums:
                    file_info['md5sum'] = md5sums[f]

                build['files'][target] = file_info

        if i < len(versions) - 1:
            new_commit = version.commit
            old_commit = versions[i + 1].commit

            # Get commit log
            process = subprocess.Popen(
                ['git', 'log', '--pretty=format:%H\n%s',
                    '%s..%s' % (old_commit, new_commit)],
                stdout=subprocess.PIPE,
                universal_newlines=True
            )
            log = process.communicate()[0].split('\n')

            if len(log) <= 1:
                raise Exception('Failed to generate changelog from %s to %s'
                                % (old_commit, new_commit))

            for j in range(0, len(log) - 1, 2):
                commit = log[j]
                subject = log[j + 1]

                build['commits'].append({
                    'id': commit,
                    'short_id': commit[0:7],
                    'message': subject
                })

        builds.append(build)

    return builds


def main():
    if len(sys.argv) != 2:
        print('Usage: %s [files directory]' % sys.argv[0])
        sys.exit(1)

    # Files directory
    filesdir = sys.argv[1]
    if not os.path.exists(filesdir):
        os.makedirs(filesdir)

    sourcedir = os.path.dirname(os.path.realpath(__file__))
    resdir = os.path.join(sourcedir, 'res')
    outfile = os.path.join(filesdir, 'index.html')

    builds = get_builds(filesdir)

    env = jinja2.Environment(loader=jinja2.FileSystemLoader(sourcedir),
                             undefined=jinja2.StrictUndefined)
    template = env.get_template('index.template.html')
    html = template.render(builds=builds)

    with open(outfile, 'wb') as f:
        f.write(html.encode('utf-8'))

    distutils.dir_util.copy_tree(resdir, filesdir)


if __name__ == '__main__':
    main()
