EFS_BACKUP_INT="$AUSDIM_DATA_PATH/efs.tar.gz"
EFS_BACKUP_EXT="/storage/extSdCard/efs.tar.gz"

if [ ! -f $EFS_BACKUP_INT ]; then

	/sbin/busybox tar cvz -f $EFS_BACKUP_INT /efs
	/sbin/busybox chmod 666 $EFS_BACKUP_INT

	/sbin/busybox cp $EFS_BACKUP_INT $EFS_BACKUP_EXT
	
	echo $(date) EFS Backup: Not found, now creating one >> $AUSDIM_LOGFILE

fi
