create_python_windows() {
    local URL="http://python.org/ftp/python/3.3.3/python-3.3.3.msi"
    local MD5="ab6a031aeca66507e4c8697ff93a0007"
    download_md5 "${URL}" "${MD5}" "${BUILDDIR}/windowsbinaries"

    rm -rf "${TARGETDIR}/pythonportable/"

    DISPLAY= wine msiexec \
        /a \
        $(winepath -w "${BUILDDIR}/windowsbinaries/python-3.3.3.msi") \
        /qb \
        TARGETDIR=$(winepath -w "${TARGETDIR}/pythonportable/")

    pushd "${TARGETDIR}/pythonportable/"

    # Don't add unneeded files to the zip
    find -name __pycache__ -delete

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
    rm -r Lib/email
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
    rm Lib/{aifc.py,antigravity.py,argparse.py,ast.py}
    rm Lib/{base64.py,bdb.py,binhex.py,bz2.py}
    rm Lib/{calendar.py,cgi.py,cgitb.py,chunk.py,cmd.py,code.py,codeop.py,colorsys.py,compileall.py,contextlib.py,cProfile.py,crypt.py,csv.py}
    rm Lib/{datetime.py,decimal.py,difflib.py,dis.py,doctest.py,dummy_threading.py}
    rm Lib/{filecmp.py,fileinput.py,formatter.py,fractions.py,ftplib.py}
    rm Lib/{getopt.py,getpass.py,gettext.py,glob.py}
    rm Lib/hmac.py
    rm Lib/{imaplib.py,imghdr.py,inspect.py,ipaddress.py}
    rm Lib/lzma.py
    rm Lib/{macpath.py,macurl2path.py,mailbox.py,mailcap.py,mimetypes.py,modulefinder.py}
    rm Lib/{netrc.py,nntplib.py,nturl2path.py,numbers.py}
    rm Lib/{opcode.py,optparse.py,os2emxpath.py}
    rm Lib/{pdb.py,pickle.py,pickletools.py,pipes.py,pkgutil.py,plistlib.py,poplib.py,pprint.py,profile.py,pstats.py,pty.py,py_compile.py,pyclbr.py,pydoc.py}
    rm Lib/{queue.py,quopri.py}
    rm Lib/{rlcompleter.py,runpy.py}
    rm Lib/{sched.py,shelve.py,shlex.py,smtpd.py,smtplib.py,sndhdr.py,socket.py,socketserver.py,ssl.py,string.py,stringprep.py,sunau.py,symbol.py,symtable.py}
    rm Lib/{tabnanny.py,telnetlib.py,textwrap.py,this.py,timeit.py,tty.py,turtle.py}
    rm Lib/{uu.py,uuid.py}
    rm Lib/{wave.py,webbrowser.py}
    rm Lib/xdrlib.py

    popd
}

