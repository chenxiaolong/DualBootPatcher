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
import tempfile
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
win32_launcher = config['builder']['win32_launcher']
mingw_prefix = config['builder']['mingw_prefix']

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


def download(url, filename=None, directory=None):
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

    if not os.path.exists(parentdir):
        os.makedirs(parentdir)

    urlretrieve(url, filename=target, reporthook=download_progress)
    print('\r' + ' ' * 4 + '\r', end='')


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

    with tarfile.open(filename) as tar:
        if files:
            for member in tar.getmembers():
                if (files is not None and member.name in files) or \
                        (regex is not None and re.search(regex, member.name)):
                    member.name = os.path.basename(member.name)
                    tar.extract(member, path=destdir)
        else:
            tar.extractall(destdir)


def remove_files(listfile, basedir):
    missing = list()

    with open(listfile, 'r') as f:
        for line in f:
            if line:
                line = line.strip('\n')
            if not line:
                continue

            toremove = os.path.join(basedir, line)

            if not os.path.exists(toremove):
                missing.append(line)
                continue

            isdir = False
            if line.endswith('/'):
                isdir = True
            elif os.path.isdir(toremove):
                isdir = True

            if isdir:
                shutil.rmtree(toremove)
            else:
                os.remove(toremove)

    if missing:
        raise Exception('Fix %s: these are missing: %s' %
                        (listfile, str(missing)))


def upx_compress(files, lzma=True):
    if type(files) == str:
        files = [files]

    if lzma:
        command = ['upx', '-v', '--lzma']
    else:
        command = ['upx', '-v', '-9']
    command.extend(files)

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


def create_python_windows(targetdir):
    pyver = '3.4.0'
    url = 'http://python.org/ftp/python/%s/python-%s.msi' % (pyver, pyver)
    md5 = 'e3be8a63294e42e126493ca96cfe48bd'

    pyinst = os.path.join(builddir, 'windowsbinaries', 'python-%s.msi' % pyver)
    pyport = os.path.join(targetdir, 'pythonportable')
    removefiles = os.path.join(builddir, 'remove.winpython.txt')

    if not os.path.exists(pyinst):
        print('Downloading Python %s for Windows ...' % pyver)
        download(url, filename=pyinst)

    print('Checking MD5 of Windows Python installer ...')
    if md5sum(pyinst) != md5:
        raise Exception('MD5 checksum does not match')

    print('Getting Windows paths ...')
    exit_status, output, error = run_command(
        ['winepath', '-w', pyinst, pyport]
    )

    check_if_failed(exit_status, output, error,
                    'Failed to get Windows paths using winepath')

    winpaths = output.split('\n')
    winpyinst = winpaths[0]
    winpyport = winpaths[1]

    print('Extracting Windows Python installer ...')
    exit_status, output, error = run_command(
        ['wine', 'msiexec', '/a', winpyinst, '/qb', 'TARGETDIR=' + winpyport]
    )

    check_if_failed(exit_status, output, error,
                    'Failed to extract Python installer')

    print('Removing unneeded scripts and files ...')
    remove_files(removefiles, pyport)
    for root, dirs, files in os.walk(pyport):
        if '__pycache__' in dirs:
            shutil.rmtree(os.path.join(root, '__pycache__'))


def create_python_android(targetdir):
    pyver = '3.4.1'
    filename = 'python-install-%s.tar.xz' % pyver
    url = 'http://fs1.d-h.st/download/00122/wHx/' + filename
    md5 = 'a4c037587da9488d40fc01be4839afc8'

    pytar = os.path.join(builddir, 'androidbinaries', filename)
    pyport = os.path.join(targetdir, 'pythonportable')
    removefiles = os.path.join(builddir, 'remove.androidpython.txt')

    if not os.path.exists(pytar):
        print('Downloading Python %s for Android ...' % pyver)
        download(url, filename=pytar)

    print('Checking MD5 of Android Python archive ...')
    if md5sum(pytar) != md5:
        raise Exception('MD5 checksum does not match')

    tempdir = tempfile.mkdtemp()
    extract_tar(pytar, tempdir)
    shutil.move(os.path.join(tempdir, 'python-install'), pyport)
    shutil.rmtree(tempdir)

    print('Removing unneeded scripts and files ...')
    remove_files(removefiles, pyport)


