#!/sbin/sh

# Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
