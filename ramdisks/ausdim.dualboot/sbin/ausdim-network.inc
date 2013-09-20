# CIFS support
if [ "`/sbin/busybox grep network_cifs=on $AUSDIM_CONFIG`" ]; then

	insmod /lib/modules/cifs.ko

	echo $(date) "CIFS support enabled" >> $AUSDIM_LOGFILE
fi


# NTFS support
if [ "`/sbin/busybox grep ntfs=on $AUSDIM_CONFIG`" ]; then

	insmod /lib/modules/ntfs.ko

	echo $(date) "NTFS support enabled" >> $AUSDIM_LOGFILE
fi
