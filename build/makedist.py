#!/usr/bin/env python3

# Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

import sys
sys.dont_write_bytecode = True

import argparse
import configparser
import getpass
import glob
import hashlib
import io
import os
import pexpect
import re
import shutil
import subprocess
import tarfile
import textwrap
import zipfile
from urllib.request import urlretrieve

basename_android = 'DualBootPatcherAndroid'
basename_pc = 'DualBootPatcher'

# ********************************

# Global variables
buildtype = 'debug'
buildpc = True
buildandroid = False

# ********************************

builddir = os.path.dirname(os.path.realpath(__file__))
topdir = os.path.abspath(os.path.join(builddir, '..'))
distdir = os.path.join(topdir, 'dist')
androiddir = os.path.join(topdir, 'Android_GUI')

# ********************************

config = configparser.ConfigParser()
config.read(os.path.join(builddir, 'build.conf'))

if os.path.exists(os.path.join(builddir, 'build.custom.conf')):
    config.read(os.path.join(builddir, 'build.custom.conf'))

version = config['builder']['version']
android_sdk = config['builder']['android-sdk']
android_ndk = config['builder']['android-ndk']

# ********************************


def run_command(command, stdin_data=None, cwd=None, env=None):
    try:
        process = subprocess.Popen(
            command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd=cwd,
            env=env,
            universal_newlines=True
        )
        output, error = process.communicate(input=stdin_data)

        exit_code = process.returncode

        return (exit_code, output, error)

    except:
        raise Exception("Failed to run command: \"%s\"" % ' '.join(command))


def check_if_failed(exit_status, output=None, error=None, fail_msg='Failed'):
    if exit_status != 0:
        if output or error:
            print('--- Error begin ---')
            if output:
                print('--- stdout begin ---')
                print(output)
                print('--- stdout end ---')
            if error:
                print('--- stderr begin ---')
                print(error)
                print('--- stderr end ---')
            print('--- Error end ---')
        raise Exception(fail_msg)


def download_progress(count, block_size, total_size):
    percent = int(100 * count * block_size / total_size)
    sys.stdout.write('\r%s%%' % percent)
    sys.stdout.flush()


def download(url, filename=None, directory=None, md5=None):
    if not filename and not directory:
        raise Exception('Either filename or directory must be set')

    if filename and directory:
        raise Exception('Filename and directory cannot both be set')

    if directory:
        filename = url.split('/')[-1]
        target = os.path.join(directory, filename)
        parentdir = os.path.abspath(directory)
    else:
        target = filename
        parentdir = os.path.dirname(os.path.abspath(filename))

    if not os.path.exists(target):
        if not os.path.exists(parentdir):
            os.makedirs(parentdir)

        print('Downloading: ' + url)

        urlretrieve(url, filename=target, reporthook=download_progress)
        print('\r' + ' ' * 4 + '\r', end='')

    if md5 is not None and md5sum(target) != md5:
        raise Exception('MD5 checksum of %s does not match' % target)


def md5sum(filename):
    hasher = hashlib.md5()

    with open(filename, 'rb') as f:
        while True:
            buf = f.read(65536)
            if not buf:
                break
            hasher.update(buf)

    return hasher.hexdigest()


def extract_tar(filename, destdir, files=None, regex=None):
    if not os.path.exists(destdir):
        os.makedirs(destdir)

    if not tarfile.is_tarfile(filename):
        raise Exception('%s is not a tar file' % filename)

    print('Extracting %s ...' % filename)

    with tarfile.open(filename) as tar:
        if files:
            for member in tar.getmembers():
                if (files is not None and member.name in files) or \
                        (regex is not None and re.search(regex, member.name)):
                    member.name = os.path.basename(member.name)
                    tar.extract(member, path=destdir)
        else:
            tar.extractall(destdir)


