#!/sbin/busybox sh
# universal configurator interface
# by Gokhan Moral

# You probably won't need to modify this file
# You'll need to modify the files in /res/customconfig directory

ACTION_SCRIPTS=/res/customconfig/actions
source /res/customconfig/customconfig-helper

# first, read defaults
read_defaults

# read the config from the active profile
read_config

case "${1}" in
  rename)
    rename_profile "$2" "$3"
    exit 0
    ;;
  delete)
    delete_profile "$2"
    exit 0
    ;;
  select)
    select_profile "$2"
    exit 0
    ;;
  config)
    print_config
    ;;
  list)
    list_profile
    ;;
  apply)
    apply_config
    ;;
  *)
    . ${ACTION_SCRIPTS}/$1 $1 $2 $3 $4 $5 $6
    ;;
esac;

# write back the config to the active profile
write_config
