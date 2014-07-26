#!/sbin/sh
#
# Copyright (c) 2014 Xiao-Long Chen <chenxiaolong@cxl.epac.to>
#
# Back up and restore permissions of the secondary ROM

usage() {
  echo "Usage: ${0} [backup|restore] [/system|/cache]"
}

case ${1} in
backup)
  if [ "x${2}" != 'x/system' ] && [ "x${2}" != 'x/cache' ]; then
    usage
    exit 1
  fi

  cd "${2}"

  if [ -d dual ]; then
    /tmp/getfacl -R dual > "/tmp/${2#/}.dual.acl.txt"
    /tmp/getfattr -R -n security.selinux dual > "/tmp/${2#/}.dual.attr.txt"
  fi

  if test -n "$(find . -maxdepth 1 -name 'multi-slot-*')"; then
    for slot in multi-slot-*; do
      /tmp/getfacl -R "${slot}" > "/tmp/${2#/}.${slot}.acl.txt"
      /tmp/getfattr -R -n security.selinux "${slot}" > "/tmp/${2#/}.${slot}.attr.txt"
    done
  fi

  cd /
  ;;

restore)
  if [ "x${2}" != 'x/system' ] && [ "x${2}" != 'x/cache' ]; then
    usage
    exit 1
  fi

  cd "${2}"

  if [ -d dual ]; then
    /tmp/setfacl --restore="/tmp/${2#/}.dual.acl.txt"
    /tmp/setfattr --restore="/tmp/${2#/}.dual.attr.txt"
  fi

  if test -n "$(find . -maxdepth 1 -name 'multi-slot-*')"; then
    for slot in multi-slot-*; do
      /tmp/setfacl --restore="/tmp/${2#/}.${slot}.acl.txt"
      /tmp/setfattr --restore="/tmp/${2#/}.${slot}.attr.txt"
    done
  fi

  cd /
  ;;

*)
  usage
  exit 1
  ;;
esac