def create_pyyaml(targetdir, pysitelib):
    yamlver = '3.11'
    filename = 'PyYAML-%s.tar.gz' % yamlver
    url = 'http://pyyaml.org/download/pyyaml/' + filename
    md5 = 'f50e08ef0fe55178479d3a618efe21db'

    tarball = os.path.join(builddir, 'androidbinaries', filename)

    if not os.path.exists(tarball):
        print('Downloading PyYAML %s ...' % yamlver)
        download(url, filename=tarball)

    print('Checking MD5 of PyYAML tarball ...')
    if md5sum(tarball) != md5:
        raise Exception('MD5 checksum does not match')

    tempdir = tempfile.mkdtemp()
    extract_tar(tarball, tempdir)
    shutil.move(os.path.join(tempdir, 'PyYAML-' + yamlver, 'lib3', 'yaml'),
                os.path.join(pysitelib, 'yaml'))
    shutil.rmtree(tempdir)


def create_pyqt_windows(targetdir):
    filename = 'PyQt5_QtBase5.2.1_Python3.4.0_win32_msvc2010.7z'
    url = 'http://fs1.d-h.st/download/00115/LGr/' + filename
    md5 = '478a2151c1036e27462e324703ad9287'

    pyqtinst = os.path.join(builddir, 'windowslibraries', filename)
    pyport = os.path.join(targetdir, 'pythonportable')
    pysitelib = os.path.join(pyport, 'Lib', 'site-packages')
    pyqtdir = os.path.join(pysitelib, 'PyQt5')
    qtconf = os.path.join(pyport, 'qt.conf')

    if not os.path.exists(pyqtinst):
        print('Downloading PyQt 5.2.1 (for Python 3.4.0) for Windows ...')
        download(url, filename=pyqtinst)

    print('Checking MD5 of PyQt5 archive ...')
    if md5sum(pyqtinst) != md5:
        raise Exception('MD5 checksum does not match')

    print('Extracting PyQt5 archive ...')
    exit_status, output, error = run_command(
        ['7z', 'x', '-o' + pysitelib, pyqtinst]
    )

    check_if_failed(exit_status, output, error,
                    'Failed to extract PyQt5 archive using p7zip')

    print('Compressing PyQt5 libraries with UPX ...')
    if buildtype != 'ci':
        upx_compress(glob.glob(pyqtdir + os.sep + '*.dll'))

    print('Create qt.conf configuration file ...')
    with open(qtconf, 'w') as f:
        f.write('[Paths]\n')
        f.write('Prefix = Lib/site-packages/PyQt5\n')
        f.write('Binaries = Lib/site-packages/PyQt5\n')


def create_binaries_windows(targetdir):
    binpath = os.path.join(builddir, 'windowsbinaries')
    tbinpath = os.path.join(targetdir, 'binaries', 'windows')

    # Our mini-Cygwin :)
    baseurl = "http://mirrors.kernel.org/sourceware/cygwin/x86/release"

    f_cygwin = 'cygwin-1.7.28-2.tar.xz'
    f_libintl = 'libintl8-0.18.1.1-2.tar.bz2'
    f_libiconv = 'libiconv2-1.14-2.tar.bz2'
    f_patch = 'patch-2.7.1-1.tar.bz2'
    f_diffutils = 'diffutils-3.2-1.tar.bz2'

    urls = [
        baseurl + '/cygwin/' + f_cygwin,
        baseurl + '/gettext/libintl8/' + f_libintl,
        baseurl + '/libiconv/libiconv2/' + f_libiconv,
        baseurl + '/patch/' + f_patch,
        baseurl + '/diffutils/' + f_diffutils
    ]

    for url in urls:
        filename = url.split('/')[-1]
        if not os.path.exists(os.path.join(binpath, filename)):
            print('Downloading %s ...' % filename)
            download(url, directory=binpath)

    print('Extracting cygwin libraries and binaries ...')
    extract_tar(
        os.path.join(binpath, f_cygwin),
        tbinpath,
        files=['usr/bin/cygwin1.dll']
    )
    extract_tar(
        os.path.join(binpath, f_libintl),
        tbinpath,
        files=['usr/bin/cygintl-8.dll']
    )
    extract_tar(
        os.path.join(binpath, f_libiconv),
        tbinpath,
        files=['usr/bin/cygiconv-2.dll']
    )
    extract_tar(
        os.path.join(binpath, f_patch),
        tbinpath,
        files=['usr/bin/patch.exe']
    )
    shutil.move(os.path.join(tbinpath, 'patch.exe'),
                os.path.join(tbinpath, 'hctap.exe'))
    extract_tar(
        os.path.join(binpath, f_diffutils),
        tbinpath,
        files=['usr/bin/diff.exe']
    )

    skip = ['cygwin1.dll']

    binfiles = glob.glob(tbinpath + os.sep + '*.exe')
    dllfiles = glob.glob(tbinpath + os.sep + '*.dll')

    for f in binfiles + dllfiles:
        os.chmod(f, 0o0755)

    if buildtype != 'ci':
        compress = binfiles + dllfiles
        for f in compress:
            for s in skip:
                if f.endswith(os.sep + s):
                    compress.remove(f)

        print('Compressing cygwin libraries and binaries with UPX ...')
        upx_compress(compress, lzma=False)


