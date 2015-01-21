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

import os
import re
import shutil
import subprocess
import sys

name = 'Dual Boot Patcher'
github = 'https://github.com/chenxiaolong/DualBootPatcher'
maxbuilds = 100


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

    def __hash__(self):
        return hash(self.ver)


class HTMLWriter():
    def __init__(self):
        self._file = None
        self._stack = list()

    def open(self, filename):
        self._file = open(filename, 'w')

    def close(self):
        self._file.close()
        self._file = None
        if self._stack:
            raise Exception('Tag stack not empty when closing HTML file: %s'
                            % self._stack)

    def push(self, tag, attrs=None):
        self._stack.append(tag)
        if attrs:
            attrs_str = ' '.join('%s="%s"' % (i, attrs[i]) for i in attrs)
            self._file.write('<%s %s>' % (tag, attrs_str))
        else:
            self._file.write('<%s>' % tag)

    def pop(self, tag):
        top_tag = self._stack.pop()
        if top_tag != tag:
            raise ValueError('Tag <%s> does not match tag at top of'
                             'stack: <%s>' % (tag, top_tag))
        self._file.write('</%s>' % tag)

    def write_tag(self, tag):
        self._file.write('<%s />' % tag)

    def write(self, string):
        self._file.write(string)


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
writer = HTMLWriter()
writer.open(os.path.join(filesdir, 'index.html'))


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


# HTML header boilerplate
writer.push('html')
writer.push('head')
writer.push('title')
writer.write('%s snapshots' % name)
writer.pop('title')
writer.pop('head')
writer.push('body')
writer.push('h1')
writer.write('%s Snapshots (Unstable Builds)' % name)
writer.pop('h1')

# TODO: Remove when complete
writer.push('style')
writer.write('.warning { color: red; font-size: 30px; font-weight: bold; }')
writer.write('.code { font-family: monospace; white-space: pre; }')
writer.pop('style')
writer.write('''
<a class="warning">NOTE:</a>These snapshots now fully support Lollipop (including 'system.new.img' block image installers), but these builds are <b>NOT</b> compatible with ROMs patched with <b>8.0.0.r436.g41f104b</b> or earlier. ROMs patched with older versions will not show up new versions of the app and vice versa.
<br />
<br />
Most people should start off fresh, but if you want to keep the currently multibooted ROMs, follow the steps below.
<br />
<ol>
<li>Move The following directories while booted into recovery or the primary ROM:</li>
<a class="code">
Secondary:
----------
/data/dual            -> /data/multiboot/dual
/cache/dual           -> /cache/multiboot/dual
/system/dual          -> /system/multiboot/dual
/data/media/0/MultiKernels/secondary.img
                      -> /data/media/0/MultiBoot/dual/boot.img

Multi-Slot-X (replacing X with actual number)
------------
/data/multi-slot-X    -> /data/multiboot/multi-slot-X
/system/multi-slot-X  ->  /system/multiboot/multi-slot-X
/cache/multi-slot-X   ->  /cache/multiboot/multi-slot-X
/data/media/0/MultiKernels/multi-slot-X
                      ->  /data/media/0/MultiBoot/multi-slot-X/boot.img
</a>
<br />
<li>Repatch ROM or kernel (don't need both) and reboot</li>
<li>Avoid using older versions of the patcher</li>
</ol>
''')
# TODO: Remove when complete


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

    # Separator
    writer.write_tag('hr')

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

    # Write timestamp
    writer.push('b')
    writer.write(timestamp)
    writer.pop('b')

    writer.write_tag('br')
    writer.write_tag('br')
    writer.push('b')
    writer.write('Files:')
    writer.pop('b')
    writer.write_tag('br')

    # Write files list
    writer.push('ul')
    for f in files:
        if version.ver in f:
            writer.push('li')
            writer.push('a', attrs={'href': f})
            writer.write(f)
            writer.pop('a')
            fullpath = os.path.join(filesdir, f)
            writer.write(' (%s)' % humanize_bytes(os.path.getsize(fullpath)))
            writer.pop('li')
    writer.pop('ul')

    if i == len(versions) - 1:
        writer.write('An earlier build is needed to generate a changelog')
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
        writer.write('WTF?? Failed to generate changelog')
        continue

    writer.push('b')
    writer.write('Changelog:')
    writer.pop('b')
    writer.write_tag('br')

    # Write commit list
    writer.push('ul')
    for j in range(0, len(log) - 1, 2):
        commit = log[j]
        subject = log[j + 1]
        writer.push('li')
        writer.push('a', attrs={'href': '%s/commit/%s' % (github, commit)})
        writer.write(commit[0:7])
        writer.pop('a')
        writer.write(': %s' % subject)
        writer.pop('li')
    writer.pop('ul')


# HTML footer boilerplate
writer.pop('body')
writer.pop('html')
writer.close()
