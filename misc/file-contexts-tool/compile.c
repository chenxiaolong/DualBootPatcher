#define _GNU_SOURCE

#include "compile.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "callbacks.h"
#include "label_file.h"

static int process_file(struct pcre_shim *shim,
                        struct selabel_handle *rec, const char *filename)
{
    unsigned int line_num;
    int rc;
    char *line_buf = NULL;
    size_t line_len = 0;
    FILE *context_file;
    const char *prefix = NULL;

    context_file = fopen(filename, "re");
    if (!context_file) {
        selinux_log("%s: Failed to open for reading: %s\n",
                    filename, strerror(errno));
        return -1;
    }

    line_num = 0;
    rc = 0;
    while (getline(&line_buf, &line_len, context_file) > 0) {
        rc = process_line(shim, rec, filename, prefix, line_buf, ++line_num);
        if (rc) {
            goto out;
        }
    }
out:
    free(line_buf);
    fclose(context_file);
    return rc;
}

/*
 * File Format
 *
 * u32 - magic number
 * u32 - version
 * u32 - length of pcre version EXCLUDING nul
 * char - pcre version string EXCLUDING nul
 * u32 - number of stems
 * ** Stems
 * u32  - length of stem EXCLUDING nul
 * char - stem char array INCLUDING nul
 * u32 - number of regexs
 * ** Regexes
 * u32  - length of upcoming context INCLUDING nul
 * char - char array of the raw context
 * u32  - length of the upcoming regex_str
 * char - char array of the original regex string including the stem.
 * u32  - mode bits for >= SELINUX_COMPILED_FCONTEXT_MODE
 *        mode_t for <= SELINUX_COMPILED_FCONTEXT_PCRE_VERS
 * s32  - stemid associated with the regex
 * u32  - spec has meta characters
 * u32  - The specs prefix_len if >= SELINUX_COMPILED_FCONTEXT_PREFIX_LEN
 * u32  - data length of the pcre regex
 * char - a bufer holding the raw pcre regex info
 * u32  - data length of the pcre regex study daya
 * char - a buffer holding the raw pcre regex study data
 */
static int write_binary_file(struct pcre_shim *shim,
                             struct saved_data *data, int fd)
{
    struct spec *specs = data->spec_arr;
    FILE *bin_file;
    size_t len;
    uint32_t magic = SELINUX_MAGIC_COMPILED_FCONTEXT;
    uint32_t section_len;
    uint32_t i;
    int rc;

    bin_file = fdopen(fd, "we");
    if (!bin_file) {
        selinux_log("Failed to open output file for writing: %s\n",
                    strerror(errno));
        return -1;
    }

    /* write some magic number */
    len = fwrite(&magic, sizeof(uint32_t), 1, bin_file);
    if (len != 1) {
        goto err;
    }

    /* write the version */
    section_len = SELINUX_COMPILED_FCONTEXT_MAX_VERS;
    len = fwrite(&section_len, sizeof(uint32_t), 1, bin_file);
    if (len != 1) {
        goto err;
    }

    /* write version of the regex back-end */
    if (!regex_version(shim)) {
        goto err;
    }
    section_len = strlen(regex_version(shim));
    len = fwrite(&section_len, sizeof(uint32_t), 1, bin_file);
    if (len != 1) {
        goto err;
    }
    len = fwrite(regex_version(shim), sizeof(char), section_len, bin_file);
    if (len != section_len) {
        goto err;
    }

    /* write the number of stems coming */
    section_len = data->num_stems;
    len = fwrite(&section_len, sizeof(uint32_t), 1, bin_file);
    if (len != 1) {
        goto err;
    }

    for (i = 0; i < section_len; i++) {
        char *stem = data->stem_arr[i].buf;
        uint32_t stem_len = data->stem_arr[i].len;

        /* write the strlen (aka no nul) */
        len = fwrite(&stem_len, sizeof(uint32_t), 1, bin_file);
        if (len != 1) {
            goto err;
        }

        /* include the nul in the file */
        stem_len += 1;
        len = fwrite(stem, sizeof(char), stem_len, bin_file);
        if (len != stem_len) {
            goto err;
        }
    }

    /* write the number of regexes coming */
    section_len = data->nspec;
    len = fwrite(&section_len, sizeof(uint32_t), 1, bin_file);
    if (len != 1) {
        goto err;
    }

