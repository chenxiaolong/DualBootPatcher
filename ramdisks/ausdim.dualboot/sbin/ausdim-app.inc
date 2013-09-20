# Versions of apps bundled with this kernel
VERSION_TWEAKS="dk_v003"
VERSION_DIALOG="dk_v002"

# Depending on the config setting, ausdim kernel manager and ausdim dialog apps
# get installed/upgraded or uninstalled

if /sbin/busybox [ "`/sbin/busybox grep ausdim_app=off $AUSDIM_CONFIG`" ]; then

	# If app is installed, uninstall it now and delete all config scripts
	STATUS=`/system/bin/pm path dk.ausdim`
	if [ ! -z $STATUS ]; then
		/system/bin/pm uninstall dk.ausdim

		/sbin/busybox mount -o remount,rw -t ext4 $SYSTEM_DEVICE /system
		/sbin/busybox rm /system/etc/init.d/*_dk*
		/sbin/busybox rm /system/etc/init.d/*_???dk*
		/sbin/busybox mount -o remount,ro -t ext4 $SYSTEM_DEVICE /system

		echo $(date) Ausdim kernel manager app uninstalled and settings deleted >> $AUSDIM_LOGFILE
	fi

	# If dialog app is installed, uninstall it now
	STATUS=`/system/bin/pm path dk.ausdim.tweaks.dialog.helper`
	if [ ! -z $STATUS ]; then
		/system/bin/pm uninstall dk.ausdim.tweaks.dialog.helper
		echo $(date) Ausdim dialog app uninstalled >> $AUSDIM_LOGFILE
	fi

else

	# if no or old version of tweaks app is installed, install/upgrade now
	if ! [ "`/sbin/busybox grep $VERSION_TWEAKS /data/app/dk.ausdim-*.apk`" ] ; then
		/system/bin/pm install -r /res/app/AusdimKernelManager.apk
		echo $(date) Ausdim kernel manager app $VERSION_TWEAKS installed/updated >> $AUSDIM_LOGFILE
	fi

	# if no or old version of dialog app is installed, install/upgrade now
	if ! [ "`/sbin/busybox grep $VERSION_DIALOG /data/app/dk.ausdim.tweaks.dialog.helper-*.apk`" ] ; then
		/system/bin/pm install -r /res/app/AusdimKernelManagerDialogHelper.apk
		echo $(date) Ausdim dialog app $VERSION_DIALOG installed/updated >> $AUSDIM_LOGFILE
	fi

fi
