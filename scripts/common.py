import multiboot.operatingsystem as OS
import multiboot.partitionconfigs as partitionconfigs
import multiboot.ramdisk as ramdisk

import re


def ask_ramdisk():
    ramdisks = ramdisk.list_ramdisks()
    if not ramdisks:
        raise Exception("No ramdisks found")

    ramdisks.sort()

    print("Which type of ramdisk do you have?")
    print("")

    counter = 0
    for i in ramdisks:
        print("%i: %s" % (counter + 1, ramdisk.suffix.sub('', i)))
        counter += 1

    print("")
    choice = input("Choice: ")

    try:
        choice = int(choice)
        choice -= 1
        if choice < 0 or choice >= counter:
            raise Exception("Invalid choice")

        return (ramdisks, choice)
    except ValueError:
        raise Exception("Invalid choice")


def ask_partconfig(file_info):
    unsupported_configs = []
    partition_configs = []
    partition_configs_raw = partitionconfigs.get()

    for i in partition_configs_raw:
        if file_info.is_partconfig_supported(i):
            partition_configs.append(i)
        else:
            unsupported_configs.append(i)

    if not partition_configs:
        raise Exception("No usable partition configurations found")

    print("Choose a partition configuration to use:")
    print("")

    counter = 0
    for i in partition_configs:
        print("%i: %s" % (counter + 1, i.name))
        print("\t- %s" % i.description)
        counter += 1

    for i in unsupported_configs:
        print("*: %s (UNSUPPORTED)" % i.name)
        print("\t- %s" % i.description)

    print("")
    choice = input("Choice: ")

    try:
        choice = int(choice)
        choice -= 1
        if choice < 0 or choice >= counter:
            raise Exception("Invalid choice")

        return (partition_configs, choice)
    except ValueError:
        raise Exception("Invalid choice")
