#!/usr/bin/python3

# Generate changelogs and create HTML output

import os
import re
import shutil
import subprocess

name = 'Dual Boot Patcher'
github = 'https://github.com/chenxiaolong/DualBootPatcher'
outdir = '/var/lib/jenkins/Dropbox/Public/Snapshots/DualBootPatcher'
html = os.path.join(outdir, 'index.html')
maxbuilds = 5


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


def quicksort(l):
    if l == []:
        return []
    else:
        pivot = l[0]
        left = quicksort([x for x in l[1:] if x >= pivot])
        right = quicksort([x for x in l[1:] if x < pivot])
        return left + [pivot] + right


if not os.path.exists(outdir):
    os.makedirs(outdir)

files = os.listdir(outdir)

versions = list()
for i in files:
    r = re.search(r'^DualBootPatcher(?:Android)?-(.*)\.SNAPSHOT.*\.(apk|zip|7z)', i)
    if not r:
        print('Skipping %s ...' % i)
        continue
    versions.append(Version(r.group(1)))

versions = quicksort(list(set(versions)))

html_file = open(html, 'w')

def write_html(string):
  html_file.write(string + '\n')

html_header = """
<html>
  <head>
    <title>%s snapshots</title>
  </head>
  <body>
    <h1>%s Snapshots (Unstable Builds)</h1>
""" % (name, name)

html_footer = """
  </body>
</html>
"""

write_html(html_header)

# Remove old builds
if len(versions) >= maxbuilds:
    for i in range(maxbuilds, len(versions)):
        for f in files:
            if versions[i].ver in f:
                print("Removing old build %s ..." % f)
                os.remove(os.path.join(outdir, f))
    del versions[maxbuilds:]

for i in range(0, len(versions)):
    version = versions[i]

    write_html('<p>')

    # Get commit log
    process = subprocess.Popen(
        [ 'git', 'show', '-s',
            '--format="%cD"',
            version.commit ],
        stdout = subprocess.PIPE,
        universal_newlines = True
    )
    timestamp = process.communicate()[0].split('\n')[0]
    timestamp = re.sub(r'"', '', timestamp)

    write_html('<b>%s<br /><br /></b>' % timestamp)

    android = list()
    pc = list()
    for f in files:
        if version.ver in f:
            if f.endswith('apk'):
                android.append(f)
            elif f.endswith('zip') or f.endswith('7z'):
                pc.append(f)

    android.sort()
    pc.sort()

    for f in android:
        write_html('Android: <a href="%s">%s</a><br />' % (f, f))

    for f in pc:
        write_html('PC: <a href="%s">%s</a><br />' % (f, f))

    write_html('</p>')

    if i == len(versions) - 1:
        write_html('An earlier build is needed to generate a changelog')
        continue

    new_commit = version.commit
    old_commit = versions[i + 1].commit

    # Get commit log
    process = subprocess.Popen(
        [ 'git', 'log',
            '--pretty=format:%H\n%s',
            '%s..%s' % (old_commit, new_commit) ],
        stdout = subprocess.PIPE,
        universal_newlines = True
    )
    log = process.communicate()[0].split('\n')

    write_html('<ul>')

    if len(log) <= 1:
        write_html("WTF something went wrong with the script ...")
        continue

    for j in range(0, len(log) - 1, 2):
        commit = log[j]
        subject = log[j + 1]
        write_html('<li><a href="%s/commit/%s">%s</a>: %s</li>'
            % (github, commit, commit[0:7], subject))

    write_html('</ul>')

    write_html('<hr />')

write_html(html_footer)
html_file.close()