def create_binaries_android(targetdir):
    binpath = os.path.join(builddir, 'androidbinaries')
    tbinpath = os.path.join(targetdir, 'binaries', 'android')

    os.makedirs(tbinpath)

    tempdir = tempfile.mkdtemp()

    print('Creating Android NDK toolchain ...')
    exit_status, output, error = run_command(
        ['bash', os.path.join(android_ndk, 'build', 'tools',
                              'make-standalone-toolchain.sh'),
         '--verbose', '--platform=android-18',
         '--install-dir=' + tempdir,
         '--ndk-dir=' + android_ndk,
         '--system=linux-x86_64']
    )

    check_if_failed(exit_status, output, error,
                    'Failed to create Android NDK toolchain')

    filename = 'patch-2.7.tar.xz'
    url = 'ftp://ftp.gnu.org/gnu/patch/' + filename
    if not os.path.exists(os.path.join(binpath, filename)):
        print('Downloading %s ...' % filename)
        download(url, directory=binpath)

    patchtar = os.path.join(binpath, filename)
    extract_tar(patchtar, tempdir)
    patchdir = os.path.join(tempdir, 'patch-2.7')

    envpath = os.environ.copy()
    envpath['PATH'] += os.pathsep + os.path.join(tempdir, 'bin')

    exit_status, output, error = run_command(
        ['./configure', '--host=arm-linux-androideabi'],
        cwd=patchdir,
        env=envpath
    )

    check_if_failed(exit_status, output, error,
                    'Failed to run ./configure for patch-2.7')

    exit_status, output, error = run_command(
        ['make'],
        cwd=patchdir,
        env=envpath
    )

    check_if_failed(exit_status, output, error,
                    'Failed to compile patch-2.7')

    shutil.copy2(os.path.join(patchdir, 'src', 'patch'), tbinpath)

    shutil.rmtree(tempdir)


def create_shortcuts_windows(targetdir):
    fail_compile = 'Failed to compile Windows launcher'

    bingui = os.path.join(targetdir, 'PatchFileWindowsGUI.exe')
    bincli = os.path.join(targetdir, 'PatchFileWindows.exe')
    manifest = os.path.join(builddir, 'shortcuts', 'windows-exe.manifest')

    tempdir = tempfile.mkdtemp()

    icon = os.path.join(androiddir, 'ic_launcher-web.png')
    icon_sizes = [256, 64, 48, 40, 32, 24, 20, 16]

    print('Generating icons for Windows ...')
    for size in icon_sizes:
        exit_status, output, error = run_command(
            ['convert', '-resize', '%dx%d' % (size, size), icon,
             'output-%d.ico' % size],
            cwd=tempdir
        )

        check_if_failed(exit_status, output, error,
                        'Failed to convert icon using ImageMagick')

    icon_files = ['output-%d.ico' % x for x in icon_sizes]
    exit_status, output, error = run_command(
        ['convert'] + icon_files + ['combined.ico'],
        cwd=tempdir
    )

    check_if_failed(exit_status, output, error,
                    'Failed to combine icons into a single .ico file')

    # Compile launcher
    print('Compiling Windows launcher executables ...')
    if win32_launcher == 'C++':
        with open(os.path.join(tempdir, 'res.rc'), 'w') as f:
            f.write('1 ICON "combined.ico"\n')
            # 24 = RT_MANIFEST, but windres doesn't seem to like it
            f.write('1 24 \"%s\"' % manifest)

        exit_status, output, error = run_command(
            [mingw_prefix + 'windres', 'res.rc', 'res.o'],
            cwd=tempdir
        )

        check_if_failed(exit_status, output, error,
                        'Failed to compile Windows executable resource file')

        cxxflags_gui = '-static -Wl,-subsystem,windows'
        cxxflags_cli = '-static'

        gpp = mingw_prefix + 'g++'
        strip = mingw_prefix + 'strip'

        exit_status, output, error = run_command(
            [gpp, cxxflags_gui, '-DGUI', '-DSCRIPT="scripts\\\\qtmain.py"',
             os.path.join(builddir, 'shortcuts', 'windows-exe.cpp'),
             os.path.join(tempdir, 'res.o'),
             '-lshlwapi',
             '-o', bingui]
        )

        check_if_failed(exit_status, output, error, fail_compile)

        exit_status, output, error = run_command(
            [gpp, cxxflags_cli, '-DSCRIPT="scripts\\\\patchfile.py"',
             os.path.join(builddir, 'shortcuts', 'windows-exe.cpp'),
             os.path.join(tempdir, 'res.o'),
             '-lshlwapi',
             '-o', bincli]
        )

        check_if_failed(exit_status, output, error, fail_compile)

        exit_status, output, error = run_command(
            [strip, bingui, bincli]
        )

        check_if_failed(exit_status, output, error,
                        'Failed to strip symbols from Windows launchers')

        print('Compressing Windows launcher with UPX ...')
        if buildtype != 'ci':
            upx_compress([bingui, bincli])

    elif win32_launcher == 'C#':
        exit_status, output, error = run_command(
            ['gmcs', os.path.join(builddir, 'shortcuts', 'windows-exe.cs'),
             '-win32icon:' + os.path.join(tempdir, 'combined.ico'),
             '-target:winexe', '-define:GUI',
             '-out:' + bingui]
        )

        check_if_failed(exit_status, output, error, fail_compile)

        exit_status, output, error = run_command(
            ['gmcs', os.path.join(builddir, 'shortcuts', 'windows-exe.cs'),
             '-win32icon:' + os.path.join(tempdir, 'combined.ico'),
             '-out:' + bincli]
        )

        check_if_failed(exit_status, output, error, fail_compile)

    shutil.rmtree(tempdir)


