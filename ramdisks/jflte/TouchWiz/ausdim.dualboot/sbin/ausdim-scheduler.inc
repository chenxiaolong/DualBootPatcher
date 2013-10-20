# change scheduler to deadline, noop, cfq or sio (if not set to default)

# Scheduler for internal sd memory

SCHEDULER=""

if /sbin/busybox [ "`/sbin/busybox grep scheduler_int=deadline $AUSDIM_CONFIG`" ]; then
  SCHEDULER="deadline"
fi

if /sbin/busybox [ "`/sbin/busybox grep scheduler_int=noop $AUSDIM_CONFIG`" ]; then
  SCHEDULER="noop"
fi

if /sbin/busybox [ "`/sbin/busybox grep scheduler_int=cfq $AUSDIM_CONFIG`" ]; then
  SCHEDULER="cfq"
fi

if /sbin/busybox [ "`/sbin/busybox grep scheduler_int=sio $AUSDIM_CONFIG`" ]; then
  SCHEDULER="sio"
fi

if /sbin/busybox [ "`/sbin/busybox grep scheduler_int=row $AUSDIM_CONFIG`" ]; then
  SCHEDULER="row"
fi

if /sbin/busybox [ "`/sbin/busybox grep scheduler_int=bfq $AUSDIM_CONFIG`" ]; then
  SCHEDULER="bfq"
fi

if /sbin/busybox [ "`/sbin/busybox grep scheduler_int=fiops $AUSDIM_CONFIG`" ]; then
  SCHEDULER="fiops"
fi

if /sbin/busybox [ "`/sbin/busybox grep scheduler_int=vr $AUSDIM_CONFIG`" ]; then
  SCHEDULER="vr"
fi

if /sbin/busybox [ "`/sbin/busybox grep scheduler_int=zen $AUSDIM_CONFIG`" ]; then
  SCHEDULER="zen"
fi

if /sbin/busybox [ "`/sbin/busybox grep scheduler_int=fifo $AUSDIM_CONFIG`" ]; then
  SCHEDULER="fifo"
fi

if [ ! -z $SCHEDULER ]; then
  echo "$SCHEDULER" > /sys/block/mmcblk0/queue/scheduler
  echo $(date) Internal sd memory scheduler set to $SCHEDULER >> $AUSDIM_LOGFILE
fi


# Scheduler for external sd memory

SCHEDULER=""

if /sbin/busybox [ "`/sbin/busybox grep scheduler_ext=deadline $AUSDIM_CONFIG`" ]; then
  SCHEDULER="deadline"
fi

if /sbin/busybox [ "`/sbin/busybox grep scheduler_ext=noop $AUSDIM_CONFIG`" ]; then
  SCHEDULER="noop"
fi

if /sbin/busybox [ "`/sbin/busybox grep scheduler_ext=cfq $AUSDIM_CONFIG`" ]; then
  SCHEDULER="cfq"
fi

if /sbin/busybox [ "`/sbin/busybox grep scheduler_ext=sio $AUSDIM_CONFIG`" ]; then
  SCHEDULER="sio"
fi

if /sbin/busybox [ "`/sbin/busybox grep scheduler_ext=row $AUSDIM_CONFIG`" ]; then
  SCHEDULER="row"
fi

if /sbin/busybox [ "`/sbin/busybox grep scheduler_ext=bfq $AUSDIM_CONFIG`" ]; then
  SCHEDULER="bfq"
fi

if /sbin/busybox [ "`/sbin/busybox grep scheduler_ext=fiops $AUSDIM_CONFIG`" ]; then
  SCHEDULER="fiops"
fi

if /sbin/busybox [ "`/sbin/busybox grep scheduler_ext=vr $AUSDIM_CONFIG`" ]; then
  SCHEDULER="vr"
fi

if /sbin/busybox [ "`/sbin/busybox grep scheduler_ext=zen $AUSDIM_CONFIG`" ]; then
  SCHEDULER="zen"
fi

if /sbin/busybox [ "`/sbin/busybox grep scheduler_ext=fifo $AUSDIM_CONFIG`" ]; then
  SCHEDULER="fifo"
fi

if [ ! -z $SCHEDULER ]; then
  echo "$SCHEDULER" > /sys/block/mmcblk1/queue/scheduler
  echo $(date) External sd memory scheduler set to $SCHEDULER >> $AUSDIM_LOGFILE
fi