def upx_compress(files, lzma=True, force=False):
    if type(files) == str:
        files = [files]

    command = ['upx', '-v']

    if lzma:
        command.append('--lzma')
    else:
        command.append('-9')

    if force:
        command.append('--force')

    command.extend(files)

    print('Compressing the following with UPX ...')
    for f in files:
        print('  ' + f)

    exit_status, output, error = run_command(command)
    check_if_failed(exit_status, output, error,
                    'Failed to compress files using upx: ' + str(files))


def get_target_info(android=False):
    targetname = ''
    targetdir = ''
    targetfile = ''

    if android:
        targetname = '%s-%s' % (basename_android, version)
    else:
        suffix = ''
        if buildtype == 'debug':
            suffix = '-debug'
        elif buildtype == 'release':
            suffix = '-release'
        elif buildtype == 'ci':
            suffix = '-snapshot'
        targetname = '%s-%s%s' % (basename_pc, version, suffix)

    if android:
        fileext = 'tar.xz'
    elif buildtype == 'ci':
        fileext = '7z'
    else:
        fileext = 'zip'

    targetfile = os.path.join(distdir, '%s.%s' % (targetname, fileext))
    targetdir = os.path.join(distdir, targetname)

    return (targetname, targetdir, targetfile)


def create_android_toolchain():
    print('Creating Android NDK toolchain ...')

    envscript = os.path.join(builddir, 'compile', 'env.sh')

    exit_status, output, error = run_command(
        ['bash', '-c', 'source %s; setup_toolchain' % envscript]
    )

    check_if_failed(exit_status, output, error,
                    'Failed to create Android NDK toolchain')


def create_syncdaemon(targetdir):
    create_android_toolchain()
    print('Compiling syncdaemon ...')

    syncdaemondir = os.path.join(builddir, 'syncdaemon')
    ramdisksdir = os.path.join(targetdir, 'ramdisks')

    exit_status, output, error = run_command(
        [os.path.join(syncdaemondir, 'build.sh')]
    )

    check_if_failed(exit_status, output, error,
                    'Failed to compile syncdaemon')

    shutil.move(os.path.join(syncdaemondir, 'syncdaemon'),
                os.path.join(ramdisksdir, 'syncdaemon'))


def create_acl_attr_android(targetdir):
    print('Compiling acl and attr ...')

    compiledir = os.path.join(builddir, 'compile')
    patchesdir = os.path.join(targetdir, 'patches')

    exit_status, output, error = run_command(
        [os.path.join(compiledir, 'build-attr-acl.sh')],
        cwd=compiledir
    )

    check_if_failed(exit_status, output, error,
                    'Failed to compile acl and attr')

    shutil.move(os.path.join(compiledir, 'getfacl'),
                os.path.join(patchesdir, 'getfacl'))
    shutil.move(os.path.join(compiledir, 'getfattr'),
                os.path.join(patchesdir, 'getfattr'))
    shutil.move(os.path.join(compiledir, 'setfacl'),
                os.path.join(patchesdir, 'setfacl'))
    shutil.move(os.path.join(compiledir, 'setfattr'),
                os.path.join(patchesdir, 'setfattr'))


