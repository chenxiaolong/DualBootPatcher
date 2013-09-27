#!/system/bin/sh
# Copyright (c) 2012, Code Aurora Forum. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of Code Aurora Forum, Inc. nor the names of its
#       contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
# ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#
# Allow unique persistent serial numbers for devices connected via usb
# User needs to set unique usb serial number to persist.usb.serialno and
# if persistent serial number is not set then Update USB serial number if
# passed from command line
#

#
# Allow persistent usb charging disabling
# User needs to set usb charging disabled in persist.usb.chgdisabled
#
echo 1 > /sys/devices/system/cpu/cpu1/online
echo 1 > /sys/devices/system/cpu/cpu2/online
echo 1 > /sys/devices/system/cpu/cpu3/online
echo 1 > /sys/module/rpm_resources/enable_low_power/L2_cache
echo 1 > /sys/module/rpm_resources/enable_low_power/pxo
echo 1 > /sys/module/rpm_resources/enable_low_power/vdd_dig
echo 1 > /sys/module/rpm_resources/enable_low_power/vdd_mem
echo 1 > /sys/module/pm_8x60/modes/cpu0/power_collapse/suspend_enabled
echo 1 > /sys/module/pm_8x60/modes/cpu1/power_collapse/suspend_enabled
echo 1 > /sys/module/pm_8x60/modes/cpu2/power_collapse/suspend_enabled
echo 1 > /sys/module/pm_8x60/modes/cpu3/power_collapse/suspend_enabled
echo 0 > /sys/module/pm_8x60/modes/cpu0/standalone_power_collapse/suspend_enabled
echo 0 > /sys/module/pm_8x60/modes/cpu1/standalone_power_collapse/suspend_enabled
echo 0 > /sys/module/pm_8x60/modes/cpu2/standalone_power_collapse/suspend_enabled
echo 0 > /sys/module/pm_8x60/modes/cpu3/standalone_power_collapse/suspend_enabled
echo 0 > /sys/module/pm_8x60/modes/cpu0/standalone_power_collapse/idle_enabled
echo 0 > /sys/module/pm_8x60/modes/cpu1/standalone_power_collapse/idle_enabled
echo 0 > /sys/module/pm_8x60/modes/cpu2/standalone_power_collapse/idle_enabled
echo 0 > /sys/module/pm_8x60/modes/cpu3/standalone_power_collapse/idle_enabled
echo 0 > /sys/module/pm_8x60/modes/cpu0/power_collapse/idle_enabled
echo 0 > /sys/module/pm_8x60/modes/cpu1/power_collapse/idle_enabled
echo 0 > /sys/module/pm_8x60/modes/cpu2/power_collapse/idle_enabled
echo 0 > /sys/module/pm_8x60/modes/cpu3/power_collapse/idle_enabled
echo 1 > /sys/module/pm_8660/modes/cpu0/power_collapse/suspend_enabled
echo 1 > /sys/module/pm_8660/modes/cpu1/power_collapse/suspend_enabled
echo 1 > /sys/module/pm_8660/modes/cpu2/power_collapse/suspend_enabled
echo 1 > /sys/module/pm_8660/modes/cpu3/power_collapse/suspend_enabled
echo 1 > /sys/module/pm_8660/modes/cpu0/standalone_power_collapse/suspend_enabled
echo 1 > /sys/module/pm_8660/modes/cpu1/standalone_power_collapse/suspend_enabled
echo 1 > /sys/module/pm_8660/modes/cpu2/standalone_power_collapse/suspend_enabled
echo 1 > /sys/module/pm_8660/modes/cpu3/standalone_power_collapse/suspend_enabled
echo 0 > /sys/module/pm_8660/modes/cpu0/standalone_power_collapse/idle_enabled
echo 0 > /sys/module/pm_8660/modes/cpu1/standalone_power_collapse/idle_enabled
echo 0 > /sys/module/pm_8660/modes/cpu2/standalone_power_collapse/idle_enabled
echo 0 > /sys/module/pm_8660/modes/cpu3/standalone_power_collapse/idle_enabled
echo 0 > /sys/module/pm_8660/modes/cpu0/power_collapse/idle_enabled
echo 0 > /sys/module/pm_8660/modes/cpu1/power_collapse/idle_enabled
echo 0 > /sys/module/pm_8660/modes/cpu2/power_collapse/idle_enabled
echo 0 > /sys/module/pm_8660/modes/cpu3/power_collapse/idle_enabled
echo "powersave" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo "powersave" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
echo "powersave" > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
echo "powersave" > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
echo 0 > /sys/devices/system/cpu/cpu1/online
echo 0 > /sys/devices/system/cpu/cpu2/online
echo 0 > /sys/devices/system/cpu/cpu3/online
