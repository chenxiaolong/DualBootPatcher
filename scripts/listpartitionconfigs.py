#!/usr/bin/env python3

import sys

sys.dont_write_bytecode = True

try:
    import multiboot.operatingsystem as OS
except Exception as e:
    print(str(e))
    sys.exit(1)

import multiboot.partitionconfigs as partitionconfigs

to_file = False
if len(sys.argv) == 2:
    to_file = True


def output(line):
    if to_file:
        f.write((line + '\n').encode("UTF-8"))
    else:
        print(line)

if to_file:
    f = open(sys.argv[1], 'wb')

partition_configs = partitionconfigs.get()

if not partition_configs:
    print("No partition configurations found")
    sys.exit(1)

ids = []

for i in partition_configs:
    ids.append(i.id)
    output("%s.%s=%s" % (i.id, 'name', i.name))
    output("%s.%s=%s" % (i.id, 'description', i.description))
    output("%s.%s=%s" % (i.id, 'kernel', i.kernel))

output("partitionconfigs=" + ','.join(ids))

if to_file:
    f.close()
