#!/bin/bash

set -eu

export HOME=${BUILDER_HOME}

exec "${@}"
