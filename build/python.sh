create_python_windows() {
    local URL="http://python.org/ftp/python/3.4.0/python-3.4.0.msi"
    local MD5="e3be8a63294e42e126493ca96cfe48bd"
    download_md5 "${URL}" "${MD5}" "${BUILDDIR}/windowsbinaries"

    rm -rf "${TARGETDIR}/pythonportable/"

    DISPLAY= wine msiexec \
        /a \
        $(winepath -w "${BUILDDIR}/windowsbinaries/python-3.4.0.msi") \
        /qb \
        TARGETDIR=$(winepath -w "${TARGETDIR}/pythonportable/")

    pushd "${TARGETDIR}/pythonportable/"

    # Don't add unneeded files to the zip
    find -name __pycache__ | xargs rm -rf

    rm -r DLLs
    rm -r Doc
    rm -r include
    rm -r libs
    rm -r tcl
    rm -r Tools
    rm NEWS.txt
    rm README.txt
    rm -r Lib/{concurrent,ctypes,curses}
    rm -r Lib/{dbm,distutils}
    rm -r Lib/{email,ensurepip}
    rm -r Lib/{html,http}
    rm -r Lib/idlelib
    rm -r Lib/json
    rm -r Lib/{lib2to3,logging}
    rm -r Lib/{msilib,multiprocessing}
    rm -r Lib/pydoc_data
    rm -r Lib/{site-packages,sqlite3}
    rm -r Lib/{test,tkinter,turtledemo}
    rm -r Lib/wsgiref
    rm -r Lib/{unittest,urllib}
    rm -r Lib/venv
    rm -r Lib/{xml,xmlrpc}

    rm Lib/{__phello__.foo.py,_compat_pickle.py,_dummy_thread.py,_markupbase.py,_osx_support.py,_pyio.py,_strptime.py,_threading_local.py}
    rm Lib/{aifc.py,antigravity.py,argparse.py,ast.py,asynchat.py}
    rm Lib/{base64.py,bdb.py,binhex.py,bisect.py,bz2.py}
    rm Lib/{calendar.py,cgi.py,cgitb.py,chunk.py,cmd.py,code.py,codeop.py,colorsys.py,compileall.py,cProfile.py,crypt.py,csv.py}
    rm Lib/{datetime.py,decimal.py,difflib.py,dis.py,doctest.py,dummy_threading.py}
    rm Lib/{filecmp.py,fileinput.py,formatter.py,fractions.py,ftplib.py}
    rm Lib/{getopt.py,getpass.py,gettext.py,glob.py}
    rm Lib/hmac.py
    rm Lib/{imaplib.py,imghdr.py,inspect.py,ipaddress.py}
    rm Lib/lzma.py
    rm Lib/{macpath.py,macurl2path.py,mailbox.py,mailcap.py,mimetypes.py,modulefinder.py}
    rm Lib/{netrc.py,nntplib.py,nturl2path.py,numbers.py}
    rm Lib/{opcode.py,optparse.py}
    rm Lib/{pdb.py,pickle.py,pickletools.py,pipes.py,pkgutil.py,plistlib.py,poplib.py,pprint.py,profile.py,pstats.py,pty.py,py_compile.py,pyclbr.py,pydoc.py}
    rm Lib/{queue.py,quopri.py}
    rm Lib/{rlcompleter.py,runpy.py}
    rm Lib/{sched.py,shelve.py,shlex.py,smtpd.py,smtplib.py,sndhdr.py,socket.py,socketserver.py,ssl.py,statistics.py,string.py,stringprep.py,sunau.py,symbol.py,symtable.py}
    rm Lib/{tabnanny.py,telnetlib.py,textwrap.py,this.py,timeit.py,tty.py,turtle.py}
    rm Lib/{uu.py,uuid.py}
    rm Lib/{wave.py,webbrowser.py}
    rm Lib/xdrlib.py

    popd
}

