#!/sbin/busybox sh
#
# credits to various people

# define config file pathes, file variables and block devices
AUSDIM_PATH="/data/media/0/ausdim-kernel"
AUSDIM_CONFIG="$AUSDIM_PATH/ausdim-kernel.conf"

AUSDIM_DATA_PATH="/data/media/0/ausdim-kernel-data"
AUSDIM_LOGFILE="$AUSDIM_DATA_PATH/ausdim-kernel.log"

SYSTEM_DEVICE="/dev/block/mmcblk0p16"
CACHE_DEVICE="/dev/block/mmcblk0p18"
DATA_DEVICE="/dev/block/mmcblk0p29"

# check if Ausdim kernel path exists and if so, execute config file
if [ -d "$AUSDIM_PATH" ] ; then

	# check and create or update the configuration file
	. /sbin/ausdim-configfile.inc

else
	AUSDIM_CONFIG=""
fi

# If not yet exists, create a AUSDIM-kernel-data folder on sdcard 
# which is used for many purposes (set permissions and owners correctly)
if [ ! -d "$AUSDIM_DATA_PATH" ] ; then
	/sbin/busybox mkdir $AUSDIM_DATA_PATH
	/sbin/busybox chmod 775 $AUSDIM_DATA_PATH
	/sbin/busybox chown 1023:1023 $AUSDIM_DATA_PATH
fi

# maintain log file history
rm $AUSDIM_LOGFILE.3
mv $AUSDIM_LOGFILE.2 $AUSDIM_LOGFILE.3
mv $AUSDIM_LOGFILE.1 $AUSDIM_LOGFILE.2
mv $AUSDIM_LOGFILE $AUSDIM_LOGFILE.1

# Initialize the log file (chmod to make it readable also via /sdcard link)
echo $(date) Ausdim-Kernel initialisation started > $AUSDIM_LOGFILE
/sbin/busybox chmod 666 $AUSDIM_LOGFILE
/sbin/busybox cat /proc/version >> $AUSDIM_LOGFILE
echo "=========================" >> $AUSDIM_LOGFILE

# Include version information about firmware in log file
/sbin/busybox grep ro.build.version /system/build.prop >> $AUSDIM_LOGFILE
echo "=========================" >> $AUSDIM_LOGFILE

# Fix potential issues and clean up
#. /sbin/ausdim-fix-and-cleanup.inc

# Custom boot animation support
. /sbin/ausdim-bootanimation.inc

# Now wait for the rom to finish booting up
# (by checking for any android process)
while ! /sbin/busybox pgrep android.process.acore ; do
  /sbin/busybox sleep 1
done
echo $(date) Rom boot trigger detected, continuing after 8 more seconds... >> $AUSDIM_LOGFILE
/sbin/busybox sleep 8

# Governor
. /sbin/ausdim-governor.inc

# IO Scheduler
. /sbin/ausdim-scheduler.inc

# ScreenOFF CPU max frequency
. /sbin/ausdim-screenoffmaxfrequency.inc

# CPU min frequency
. /sbin/ausdim-cpuminfrequency.inc

# CPU max frequency
. /sbin/ausdim-cpumaxfrequency.inc

# CPU undervolting support
. /sbin/ausdim-cpuundervolt.inc

# GPU frequency
. /sbin/ausdim-gpufrequency.inc

# LED settings
. /sbin/ausdim-led.inc

# Sdcard buffer tweaks
. /sbin/ausdim-sdcard.inc

# Support for additional network protocols (CIFS, NFS)
. /sbin/ausdim-network.inc

# exFat support
. /sbin/ausdim-exfat.inc

# Auto root support
. /sbin/ausdim-autoroot.inc

# Configuration app support
. /sbin/ausdim-app.inc

# EFS backup
. /sbin/ausdim-efsbackup.inc

# init.d support
. /sbin/ausdim-initd.inc

# Finished
echo $(date) Ausdim-Kernel initialisation completed >> $AUSDIM_LOGFILE
