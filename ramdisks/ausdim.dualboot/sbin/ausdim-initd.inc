# init.d support
# - Folder /system/etc/init.d will be create if not exists
# - All script files in the folders will get permission 777 to fix if user forgot to set

# remount /system in RW mode
/sbin/busybox mount -o remount,rw -t ext4 $SYSTEM_DEVICE /system

# check if init.d folder under /system/etc exists, and if not create it
if [ ! -d "/system/etc/init.d" ] ; then
  /sbin/busybox mkdir /system/etc/init.d
  echo $(date) Folder init.d did not exist, now created >> $AUSDIM_LOGFILE
fi

# set permissions of scripts
/sbin/busybox chmod 755 /system/etc/init.d/*
/sbin/busybox chmod 755 /data/init.d/*
/sbin/busybox chmod 755 /etc/init.d/*

# Delete all "old" (and newer) Ausdim-Tweaks scripts from previous kernel versions
/sbin/busybox rm /system/etc/init.d/*_dk*
/sbin/busybox rm /system/etc/init.d/*_002dk*
/sbin/busybox rm /system/etc/init.d/*_003dk*

# remount /system back to RO mode
/sbin/busybox mount -o remount,ro -t ext4 $SYSTEM_DEVICE /system

# /system/etc/init.d
if cd /system/etc/init.d >/dev/null 2>&1 ; then
    for file in * ; do
        if ! cat "$file" >/dev/null 2>&1 ; then continue ; fi
        /system/bin/sh "$file"
        echo $(date) init.d file $file executed >> $AUSDIM_LOGFILE
    done
fi

# /data/init.d
if cd /data/init.d >/dev/null 2>&1 ; then
    for file in * ; do
        if ! cat "$file" >/dev/null 2>&1 ; then continue ; fi
        /system/bin/sh "$file"
        echo $(date) init.d file $file executed >> $AUSDIM_LOGFILE
    done
fi

# /etc/init.d
if cd /etc/init.d >/dev/null 2>&1 ; then
    for file in 11* ; do
        if ! ls "$file" >/dev/null 2>&1 ; then continue ; fi
        /system/bin/sh "$file"
        echo $(date) init.d file $file executed >> $AUSDIM_LOGFILE
    done
fi
