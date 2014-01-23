#!/usr/bin/python3

# Generate changelogs and create HTML output

import os
import re
import shutil
import subprocess

name = 'Dual Boot Patcher'
github = 'https://github.com/chenxiaolong/DualBootPatcher'
download = 'http://dl.dropbox.com/u/486665/Snapshots/DualBootPatcher/'
outdir = '/var/lib/jenkins/Dropbox/Public/Snapshots/DualBootPatcher'
html = os.path.join(outdir, 'index.html')
distdir = os.path.join(os.path.dirname(os.path.realpath(__file__)), '..', 'dist')


class Version():
    def __init__(self, version):
        self.ver = version
        s = version.split('.')
        self.p1 = int(s[0])
        self.p2 = int(s[1])
        self.p3 = int(s[2])
        self.p4 = s[3]
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


files = os.listdir(distdir)

versions = list()
for i in files:
    r = re.search(r'^DualBootPatcher(?:Android)?-(.*)\.SNAPSHOT.*\.(apk|zip)', i)
    if not r:
        print('Skipping %s ...' % i)
        continue
    versions.append(Version(r.group(1)))

versions = quicksort(list(set(versions)))

if not os.path.exists(outdir):
    os.makedirs(outdir)

html_file = open(html, 'w')

def write_html(string):
  html_file.write(string + '\n')

html_header = """
<html>
  <head>
    <title>%s builds</title>
  </head>
  <body>
    <h1>%s builds</h1>
""" % (name, name)

html_footer = """
  </body>
</html>
"""

write_html(html_header)

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

    for f in files:
        if version.ver in f:
            shutil.copy(
                os.path.join(distdir, f),
                os.path.join(outdir, f)
            )
            link = download + f

            if f.endswith('apk'):
                filetype = 'apk'
            elif f.endswith('zip'):
                filetype = 'zip'

            write_html('%s: <a href="%s">%s</a><br />' % (filetype, link, f))

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
