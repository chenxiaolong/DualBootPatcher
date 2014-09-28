#!/bin/bash

copy_trans() {
  local from="${1}"
  if [[ "${#}" -ge 2 ]]; then
    local to="${2}"
  else
    local to="${from}"
  fi

  mkdir -p Android_GUI/res/values-"${to}"
  cp translations/dualbootpatcher.android-gui/"${from}".xml \
    Android_GUI/res/values-"${to}"/strings.xml
}

copy_trans fr
copy_trans it
copy_trans pt
copy_trans ru_RU ru