    for (i = 0; i < section_len; i++) {
        char *context = specs[i].lr.ctx_raw;
        char *regex_str = specs[i].regex_str;
        mode_t mode = specs[i].mode;
        size_t prefix_len = specs[i].prefix_len;
        int32_t stem_id = specs[i].stem_id;
        struct regex_data *re = specs[i].regex;
        uint32_t to_write;

        /* length of the context string (including nul) */
        to_write = strlen(context) + 1;
        len = fwrite(&to_write, sizeof(uint32_t), 1, bin_file);
        if (len != 1) {
            goto err;
        }

        /* original context string (including nul) */
        len = fwrite(context, sizeof(char), to_write, bin_file);
        if (len != to_write) {
            goto err;
        }

        /* length of the original regex string (including nul) */
        to_write = strlen(regex_str) + 1;
        len = fwrite(&to_write, sizeof(uint32_t), 1, bin_file);
        if (len != 1) {
            goto err;
        }

        /* original regex string */
        len = fwrite(regex_str, sizeof(char), to_write, bin_file);
        if (len != to_write) {
            goto err;
        }

        /* binary F_MODE bits */
        to_write = mode;
        len = fwrite(&to_write, sizeof(uint32_t), 1, bin_file);
        if (len != 1) {
            goto err;
        }

        /* stem for this regex (could be -1) */
        len = fwrite(&stem_id, sizeof(stem_id), 1, bin_file);
        if (len != 1) {
            goto err;
        }

        /* does this spec have a metaChar? */
        to_write = specs[i].hasMetaChars;
        len = fwrite(&to_write, sizeof(to_write), 1, bin_file);
        if (len != 1) {
            goto err;
        }

        /* For SELINUX_COMPILED_FCONTEXT_PREFIX_LEN */
        to_write = prefix_len;
        len = fwrite(&to_write, sizeof(to_write), 1, bin_file);
        if (len != 1) {
            goto err;
        }

        /* Write regex related data */
        rc = regex_writef(shim, re, bin_file);
        if (rc < 0) {
            goto err;
        }
    }

    rc = 0;
out:
    fclose(bin_file);
    return rc;
err:
    rc = -1;
    goto out;
}

static void free_specs(struct pcre_shim *shim, struct saved_data *data)
{
    struct spec *specs = data->spec_arr;
    unsigned int num_entries = data->nspec;
    unsigned int i;

    for (i = 0; i < num_entries; i++) {
        free(specs[i].lr.ctx_raw);
        free(specs[i].regex_str);
        free(specs[i].type_str);
        regex_data_free(shim, specs[i].regex);
    }
    free(specs);

    num_entries = data->num_stems;
    for (i = 0; i < num_entries; i++) {
        free(data->stem_arr[i].buf);
    }
    free(data->stem_arr);

    memset(data, 0, sizeof(*data));
}

int compile(struct pcre_shim *shim,
            const char *source_file, const char *target_file)
{
    char *tmp = NULL;
    int fd, rc;
    struct selabel_handle *rec = NULL;
    struct saved_data *data = NULL;
    struct stat sb;

    if (stat(source_file, &sb) < 0) {
        selinux_log("%s: Failed to stat file: %s\n",
                    source_file, strerror(errno));
        return -1;
    }

    /* Generate dummy handle for process_line() function */
    rec = (struct selabel_handle *) calloc(1, sizeof(*rec));
    if (!rec) {
        selinux_log("Failed to calloc handle: %s\n", strerror(errno));
        return -1;
    }

    data = (struct saved_data *) calloc(1, sizeof(*data));
    if (!data) {
        selinux_log("Failed to calloc saved_data: %s\n", strerror(errno));
        free(rec);
        return -1;
    }

    rec->data = data;

    rc = process_file(shim, rec, source_file);
    if (rc < 0) {
        goto err;
    }

    rc = sort_specs(data);
    if (rc) {
        goto err;
    }

    rc = asprintf(&tmp, "%s.XXXXXX", target_file);
    if (rc < 0) {
        goto err;
    }

    fd = mkstemp(tmp);
    if (fd < 0) {
        selinux_log("%s: Failed to open for writing: %s\n",
                    tmp, strerror(errno));
        goto err;
    }

    if (fchmod(fd, sb.st_mode) < 0) {
        selinux_log("%s: Failed to chmod file: %s\n",
                    tmp, strerror(errno));
        goto err_unlink;
    }

    rc = write_binary_file(shim, data, fd);
    if (rc < 0) {
        goto err_unlink;
    }

    rc = rename(tmp, target_file);
    if (rc < 0) {
        selinux_log("%s: Failed to rename to target: %s\n",
                    tmp, strerror(errno));
        goto err_unlink;
    }

    rc = 0;
out:
    free_specs(shim, data);
    free(rec);
    free(data);
    free(tmp);

    return rc;

err_unlink:
    unlink(tmp);
err:
    rc = -1;
    goto out;
}