def create_release(targetdir, targetfile, android=False):
    print('Creating release archive ...')

    copy_for_rel = [
        'README.jflte.txt',
        'patch-file.sh',
        'patches/',
        'patchinfo/',
        'ramdisks/',
        'scripts/',
        'useful/'
    ]

    if android:
        copy_for_rel.remove('README.jflte.txt')
        copy_for_rel.remove('patch-file.sh')
        copy_for_rel.remove('useful/')

    for f in copy_for_rel:
        if os.path.isdir(os.path.join(topdir, f)):
            shutil.copytree(os.path.join(topdir, f),
                            os.path.join(targetdir, f))
        else:
            shutil.copy2(os.path.join(topdir, f), targetdir)

    create_syncdaemon(targetdir)
    create_acl_attr_android(targetdir)

    if buildtype == 'release':
        upx_compress([os.path.join(targetdir, 'ramdisks', 'busybox-static')])
        upx_compress(glob.glob(targetdir + os.sep + 'ramdisks' + os.sep +
                               'init' + os.sep + 'init-*'))
        upx_compress(glob.glob(targetdir + os.sep + 'ramdisks' + os.sep +
                               'init' + os.sep + 'jflte' + os.sep + '*'))

    with open(os.path.join(topdir, 'patcher.yaml.in')) as f_in:
        with open(os.path.join(targetdir, 'patcher.yaml'), 'w') as f_out:
            for line in f_in:
                f_out.write(line.replace('@VERSION@', version))

    if android:
        remove = [
            'patchinfo/README',
            'patchinfo/Sample.py',
            'scripts/qtmain.py',
        ]

        for f in remove:
            os.remove(os.path.join(targetdir, f))

    if not android:
        shutil.copyfile(os.path.join(androiddir, 'ic_launcher-web.png'),
                        os.path.join(targetdir, 'scripts', 'icon.png'))

    if android:
        with tarfile.open(targetfile, 'w:xz') as t:
            t.add(targetdir, arcname=os.path.basename(targetdir))

    elif buildtype == 'ci':
        exit_status, output, error = run_command(
            ['7z', 'a', '-t7z', '-m0=lzma', '-mx=9', '-mfb=64', '-md=32m',
             '-ms=on', targetfile, targetdir],
            cwd=distdir
        )

        check_if_failed(exit_status, output, error,
                        'Failed to compress output directory with p7zip')

    else:
        z = zipfile.ZipFile(targetfile, 'w', zipfile.ZIP_DEFLATED)
        for root, dirs, files in os.walk(targetdir):
            for f in files:
                arcdir = os.path.relpath(root, start=distdir)
                z.write(os.path.join(root, f),
                        arcname=os.path.join(arcdir, f))
        z.close()


def build_pc():
    targetname, targetdir, targetfile = get_target_info()

    if os.path.exists(targetfile):
        print('Removing old archive ...')
        os.remove(targetfile)

    if os.path.exists(targetdir):
        print('Removing old distribution ...')
        shutil.rmtree(targetdir)
    os.makedirs(targetdir)

    create_release(targetdir, targetfile)

    shutil.rmtree(targetdir)


def build_android_app(targetname):
    if buildtype == 'release':
        suffix = '-signed'
    elif buildtype == 'debug':
        suffix = '-debug'
    else:
        suffix = '-snapshot'

    apkname = targetname + suffix + '.apk'

    print('Removing old apk ...')
    if os.path.exists(os.path.join(distdir, apkname)):
        os.remove(os.path.join(distdir, apkname))

    gradle = os.path.join(androiddir, 'build.gradle')
    with open(gradle + '.in') as f_in:
        with open(os.path.join(gradle), 'w') as f_out:
            for line in f_in:
                f_out.write(re.sub(r'@VERSION@', version, line))

    # Add JNI stuff
    print('Setting up JNI sources ...')
    exit_status, output, error = run_command(
        [os.path.join(builddir, 'compile', 'build-jni-libs.sh')]
    )

    if buildtype == 'release':
        task = 'assembleRelease'
        apkfile = 'Android_GUI-release.apk'

    elif buildtype == 'debug':
        task = 'assembleDebug'
        apkfile = 'Android_GUI-debug-unaligned.apk'

    elif buildtype == 'ci':
        task = 'assembleCi'
        apkfile = 'Android_GUI-ci.apk'

    print('Compiling Android app ...')
    output = io.StringIO()
    sdkenv = os.environ.copy()
    sdkenv['ANDROID_HOME'] = android_sdk
    sdkenv['ANDROID_NDK_HOME'] = android_ndk
    sdkenv['TERM'] = 'xterm'
    sdkenv['PATH'] += os.pathsep + android_ndk
    child = pexpect.spawnu(androiddir + '/gradlew', [task],
                           cwd=androiddir, env=sdkenv, timeout=600)
    child.logfile = output

    if buildtype == 'release':
        msg_keystorepw = 'Enter keystore password: '
        msg_alias = 'Enter alias key: '
        msg_keypw = 'Enter key password: '
        keystorepw = getpass.getpass(msg_keystorepw)
        alias = input(msg_alias)
        keypw = getpass.getpass(msg_keypw)

        child.expect_exact([msg_keystorepw])
        child.sendline(keystorepw)
        child.expect_exact([msg_alias])
        child.sendline(alias)
        child.expect_exact([msg_keypw])
        child.sendline(keypw)
        child.expect(pexpect.EOF)
        output.seek(0)
        output = output.read()

        removed = '[removed]'
        output = output.replace(keystorepw, removed)
        output = output.replace(alias, removed)
        output = output.replace(keypw, removed)

    else:
        child.expect(pexpect.EOF)
        output.seek(0)
        output = output.read()

    child.close()

    check_if_failed(child.exitstatus, output, None,
                    'Failed to compile Android app')

    shutil.move(os.path.join(androiddir, 'build', 'outputs', 'apk', apkfile),
                os.path.join(distdir, apkname))


