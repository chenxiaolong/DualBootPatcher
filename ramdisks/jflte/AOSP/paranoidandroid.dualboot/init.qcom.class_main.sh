#!/system/bin/sh
# Copyright (c) 2012, The Linux Foundation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of The Linux Foundation nor
#       the names of its contributors may be used to endorse or promote
#       products derived from this software without specific prior written
#       permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#
# start ril-daemon only for targets on which radio is present
#
baseband=`getprop ro.baseband`
multirild=`getprop ro.multi.rild`
dsds=`getprop persist.dsds.enabled`
netmgr=`getprop ro.use_data_netmgrd`
sgltecsfb=`getprop persist.radio.sglte_csfb`

case "$baseband" in
    "apq")
    setprop ro.radio.noril yes
    stop ril-daemon
esac

case "$baseband" in
    "msm" | "csfb" | "svlte2a" | "mdm" | "sglte" | "sglte2" | "dsda2" | "unknown")
    start qmuxd
    case "$baseband" in
        "svlte2a" | "csfb")
        start qmiproxy
        ;;
        "sglte" | "sglte2")
        if [ "x$sgltecsfb" != "xtrue" ]; then
          start qmiproxy
        else
          setprop persist.radio.voice.modem.index 0
        fi
        ;;
        "dsda2")
          setprop ro.multi.rild true
          setprop persist.multisim.config dsda
          stop ril-daemon
          start ril-daemon
          start ril-daemon1
    esac
    case "$multirild" in
        "true")
         case "$dsds" in
             "true")
             start ril-daemon1
         esac
    esac
    case "$netmgr" in
        "true")
        start netmgrd
    esac
esac
