#!/system/bin/sh
# Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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
# start two rild when dsds property enabled
#
dsds=`getprop persist.multisim.config`
if [ "$dsds" = "dsds" ]; then
    setprop ro.multi.rild true
    stop ril-daemon
    start ril-daemon
    start ril-daemon1
fi

    #To allow interfaces to get v6 address when tethering is enabled
    echo 2 > /proc/sys/net/ipv6/conf/rmnet0/accept_ra
    echo 2 > /proc/sys/net/ipv6/conf/rmnet1/accept_ra
    echo 2 > /proc/sys/net/ipv6/conf/rmnet2/accept_ra
    echo 2 > /proc/sys/net/ipv6/conf/rmnet3/accept_ra
    echo 2 > /proc/sys/net/ipv6/conf/rmnet4/accept_ra
    echo 2 > /proc/sys/net/ipv6/conf/rmnet5/accept_ra
    echo 2 > /proc/sys/net/ipv6/conf/rmnet6/accept_ra
    echo 2 > /proc/sys/net/ipv6/conf/rmnet7/accept_ra
    echo 2 > /proc/sys/net/ipv6/conf/rmnet_sdio0/accept_ra
    echo 2 > /proc/sys/net/ipv6/conf/rmnet_sdio1/accept_ra
    echo 2 > /proc/sys/net/ipv6/conf/rmnet_sdio2/accept_ra
    echo 2 > /proc/sys/net/ipv6/conf/rmnet_sdio3/accept_ra
    echo 2 > /proc/sys/net/ipv6/conf/rmnet_sdio4/accept_ra
    echo 2 > /proc/sys/net/ipv6/conf/rmnet_sdio5/accept_ra
    echo 2 > /proc/sys/net/ipv6/conf/rmnet_sdio6/accept_ra
    echo 2 > /proc/sys/net/ipv6/conf/rmnet_sdio7/accept_ra
    echo 2 > /proc/sys/net/ipv6/conf/rmnet_usb0/accept_ra
    echo 2 > /proc/sys/net/ipv6/conf/rmnet_usb1/accept_ra
    echo 2 > /proc/sys/net/ipv6/conf/rmnet_usb2/accept_ra
    echo 2 > /proc/sys/net/ipv6/conf/rmnet_usb3/accept_ra