def create_shortcuts_linux(targetdir):
    bingui = os.path.join(targetdir, 'PatchFileLinuxGUI')

    print('Compiling Linux launcher executables ...')
    exit_status, output, error = run_command(
        ['gcc', '-m32', '-static', '-DPYTHON3',
         '-DSCRIPT="scripts/qtmain.py"',
         os.path.join(builddir, 'shortcuts', 'linux-bin.c'),
         '-o', bingui]
    )

    check_if_failed(exit_status, output, error,
                    'Failed to compile Linux launcher')

    exit_status, output, error = run_command(
        ['strip', bingui]
    )

    check_if_failed(exit_status, output, error,
                    'Failed to strip symbols from Linux launcher')

    print('Compressing Linux launcher with UPX ...')
    if buildtype != 'ci':
        upx_compress([bingui])

    os.chmod(bingui, 0o755)


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

    with open(os.path.join(topdir, 'patcher.yaml.in')) as f_in:
        with open(os.path.join(targetdir, 'patcher.yaml'), 'w') as f_out:
            for line in f_in:
                f_out.write(line.replace('@VERSION@', version))

    if android:
        remove = [
            'patchinfo/README',
            'patchinfo/Sample.py',
            'scripts/gendiff.py',
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

    create_python_windows(targetdir)
    create_pyqt_windows(targetdir)

    pyport = os.path.join(targetdir, 'pythonportable')
    pysitelib = os.path.join(pyport, 'Lib', 'site-packages')
    create_pyyaml(targetdir, pysitelib)

    create_binaries_windows(targetdir)
    create_shortcuts_windows(targetdir)
    create_shortcuts_linux(targetdir)

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

    manifest = os.path.join(androiddir, 'AndroidManifest.xml')
    with open(manifest + '.in') as f_in:
        with open(os.path.join(manifest), 'w') as f_out:
            for line in f_in:
                f_out.write(re.sub(r'@VERSION@', version, line))

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
    sdkenv['TERM'] = 'xterm'
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

    create_python_android(targetdir)

    pyport = os.path.join(targetdir, 'pythonportable')
    pysitelib = os.path.join(pyport, 'lib', 'python3.4', 'site-packages')
    create_pyyaml(targetdir, pysitelib)

    create_binaries_android(targetdir)

    create_release(targetdir, targetfile, android=True)

    # Build the app
    assetdir = os.path.join(androiddir, 'assets')
    if os.path.exists(assetdir):
        shutil.rmtree(assetdir)
    os.makedirs(assetdir)
    shutil.move(targetfile, assetdir)
    shutil.copyfile(os.path.join(topdir, 'ramdisks', 'busybox-static'),
                    os.path.join(assetdir, 'tar'))

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
