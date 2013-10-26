#!/bin/bash

if which python3 >/dev/null; then
  PYTHON=python3
elif which python2 >/dev/null; then
  PYTHON=python2
else
  PYTHON=python
fi

"${PYTHON}" -B "$(cd "$(dirname "${0}")" && pwd)/../scripts/patchramdisk.py" ${@}
