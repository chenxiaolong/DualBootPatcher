#!/usr/bin/python3

# Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import html
import os
import re
import subprocess
import sys

name = 'Dual Boot Patcher'
github = 'https://github.com/chenxiaolong/DualBootPatcher'
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


class HTMLWriter():
    def __init__(self, initial_indent=0):
        self._file = None
        self._stack = list()
        self._initial_indent = initial_indent

    def open(self, filename):
        self._file = open(filename, 'w')

    def close(self):
        self._file.close()
        self._file = None
        if self._stack:
            raise Exception('Tag stack not empty when closing HTML file: %s'
                            % self._stack)

    def push(self, tag, attrs=None, newline=False, indent=False):
        if indent:
            self._file.write('    ' * (len(self._stack) +
                                       self._initial_indent))

        self._stack.append(tag)
        if attrs:
            attrs_str = ' '.join('%s="%s"' % (i, attrs[i]) for i in attrs)
            self._file.write('<%s %s>' % (tag, attrs_str))
        else:
            self._file.write('<%s>' % tag)

        if newline:
            self._file.write('\n')

    def pop(self, tag, newline=False, indent=False):
        if indent:
            self._file.write('    ' * (len(self._stack) +
                                       self._initial_indent - 1))

        top_tag = self._stack.pop()
        if top_tag != tag:
            raise ValueError('Tag <%s> does not match tag at top of'
                             'stack: <%s>' % (tag, top_tag))
        self._file.write('</%s>' % tag)

        if newline:
            self._file.write('\n')

    def write_tag(self, tag):
        self._file.write('<%s />' % tag)

    def write(self, string, escape=True):
        if escape:
            self._file.write(html.escape(string))
        else:
            self._file.write(string)

    def newline(self):
        self._file.write('\n')


def quicksort(l):
    if l == []:
        return []
    else:
        pivot = l[0]
        left = quicksort([x for x in l[1:] if x >= pivot])
        right = quicksort([x for x in l[1:] if x < pivot])
        return left + [pivot] + right


# From http://code.activestate.com/recipes/577081-humanized-representation-of-a-number-of-bytes/
def humanize_bytes(bytes, precision=1):
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
    if bytes == 1:
        return '1 byte'
    for factor, suffix in abbrevs:
        if bytes >= factor:
            break
    return '%.*f %s' % (precision, bytes / factor, suffix)


if len(sys.argv) != 2:
    print('Usage: %s [files directory]' % sys.argv[0])
    sys.exit(1)


# Files directory
filesdir = sys.argv[1]
if not os.path.exists(filesdir):
    os.makedirs(filesdir)


# HTML output
writer = HTMLWriter(initial_indent=3)
writer.open(os.path.join(filesdir, '.index.gen.html'))


# List of patcher files and versions
files = list()
versions = list()

for f in os.listdir(filesdir):
    r = re.search(r'^DualBootPatcher(?:Android)?-(.*)-.*\.(apk|zip|7z)', f)
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


# Write file list and changelog
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
        writer.write('Failed to determine timestamp for commit: %s'
                     % version.commit)

    # Write block
    writer.push('div', attrs={'class': 'panel panel-success'},
                newline=True, indent=True)

    # Write timestamp
    writer.push('div', attrs={'class': 'panel-heading'},
                newline=True, indent=True)
    writer.push('h2', attrs={'class': 'panel-title',
                             'style': 'font-size:20px;'}, indent=True)
    writer.write(str(version))
    writer.write(' ')
    writer.push('small')
    writer.write(timestamp)
    writer.pop('small')
    writer.pop('h2', newline=True)
    writer.pop('div', indent=True, newline=True)

    writer.push('div', attrs={'class': 'panel-body'},
                newline=True, indent=True)

    # "Files" title
    writer.push('h4', indent=True)
    writer.push('span', attrs={'class': 'mdi mdi-file-folder'})
    writer.pop('span')
    writer.write(' Files:')
    writer.pop('h4', newline=True)

    # Write files list
    writer.push('ul', indent=True, newline=True)
    for f in files:
        if version.ver in f:
            writer.push('li', indent=True)
            writer.push('a', attrs={'href': f})
            writer.write(f)
            writer.pop('a')
            fullpath = os.path.join(filesdir, f)
            writer.write(' (%s)' % humanize_bytes(os.path.getsize(fullpath)))
            writer.pop('li', newline=True)
    writer.pop('ul', indent=True, newline=True)

    if i == len(versions) - 1:
        writer.push('p', indent=True)
        writer.write('An earlier build is needed to generate a changelog')
        writer.pop('p', newline=True)

        writer.pop('div', indent=True, newline=True)
        writer.pop('div', indent=True, newline=True)
        continue

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
        writer.push('p', indent=True)
        writer.write('WTF?? Failed to generate changelog')
        writer.pop('p', newline=True)

        writer.pop('div', indent=True, newline=True)
        writer.pop('div', indent=True, newline=True)
        continue

    # "Changelog" title
    writer.push('h4', indent=True)
    writer.push('span', attrs={'class': 'mdi mdi-action-schedule'})
    writer.pop('span')
    writer.write(' Changelog:')
    writer.pop('h4', newline=True)

    writer.push('table', attrs={'class': 'table table-striped table-hover'},
                indent=True, newline=True)
    writer.push('thead', indent=True, newline=True)
    writer.push('tr', indent=True, newline=True)
    writer.push('th', indent=True)
    writer.write('Commit')
    writer.pop('th', newline=True)
    writer.push('th', indent=True)
    writer.write('Description')
    writer.pop('th', newline=True)
    writer.pop('tr', indent=True, newline=True)
    writer.pop('thead', indent=True, newline=True)
    writer.push('tbody', indent=True, newline=True)

    # Write commit list
    for j in range(0, len(log) - 1, 2):
        commit = log[j]
        subject = log[j + 1]
        writer.push('tr', indent=True, newline=True)
        writer.push('td', indent=True)
        writer.push('a', attrs={'href': '%s/commit/%s' % (github, commit)})
        writer.write(commit[0:7])
        writer.pop('a')
        writer.pop('td', newline=True)
        writer.push('td', indent=True)
        writer.write(subject)
        writer.pop('td', newline=True)
        writer.pop('tr', indent=True, newline=True)

    writer.pop('tbody', indent=True, newline=True)
    writer.pop('table', indent=True, newline=True)

    writer.pop('div', indent=True, newline=True)
    writer.pop('div', indent=True, newline=True)


writer.close()


sourcedir = os.path.dirname(os.path.realpath(__file__))
resdir = os.path.join(sourcedir, 'res')
templatefile = os.path.join(sourcedir, 'index.template.html')
outfile = os.path.join(filesdir, 'index.html')
genfile = os.path.join(filesdir, '.index.gen.html')
MAGIC = b'INSERT GENERATED STUFF HERE\n'

f_out = open(outfile, 'wb')

with open(templatefile, 'rb') as f_in:
    for line in f_in:
        if line == MAGIC:
            with open(genfile, 'rb') as f_gen:
                for gen_line in f_gen:
                    f_out.write(gen_line)
        else:
            f_out.write(line)

os.remove(genfile)

distutils.dir_util.copy_tree(resdir, filesdir)