create_python_android() {
    local URL="http://fs1.d-h.st/download/00095/lwy/python-install-eeba91c.tar.xz"
    local MD5SUM="869aacce52cac8febe0533905203b316"
    download_md5 "${URL}" "${MD5}" "${BUILDDIR}/androidbinaries" python-install.tar.xz

    rm -rf "${TARGETDIR}/pythonportable/"

    local TEMPDIR="$(mktemp -d --tmpdir="$(pwd)")"
    pushd "${TEMPDIR}"
    tar Jxvf "${BUILDDIR}/androidbinaries/python-install.tar.xz"
    mv python-install "${TARGETDIR}/pythonportable/"
    popd

    rm -rf "${TEMPDIR}"

    pushd "${TARGETDIR}/pythonportable/"

    # Don't add unneeded files to the zip
    find bin -type f ! -name python -delete

    local LIB=lib/python2.7

    rm -r ${LIB}/bsddb
    rm -r ${LIB}/{compiler,config,ctypes,curses}
    rm -r ${LIB}/distutils
    rm -r ${LIB}/email
    rm -r ${LIB}/hotshot
    rm -r ${LIB}/{idlelib,importlib}
    rm -r ${LIB}/json
    rm -r ${LIB}/{lib-tk,lib2to3,logging}
    rm -r ${LIB}/multiprocessing
    rm -r ${LIB}/{plat-linux3,pydoc_data}
    rm -r ${LIB}/{site-packages,sqlite3}
    rm -r ${LIB}/wsgiref
    rm -r ${LIB}/unittest
    rm -r ${LIB}/xml

    rm ${LIB}/lib-dynload/{_csv.so,_heapq.so,_hotshot.so,_json.so,_lsprof.so,_testcapi.so,audioop.so,grp.so,mmap.so,resource.so,syslog.so,termios.so}

    rm ${LIB}/{__phello__.foo.py,_LWPCookieJar.py,_MozillaCookieJar.py,_pyio.py,_strptime.py,_threading_local.py}
    rm ${LIB}/{aifc.py,antigravity.py,anydbm.py,argparse.py,ast.py,atexit.py,audiodev.py}
    rm ${LIB}/{base64.py,BaseHTTPServer.py,Bastion.py,bdb.py,binhex.py}
    rm ${LIB}/{calendar.py,cgi.py,CGIHTTPServer.py,cgitb.py,chunk.py,cmd.py,code.py,codeop.py,commands.py,colorsys.py,compileall.py,contextlib.py,Cookie.py,cookielib.py,cProfile.py,csv.py}
    rm ${LIB}/{dbhash.py,decimal.py,difflib.py,dircache.py,dis.py,doctest.py,DocXMLRPCServer.py,dumbdbm.py,dummy_thread.py,dummy_threading.py}
    rm ${LIB}/{filecmp.py,fileinput.py,formatter.py,fpformat.py,fractions.py,ftplib.py}
    rm ${LIB}/{getopt.py,getpass.py,gettext.py,glob.py}
    rm ${LIB}/{hmac.py,htmlentitydefs.py,htmllib.py,HTMLParser.py,httplib.py}
    rm ${LIB}/{ihooks.py,imaplib.py,imghdr.py,imputil.py,inspect.py}
    rm ${LIB}/{macpath.py,macurl2path.py,mailbox.py,mailcap.py,markupbase.py,md5.py,mhlib.py,mimetools.py,mimetypes.py,MimeWriter.py,mimify.py,modulefinder.py,multifile.py,mutex.py}
    rm ${LIB}/{netrc.py,new.py,nntplib.py,nturl2path.py,numbers.py}
    rm ${LIB}/{opcode.py,optparse.py,os2emxpath.py}
    rm ${LIB}/{pdb.doc,pdb.py,pickletools.py,pipes.py,pkgutil.py,plistlib.py,popen2.py,poplib.py,posixfile.py,pprint.py,profile.py,pstats.py,pty.py,py_compile.py,pyclbr.py,pydoc.py}
    rm ${LIB}/{Queue.py,quopri.py}
    rm ${LIB}/{rexec.py,rfc822.py,rlcompleter.py,robotparser.py,runpy.py}
    rm ${LIB}/{sched.py,sets.py,sgmllib.py,sha.py,shelve.py,shlex.py,SimpleHTTPServer.py,SimpleXMLRPCServer.py,smtpd.py,smtplib.py,sndhdr.py,socket.py,SocketServer.py,sre.py,ssl.py,statvfs.py,StringIO.py,stringold.py,stringprep.py,sunau.py,sunaudio.py,symbol.py,symtable.py}
    rm ${LIB}/{tabnanny.py,telnetlib.py,textwrap.py,this.py,timeit.py,toaiff.py,tty.py}
    rm ${LIB}/{urllib.py,urllib2.py,urlparse.py,user.py,UserList.py,UserString.py,uu.py,uuid.py}
    rm ${LIB}/{wave.py,webbrowser.py,whichdb.py,wsgiref.egg-info}
    rm ${LIB}/{xdrlib.py,xmllib.py,xmlrpclib.py}

    # Compress executables and libraries
    upx -v --lzma *.exe *.dll || :

    popd
}
