# Change sd card read ahead buffer


# Read ahead for internal sd memory

# Initialize buffer variable and read tweak setting
BUFFER="256"
TWEAK=$(/sbin/busybox sed -n -e 's/.*sdcardtweaks_int=\(.*\)(.*/\1/p' $AUSDIM_CONFIG)

if [ "${TWEAK:-0}" -ge 128 ] && [ "${TWEAK:-0}" -le 4096 ] ; then
	BUFFER=$TWEAK
fi

# Only change read ahead buffer if tweak is not switched off
if  [ ! "`/sbin/busybox grep sdcardtweaks=off $AUSDIM_CONFIG`" ]; then
	echo $(( $BUFFER * 1)) > /sys/block/mmcblk0/bdi/read_ahead_kb
	echo $(date) "SDcard buffer tweaks ($BUFFER kb) applied for internal sd memory" >> $AUSDIM_LOGFILE
fi


# Read ahead for external sd memory

# Initialize buffer variable and read tweak setting
BUFFER="256"
TWEAK=$(/sbin/busybox sed -n -e 's/.*sdcardtweaks_ext=\(.*\)(.*/\1/p' $AUSDIM_CONFIG)

if [ "${TWEAK:-0}" -ge 128 ] && [ "${TWEAK:-0}" -le 4096 ] ; then
	BUFFER=$TWEAK
fi

# Only change read ahead buffer if tweak is not switched off
if  [ ! "`/sbin/busybox grep sdcardtweaks=off $AUSDIM_CONFIG`" ]; then
	echo $(( $BUFFER * 1)) > /sys/block/mmcblk1/bdi/read_ahead_kb
	echo $(date) "SDcard buffer tweaks ($BUFFER kb) applied for external sd memory" >> $AUSDIM_LOGFILE
fi
