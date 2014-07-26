#!/sbin/sh
#
# Copyright (c) 2014 Xiao-Long Chen <chenxiaolong@cxl.epac.to>
#
# Back up and restore permissions of the secondary ROM

mount /system || :

case ${1} in
backup)
  cd /system

  if [ -d dual ]; then
    /tmp/getfacl -R dual > /tmp/dual.acl.txt
    /tmp/getfattr -R -n security.selinux dual > /tmp/dual.attr.txt
  fi

  if test -n "$(find . -maxdepth 1 -name 'multi-slot-*')"; then
    for slot in multi-slot-*; do
      /tmp/getfacl -R "${slot}" > "/tmp/${slot}.acl.txt"
      /tmp/getfattr -R -n security.selinux "${slot}" > "/tmp/${slot}.attr.txt"
    done
  fi

  cd /
  ;;

restore)
  cd /system

  if [ -d dual ]; then
    /tmp/setfacl --restore=/tmp/dual.acl.txt
    /tmp/setfattr --restore=/tmp/dual.attr.txt
  fi

  if test -n "$(find . -maxdepth 1 -name 'multi-slot-*')"; then
    for slot in multi-slot-*; do
      /tmp/setfacl --restore="/tmp/${slot}.acl.txt"
      /tmp/setfattr --restore="/tmp/${slot}.attr.txt"
    done
  fi

  cd /
  ;;

*)
  echo "Usage: ${0} [backup|restore]"
  exit 1
  ;;
esac

umount /system || :
