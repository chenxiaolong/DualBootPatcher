# Current version of config file
CONFVERSION="Config-version-001"

# check if current config file has correct version number and delete if not
if ! /sbin/busybox [ "`/sbin/busybox grep $CONFVERSION $AUSDIM_CONFIG`" ]; then
	/sbin/busybox rm $AUSDIM_CONFIG
fi


# check if config file exists and create a new one if not
if [ ! -f $AUSDIM_CONFIG ] ; then

	echo "Ausdim-Kernel configuration" > $AUSDIM_CONFIG
	echo $CONFVERSION >> $AUSDIM_CONFIG
	echo "============================" >> $AUSDIM_CONFIG
	echo " " >> $AUSDIM_CONFIG

	# change file permissions to make file readable via /sdcard link as well
	chmod 666 $AUSDIM_CONFIG

fi


# Now add all the required options that do not yet exists


# Governor and scheduler

if ! /sbin/busybox [ "`/sbin/busybox grep governor $AUSDIM_CONFIG`" ]; then
	echo "governor=ondemand (ondemand/asswax/badass/adaptive/wheatley/dancedance/smartassH3/abyssplugv2/userspace/powersave/interactive/performance/intellidemand/conservative/ktoonservativeq)" >> $AUSDIM_CONFIG
	echo " " >> $AUSDIM_CONFIG
fi

if ! /sbin/busybox [ "`/sbin/busybox grep scheduler_int $AUSDIM_CONFIG`" ]; then
	echo "scheduler_int=row (default/cfq/row/bfq/sio/noop/deadline)" >> $AUSDIM_CONFIG
	echo " " >> $AUSDIM_CONFIG
fi

if ! /sbin/busybox [ "`/sbin/busybox grep scheduler_ext $AUSDIM_CONFIG`" ]; then
	echo "scheduler_ext=row (defaut/cfq/row/bfq/sio/noop/deadline)" >> $AUSDIM_CONFIG
	echo " " >> $AUSDIM_CONFIG
fi


# CPU min frequency

if ! /sbin/busybox [ "`/sbin/busybox grep cpu_minfreq $AUSDIM_CONFIG`" ]; then
        echo "cpu_minfreq=default (default/81...2160)" >> $AUSDIM_CONFIG
        echo " " >> $AUSDIM_CONFIG
fi


# CPU max frequency and undervolting

if ! /sbin/busybox [ "`/sbin/busybox grep cpu_maxfreq $AUSDIM_CONFIG`" ]; then
        echo "cpu_maxfreq=default (default/81...2160)" >> $AUSDIM_CONFIG
        echo " " >> $AUSDIM_CONFIG
fi

if ! /sbin/busybox [ "`/sbin/busybox grep cpu_undervolting $AUSDIM_CONFIG`" ]; then
	echo "cpu_undervolting=off (default/25/50/75/100)" >> $AUSDIM_CONFIG
	echo " " >> $AUSDIM_CONFIG
fi


# GPU Frequency and under/overvolting

if ! /sbin/busybox [ "`/sbin/busybox grep gpu_frequency $AUSDIM_CONFIG`" ]; then
	echo "gpu_frequency=default (default/128/200/320/504/545/600/627)" >> $AUSDIM_CONFIG
	echo " " >> $AUSDIM_CONFIG
fi

# LED settings

if ! /sbin/busybox [ "`/sbin/busybox grep led_fading $AUSDIM_CONFIG`" ]; then
	echo "led_fading=off (blink/fade/blink-bright/fade-bright/blink-dark/fade-dark)" >> $AUSDIM_CONFIG
	echo " " >> $AUSDIM_CONFIG
fi

if ! /sbin/busybox [ "`/sbin/busybox grep led_speed $AUSDIM_CONFIG`" ]; then
	echo "led_speed=default (default/2x/3x/4x/5x)" >> $AUSDIM_CONFIG
	echo " " >> $AUSDIM_CONFIG
fi

# Tweaks (system, ext4 and sdcard)

if ! /sbin/busybox [ "`/sbin/busybox grep systemtweaks $AUSDIM_CONFIG`" ]; then
	echo "systemtweaks=off (off/on/smod)" >> $AUSDIM_CONFIG
	echo " " >> $AUSDIM_CONFIG
fi

if ! /sbin/busybox [ "`/sbin/busybox grep ext4tweaks $AUSDIM_CONFIG`" ]; then
	echo "ext4tweaks=on (on/off)" >> $AUSDIM_CONFIG
	echo " " >> $AUSDIM_CONFIG
fi

if ! /sbin/busybox [ "`/sbin/busybox grep sdcardtweaks_int $AUSDIM_CONFIG`" ]; then
	echo "sdcardtweaks_int=on (on/off/128...4096)" >> $AUSDIM_CONFIG
	echo " " >> $AUSDIM_CONFIG
fi

if ! /sbin/busybox [ "`/sbin/busybox grep sdcardtweaks_ext $AUSDIM_CONFIG`" ]; then
	echo "sdcardtweaks_ext=on (on/off/128...4096)" >> $AUSDIM_CONFIG
	echo " " >> $AUSDIM_CONFIG
fi


# Support for Network CIFS and NTFS

if ! /sbin/busybox [ "`/sbin/busybox grep network_cifs $AUSDIM_CONFIG`" ]; then
        echo "network_cifs=off (off/on)" >> $AUSDIM_CONFIG
        echo " " >> $AUSDIM_CONFIG
fi

if ! /sbin/busybox [ "`/sbin/busybox grep ntfs $AUSDIM_CONFIG`" ]; then
        echo "ntfs=off (off/on)" >> $AUSDIM_CONFIG
        echo " " >> $AUSDIM_CONFIG
fi


# exFat

if ! /sbin/busybox [ "`/sbin/busybox grep exfat $AUSDIM_CONFIG`" ]; then
	echo "exfat=on (on/off)" >> $AUSDIM_CONFIG
	echo " " >> $AUSDIM_CONFIG
fi


# Auto root and configuration app (both options are only available via config file)

if ! /sbin/busybox [ "`/sbin/busybox grep autoroot $AUSDIM_CONFIG`" ]; then
	echo "autoroot=off (off/on)" >> $AUSDIM_CONFIG
	echo " " >> $AUSDIM_CONFIG
fi

if ! /sbin/busybox [ "`/sbin/busybox grep ausdim_app $AUSDIM_CONFIG`" ]; then
	echo "ausdim_app=on (on/off)" >> $AUSDIM_CONFIG
	echo " " >> $AUSDIM_CONFIG
fi

# Finally remove old options that are not required anymore
# example: /sbin/busybox sed "/Config file version/d" $AUSDIM_CONFIG -i
