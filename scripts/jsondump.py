#!/usr/bin/env python3

import sys
sys.dont_write_bytecode = True

import json

try:
    import multiboot.operatingsystem as OS
except Exception as e:
    print(str(e))
    sys.exit(1)

import multiboot.autopatcher as autopatcher
import multiboot.config as config
import multiboot.fileinfo as fileinfo
import multiboot.partitionconfigs as partitionconfigs
import multiboot.ramdisk as ramdisk


j = dict()


# Fill with partition configurations
partition_configs = partitionconfigs.get()

if not partition_configs:
    print('No partition configurations found')
    sys.exit(1)

jsonarr = list()

for i in partition_configs:
    jsonobj = dict()
    jsonobj['id'] = i.id
    jsonobj['name'] = i.name
    jsonobj['description'] = i.description
    jsonobj['kernel'] = i.kernel
    jsonarr.append(jsonobj)

j['partitionconfigs'] = jsonarr


# Fill with devices
devices = config.list_devices()

jsonarr = list()

for d in devices:
    jsonobj = dict()
    jsonobj['codename'] = d
    jsonobj['name'] = config.get(d, 'name')

    jsonarr.append(jsonobj)

j['devices'] = jsonarr


# Fill with patch information
presets = fileinfo.get_infos(None)

jsonarr = list()

for i in presets:
    jsonobj = dict()
    jsonobj['path'] = i[0][:-3]
    jsonobj['name'] = i[1].name

    if i[1].patch:
        temparr = list()

        if type(i[1].patch) != list:
            patches = [i[1].patch]
        else:
            patches = i[1].patch

        for p in patches:
            if type(p) == str:
                temparr.append(p)
            else:
                temparr.append(p.__name__)

        jsonobj['patch'] = temparr

    if i[1].extract:
        temparr = list()

        if type(i[1].extract) != list:
            extractors = [i[1].extract]
        else:
            extractors = i[1].extract

        for e in extractors:
            if type(e) == str:
                temparr.append(e)
            else:
                temparr.append(e.__name__)

        jsonobj['extract'] = temparr

    jsonobj['has_boot_image'] = i[1].has_boot_image
    if i[1].has_boot_image:
        jsonobj['bootimg'] = i[1].bootimg
        jsonobj['ramdisk'] = i[1].ramdisk
        jsonobj['loki'] = i[1].loki

        if i[1].patched_init:
            jsonobj['patched_init'] = i[1].patched_init

    jsonobj['device_check'] = i[1].device_check
    jsonobj['configs'] = i[1].configs

    jsonarr.append(jsonobj)

j['fileinfos'] = jsonarr


# Fill with autopatchers
autopatchers = autopatcher.get_auto_patchers()

jsonarr = list()

for a in autopatchers:
    jsonobj = dict()
    jsonobj['name'] = a.name

    if a.patcher:
        temparr = list()

        if type(a.patcher) != list:
            patchers = [a.patcher]
        else:
            patchers = a.patcher

        for p in patchers:
            if type(p) == str:
                temparr.append(p)
            else:
                temparr.append(p.__name__)

        jsonobj['patcher'] = temparr

    if a.extractor:
        temparr = list()

        if type(a.extractor) != list:
            extractors = [a.extractor]
        else:
            extractors = a.extractor

        for e in extractors:
            if type(e) == str:
                temparr.append(e)
            else:
                temparr.append(e.__name__)

        jsonobj['extractor'] = temparr

    jsonarr.append(jsonobj)

j['autopatchers'] = jsonarr


# Fill with inits
inits = fileinfo.get_inits()

j['inits'] = inits


# Fill with ramdisks
ramdisks = ramdisk.list_ramdisks(None)

j['ramdisks'] = [r[:-4] for r in ramdisks]


# Output final JSON
print(json.dumps(j, sort_keys=True, indent=4))
