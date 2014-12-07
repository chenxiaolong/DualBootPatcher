/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <jansson.h>

#include "logging.h"

#define MAIN_CONFIG "/config.json"

#define TAG_VERSION "version"
#define TAG_PARTCONFIGS "partconfigs"
#define TAG_INSTALLED "installed"
// Partconfig
#define TAG_PC_ID "id"
#define TAG_PC_KERNEL_ID "kernel-id"
#define TAG_PC_NAME "name"
#define TAG_PC_DESC "description"
#define TAG_PC_TARGET_SYSTEM "target-system"
#define TAG_PC_TARGET_CACHE "target-cache"
#define TAG_PC_TARGET_DATA "target-data"
#define TAG_PC_TARGET_SYSTEM_PARTITION "target-system-partition"
#define TAG_PC_TARGET_CACHE_PARTITION "target-cache-partition"
#define TAG_PC_TARGET_DATA_PARTITION "target-data-partition"

static struct mainconfig config;
static int loaded = 0;


int parse_config_v1();
int parse_partconfig_v1(json_t *cur, struct partconfig *partconfig);
void free_partconfig(struct partconfig *config);


struct mainconfig * get_mainconfig()
{
    if (!loaded) {
        LOGW("mainconfig_init() should be called before get_mainconfig()");
        mainconfig_init();
    }

    return &config;
}

int mainconfig_init()
{
    if (loaded) {
        LOGW("Main config file has already been loaded");
        return 0;
    }

    // Caller is responsible for checking for failures (based on return value)
    loaded = 1;

    json_t *root;
    json_error_t error;

    root = json_load_file(MAIN_CONFIG, 0, &error);
    if (!root) {
        LOGE("Failed to parse " MAIN_CONFIG " (line %d): %s",
             error.line, error.text);
        goto error;
    }

    if (!json_is_object(root)) {
        LOGE("Config root is not an object");
        goto error;
    }

    json_t *json_version = json_object_get(root, TAG_VERSION);
    if (!json_is_integer(json_version)) {
        LOGE("Config file does not specify a version");
        goto error;
    }

    int version = json_integer_value(json_version);
    switch (version) {
    case 1:
        if (parse_config_v1(root) < 0) {
            goto error;
        }
        break;

    default:
        LOGE("Unsupported config file version: %d", version);
        goto error;
    }

    LOGV("Successfully loaded configuration file (version %d", version);

    return 0;

error:
    if (root) {
        json_decref(root);
    }
    return -1;
}

int parse_config_v1(json_t *root)
{
    json_t *partconfigs, *installed;

    partconfigs = json_object_get(root, TAG_PARTCONFIGS);
    if (!json_is_array(partconfigs)) {
        LOGE("mainconfig[partconfigs] is not an array");
        goto error;
    }

    config.partconfigs_len = json_array_size(partconfigs);
    if (config.partconfigs_len == 0) {
        LOGE("mainconfig[partconfigs] is an empty array");
        goto error;
    }

    config.partconfigs = calloc(config.partconfigs_len,
                                sizeof(struct partconfig));

    for (int i = 0; i < config.partconfigs_len; ++i) {
        json_t *cur = json_array_get(partconfigs, i);

        if (parse_partconfig_v1(cur, &config.partconfigs[i]) < 0) {
            LOGE("Failed to parse mainconfig[partconfigs]");
            goto error;
        }
    }

    installed = json_object_get(root, TAG_INSTALLED);
    if (!json_is_string(installed)) {
        LOGE("mainconfig[installed] is not a string");
        goto error;
    }

    config.installed = strdup(json_string_value(installed));

    return 0;

error:
    mainconfig_cleanup();
    return -1;
}

int parse_partconfig_v1(json_t *cur, struct partconfig *partconfig)
{
    json_t *temp;

    if (!cur) {
        return -1;
    }

    temp = json_object_get(cur, TAG_PC_ID);
    if (!json_is_string(temp)) { LOGE("partconfig[id]"); return -1; }
    partconfig->id = strdup(json_string_value(temp));

    temp = json_object_get(cur, TAG_PC_KERNEL_ID);
    if (!json_is_string(temp)) { LOGE("partconfig[kernel-id]"); return -1; }
    partconfig->kernel_id = strdup(json_string_value(temp));

    temp = json_object_get(cur, TAG_PC_NAME);
    if (!json_is_string(temp)) { LOGE("partconfig[name]"); return -1; }
    partconfig->name = strdup(json_string_value(temp));

    temp = json_object_get(cur, TAG_PC_DESC);
    if (!json_is_string(temp)) { LOGE("partconfig[description]"); return -1; }
    partconfig->description = strdup(json_string_value(temp));

    temp = json_object_get(cur, TAG_PC_TARGET_SYSTEM);
    if (!json_is_string(temp)) { LOGE("partconfig[target-system]"); return -1; }
    partconfig->target_system = strdup(json_string_value(temp));

    temp = json_object_get(cur, TAG_PC_TARGET_CACHE);
    if (!json_is_string(temp)) { LOGE("partconfig[target-cache]"); return -1; }
    partconfig->target_cache = strdup(json_string_value(temp));

    temp = json_object_get(cur, TAG_PC_TARGET_DATA);
    if (!json_is_string(temp)) { LOGE("partconfig[target-data]"); return -1; }
    partconfig->target_data = strdup(json_string_value(temp));

    temp = json_object_get(cur, TAG_PC_TARGET_SYSTEM_PARTITION);
    if (!json_is_string(temp)) { LOGE("partconfig[target-system-partition]"); return -1; }
    partconfig->target_system_partition = strdup(json_string_value(temp));

    temp = json_object_get(cur, TAG_PC_TARGET_CACHE_PARTITION);
    if (!json_is_string(temp)) { LOGE("partconfig[target-cache-partition]"); return -1; }
    partconfig->target_cache_partition = strdup(json_string_value(temp));

    temp = json_object_get(cur, TAG_PC_TARGET_DATA_PARTITION);
    if (!json_is_string(temp)) { LOGE("partconfig[target-data-partition]"); return -1; }
    partconfig->target_data_partition = strdup(json_string_value(temp));

    return 0;
}

void mainconfig_cleanup()
{
    if (config.partconfigs) {
        for (int i = 0; i < config.partconfigs_len; ++i) {
            free_partconfig(&config.partconfigs[i]);
        }
        free(config.partconfigs);
        config.partconfigs = NULL;
    }

    if (config.installed) {
        free(config.installed);
    }
}

void free_partconfig(struct partconfig *config)
{
    if (config->id) {
        free(config->id);
    }
    if (config->kernel_id) {
        free(config->kernel_id);
    }
    if (config->name) {
        free(config->name);
    }
    if (config->description) {
        free(config->description);
    }
    if (config->target_system) {
        free(config->target_system);
    }
    if (config->target_cache) {
        free(config->target_cache);
    }
    if (config->target_data) {
        free(config->target_data);
    }
    if (config->target_system_partition) {
        free(config->target_system_partition);
    }
    if (config->target_cache_partition) {
        free(config->target_cache_partition);
    }
    if (config->target_data_partition) {
        free(config->target_data_partition);
    }
}