def build_android():
    targetname, targetdir, targetfile = get_target_info(android=True)

    if os.path.exists(targetfile):
        print('Removing old archive ...')
        os.remove(targetfile)

    if os.path.exists(targetdir):
        print('Removing old distribution ...')
        shutil.rmtree(targetdir)
    os.makedirs(targetdir)

    create_release(targetdir, targetfile, android=True)

    # Build the app
    assetdir = os.path.join(androiddir, 'assets')
    if os.path.exists(assetdir):
        shutil.rmtree(assetdir)
    os.makedirs(assetdir)
    shutil.move(targetfile, assetdir)
    shutil.copyfile(os.path.join(topdir, 'ramdisks', 'busybox-static'),
                    os.path.join(assetdir, 'busybox-static'))

    if buildtype == 'release':
        os.chmod(os.path.join(assetdir, 'busybox-static'), 0o0755)
        #upx_compress([os.path.join(assetdir, 'busybox-static')])

    build_android_app(targetname)

    shutil.rmtree(targetdir)


def parse_args():
    global buildtype, buildandroid, buildpc

    parser = argparse.ArgumentParser()
    parser.formatter_class = argparse.RawDescriptionHelpFormatter
    parser.description = textwrap.dedent('''
    Dual Boot Patcher Build Script
    ------------------------------
    ''')

    parser.add_argument('-a', '--android',
                        help='Build Android app',
                        action='store_true')
    parser.add_argument('--no-pc',
                        help='Do not build PC version',
                        action='store_true')

    group = parser.add_mutually_exclusive_group()
    group.add_argument('--debug',
                       help='Build in debug mode (default)',
                       action='store_const',
                       const='debug',
                       dest='buildtype')
    group.add_argument('--release',
                       help='Build in release mode',
                       action='store_const',
                       const='release',
                       dest='buildtype')
    group.add_argument('--ci',
                       help=argparse.SUPPRESS,
                       action='store_const',
                       const='ci',
                       dest='buildtype')

    args = parser.parse_args()

    if args.buildtype:
        buildtype = args.buildtype

    if args.android:
        buildandroid = True

    if args.no_pc:
        buildpc = False

    return True


# Avoids popups from wine
os.environ['DISPLAY'] = ''


if not parse_args():
    sys.exit(1)

print('Version:        ' + version)
print('Build type:     ' + buildtype)

try:
    if buildpc:
        build_pc()

    if buildandroid:
        build_android()

    print('Done')
except Exception as e:
    import traceback
    traceback.print_exc()
    print(str(e))
    sys.exit(1)
