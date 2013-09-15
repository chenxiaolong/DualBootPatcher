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

export PATH=/system/bin

# Set platform variables
soc_hwplatform=`cat /sys/devices/system/soc/soc0/hw_platform` 2> /dev/null
soc_hwid=`cat /sys/devices/system/soc/soc0/id` 2> /dev/null
soc_hwver=`cat /sys/devices/system/soc/soc0/platform_version` 2> /dev/null

log -t BOOT -p i "MSM target '$1', SoC '$soc_hwplatform', HwID '$soc_hwid', SoC ver '$soc_hwver'"

case "$1" in
    "msm7630_surf" | "msm7630_1x" | "msm7630_fusion")
        case "$soc_hwplatform" in
            "FFA" | "SVLTE_FFA")
                # linking to surf_keypad_qwerty.kcm.bin instead of surf_keypad_numeric.kcm.bin so that
                # the UI keyboard works fine.
                ln -s  /system/usr/keychars/surf_keypad_qwerty.kcm.bin /system/usr/keychars/surf_keypad.kcm.bin
                ;;
            "Fluid")
                setprop ro.sf.lcd_density 240
                setprop qcom.bt.dev_power_class 2
                ;;
            *)
                ln -s  /system/usr/keychars/surf_keypad_qwerty.kcm.bin /system/usr/keychars/surf_keypad.kcm.bin
                ;;
        esac
        ;;

    "msm8660")
        case "$soc_hwplatform" in
            "Fluid")
                setprop ro.sf.lcd_density 240
                ;;
            "Dragon")
                setprop ro.sound.alsa "WM8903"
                ;;
        esac
        ;;

    "msm8960")
        # lcd density is write-once. Hence the separate switch case
        case "$soc_hwplatform" in
            "Liquid")
                if [ "$soc_hwver" == "196608" ]; then # version 0x30000 is 3D sku
                    setprop ro.sf.hwrotation 90
                fi

                setprop ro.sf.lcd_density 160
                ;;
            "MTP")
                setprop ro.sf.lcd_density 240
                ;;
            *)
                case "$soc_hwid" in
                    "142") #8x30 QRD
                        setprop ro.sf.lcd_density 320
                        ;;
                    "109")
                        setprop ro.sf.lcd_density 160
                        ;;
                    *)
                        setprop ro.sf.lcd_density 240
                        ;;
                esac
            ;;
        esac

        #Set up MSM-specific configuration
        #case "$soc_hwid" in
        #    87)
        #        #8960
        #        setprop debug.composition.type dyn
        #        ;;
        #    153 | 154 | 155 | 156 | 157 | 138 | 179 | 180 | 181)
        #        #8064 V2 PRIME | 8930AB | 8630AB | 8230AB | 8030AB | 8960AB | 8130/AA/AB
        #        setprop debug.composition.type c2d
        #        ;;
        #    *)
        #        ;;
        #esac

        case "$soc_hwid" in
            87 | 116 | 117 | 118 | 119 | 138 | 142 | 143 | 144 | 154 | 155 | 156 | 157 | 179 | 180 | 181)
                #Disable subsystem restart for 8x30 and 8960
                setprop persist.sys.ssr.restart_level 1
                ;;
            *)
                ;;
        esac
        ;;
esac

# Setup HDMI related nodes & permissions
# HDMI can be fb1 or fb2
# Loop through the sysfs nodes and determine
# the HDMI(dtv panel)
fb_cnt=0
for file in /sys/class/graphics/fb*
do
    value=`cat $file/msm_fb_type`
    case "$value" in
            "dtv panel")
        chown system.graphics $file/hpd
        chown system.graphics $file/vendor_name
        chown system.graphics $file/product_description
        chmod 0664 $file/hpd
        chmod 0664 $file/vendor_name
        chmod 0664 $file/product_description
        chmod 0664 $file/video_mode
        chmod 0664 $file/format_3d
        # create symbolic link
        ln -s "/dev/graphics/fb"$fb_cnt /dev/graphics/hdmi
        # Change owner and group for media server and surface flinger
        chown system.system $file/format_3d;;
    esac
    fb_cnt=$(( $fb_cnt + 1))
done

# Set date to a time after 2008
# This is a workaround for Zygote to preload time related classes properly
# date -s 20090102.130000