create_python_android() {
    local URL="http://fs1.d-h.st/download/00106/puN/python-install-3.4.0.tar.xz"
    local MD5SUM="1bca2f3af40e4f86665d6c4812318f51"
    download_md5 "${URL}" "${MD5}" "${BUILDDIR}/androidbinaries"

    rm -rf "${TARGETDIR}/pythonportable/"

    local TEMPDIR="$(mktemp -d --tmpdir="$(pwd)")"
    pushd "${TEMPDIR}"
    tar Jxvf "${BUILDDIR}/androidbinaries/python-install-3.4.0.tar.xz"
    mv python-install "${TARGETDIR}/pythonportable/"
    popd

    rm -rf "${TEMPDIR}"

    pushd "${TARGETDIR}/pythonportable/"

    # Don't add unneeded files to the zip

    local LIB=lib/python3.4

    rm -r ${LIB}/{concurrent,config-3.4m,ctypes,curses}
    rm -r ${LIB}/{dbm,distutils}
    rm -r ${LIB}/{email,ensurepip}
    rm -r ${LIB}/{html,http}
    rm -r ${LIB}/idlelib
    rm -r ${LIB}/json
    rm -r ${LIB}/{lib2to3,logging}
    rm -r ${LIB}/multiprocessing
    rm -r ${LIB}/{plat-linux,pydoc_data}
    rm -r ${LIB}/{site-packages,sqlite3}
    rm -r ${LIB}/{tkinter,turtledemo}
    rm -r ${LIB}/wsgiref
    rm -r ${LIB}/{unittest,urllib}
    rm -r ${LIB}/{xml,xmlrpc}

    rm ${LIB}/lib-dynload/_csv.*
    rm ${LIB}/lib-dynload/_ctypes.*
    rm ${LIB}/lib-dynload/_ctypes_test.*
    rm ${LIB}/lib-dynload/_datetime.*
    rm ${LIB}/lib-dynload/_elementtree.*
    rm ${LIB}/lib-dynload/_heapq.*
    rm ${LIB}/lib-dynload/_json.*
    rm ${LIB}/lib-dynload/_lsprof.*
    rm ${LIB}/lib-dynload/_multibytecodec.*
    rm ${LIB}/lib-dynload/_multiprocessing.*
    rm ${LIB}/lib-dynload/_opcode.*
    rm ${LIB}/lib-dynload/_pickle.*
    rm ${LIB}/lib-dynload/_socket.*
    rm ${LIB}/lib-dynload/_testbuffer.*
    rm ${LIB}/lib-dynload/_testcapi.*
    rm ${LIB}/lib-dynload/_testimportmultiple.*
    rm ${LIB}/lib-dynload/array.*
    rm ${LIB}/lib-dynload/audioop.*
    rm ${LIB}/lib-dynload/cmath.*
    rm ${LIB}/lib-dynload/fcntl.*
    rm ${LIB}/lib-dynload/mmap.*
    rm ${LIB}/lib-dynload/parser.*
    rm ${LIB}/lib-dynload/pyexpat.*
    rm ${LIB}/lib-dynload/resource.*
    rm ${LIB}/lib-dynload/syslog.*
    rm ${LIB}/lib-dynload/termios.*
    rm ${LIB}/lib-dynload/unicodedata.*
    rm ${LIB}/lib-dynload/xxlimited.*

    rm ${LIB}/{__phello__.foo.py,_bootlocale.py,_compat_pickle.py,_dummy_thread.py,_markupbase.py,_osx_support.py,_pyio.py,_strptime.py,_threading_local.py}
    rm ${LIB}/{aifc.py,antigravity.py,argparse.py,ast.py,asynchat.py}
    rm ${LIB}/{base64.py,bdb.py,binhex.py,bisect.py,bz2.py}
    rm ${LIB}/{calendar.py,cgi.py,cgitb.py,chunk.py,cmd.py,code.py,codeop.py,colorsys.py,compileall.py,cProfile.py,crypt.py,csv.py}
    rm ${LIB}/{datetime.py,decimal.py,difflib.py,dis.py,doctest.py,dummy_threading.py}
    rm ${LIB}/{filecmp.py,fileinput.py,formatter.py,fractions.py,ftplib.py}
    rm ${LIB}/{getopt.py,getpass.py,gettext.py,glob.py}
    rm ${LIB}/hmac.py
    rm ${LIB}/{imaplib.py,imghdr.py,inspect.py,ipaddress.py}
    rm ${LIB}/lzma.py
    rm ${LIB}/{macpath.py,macurl2path.py,mailbox.py,mailcap.py,mimetypes.py,modulefinder.py}
    rm ${LIB}/{netrc.py,nntplib.py,nturl2path.py,numbers.py}
    rm ${LIB}/{opcode.py,optparse.py}
    rm ${LIB}/{pdb.py,pickle.py,pickletools.py,pipes.py,pkgutil.py,plistlib.py,poplib.py,pprint.py,profile.py,pstats.py,pty.py,py_compile.py,pyclbr.py,pydoc.py}
    rm ${LIB}/{queue.py,quopri.py}
    rm ${LIB}/{rlcompleter.py,runpy.py}
    rm ${LIB}/{sched.py,shelve.py,shlex.py,smtpd.py,smtplib.py,sndhdr.py,socket.py,socketserver.py,ssl.py,statistics.py,string.py,stringprep.py,sunau.py,symbol.py,symtable.py}
    rm ${LIB}/{tabnanny.py,telnetlib.py,textwrap.py,this.py,timeit.py,tty.py,turtle.py}
    rm ${LIB}/{uu.py,uuid.py}
    rm ${LIB}/{wave.py,webbrowser.py}
    rm ${LIB}/xdrlib.py

    if [ "x${BUILDTYPE}" != "xci" ]; then
        # Compress executables and libraries
        upx -v --lzma *.exe *.dll || :
    fi

    popd
}
