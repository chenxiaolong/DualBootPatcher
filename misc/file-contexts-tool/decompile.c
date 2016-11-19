#define _GNU_SOURCE

#include "decompile.h"

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "label_file.h"

// Custom decompile/extract functions because we don't care about any of the
// PCRE-specific data

static const char * mode_to_string(mode_t mode)
{
    switch (mode) {
    case S_IFBLK:
        return "-b";
    case S_IFCHR:
        return "-c";
    case S_IFDIR:
        return "-d";
    case S_IFIFO:
        return "-p";
    case S_IFLNK:
        return "-l";
    case S_IFSOCK:
        return "-s";
    case S_IFREG:
        return "--";
    default:
        return NULL;
    }
}

static int skip_pcre(struct pcre_shim *shim, FILE *fp_in)
{
    size_t n;
    uint32_t entry_len;

    // Skip PCRE regex
    n = fread(&entry_len, sizeof(entry_len), 1, fp_in);

    if (shim->use_pcre2) {
        if (n != 1) {
            return -1;
        }

        if (fseek(fp_in, entry_len, SEEK_CUR) != 0) {
            return -1;
        }
    } else {
        if (n != 1 || entry_len == 0) {
            return -1;
        }

        if (fseek(fp_in, entry_len, SEEK_CUR) != 0) {
            return -1;
        }

        // Skip PCRE study data
        n = fread(&entry_len, sizeof(entry_len), 1, fp_in);
        if (n != 1 || entry_len == 0) {
            return -1;
        }

        if (fseek(fp_in, entry_len, SEEK_CUR) != 0) {
            return -1;
        }
    }

    return 0;
}

static int extract(struct pcre_shim *shim, FILE *fp_in, FILE *fp_out)
{
    size_t n;
    uint32_t magic;
    uint32_t version;
    uint32_t entry_len;
    uint32_t stem_map_len;
    uint32_t regex_array_len;

    // Check magic
    n = fread(&magic, sizeof(magic), 1, fp_in);
    if (n != 1 || magic != SELINUX_MAGIC_COMPILED_FCONTEXT) {
        selinux_log("Invalid magic field\n");
        return -1;
    }

    // Check version
    n = fread(&version, sizeof(version), 1, fp_in);
    if (n != 1 || version > SELINUX_COMPILED_FCONTEXT_MAX_VERS) {
        selinux_log("Invalid version field\n");
        return -1;
    }

    // Skip PCRE version
    if (version >= SELINUX_COMPILED_FCONTEXT_PCRE_VERS) {
        n = fread(&entry_len, sizeof(entry_len), 1, fp_in);
        if (n != 1) {
            selinux_log("Invalid PCRE version length field\n");
            return -1;
        }

        if (fseek(fp_in, entry_len, SEEK_CUR) != 0) {
            selinux_log("Invalid PCRE version field\n");
            return -1;
        }
    }

    // Skip stem map
    n = fread(&stem_map_len, sizeof(stem_map_len), 1, fp_in);
    if (n != 1 || stem_map_len == 0) {
        selinux_log("Invalid stem map length field\n");
        return -1;
    }

    for (uint32_t i = 0; i < stem_map_len; ++i) {
        uint32_t stem_len;

        // Length does not include NULL-terminator
        n = fread(&stem_len, sizeof(stem_len), 1, fp_in);
        if (n != 1 || stem_len == 0) {
            selinux_log("Invalid stem length field\n");
            return -1;
        }

        if (stem_len < UINT32_MAX) {
            if (fseek(fp_in, stem_len + 1, SEEK_CUR) != 0) {
                selinux_log("Invalid stem field\n");
                return -1;
            }
        } else {
            selinux_log("Stem length too large\n");
            return -1;
        }
    }

    // Load regexes
    n = fread(&regex_array_len, sizeof(regex_array_len), 1, fp_in);
    if (n != 1 || regex_array_len == 0) {
        selinux_log("Invalid regex array length field\n");
        return -1;
    }

    for (uint32_t i = 0; i < regex_array_len; ++i) {
        char *context = NULL;
        char *regex = NULL;
        uint32_t mode;
        const char *mode_string = NULL;

        // Read context string
        n = fread(&entry_len, sizeof(entry_len), 1, fp_in);
        if (n != 1 || entry_len == 0) {
            selinux_log("Invalid context string length field\n");
            goto loop_err;
        }

        context = malloc(entry_len);
        if (!context) {
            selinux_log("Failed to malloc: %s\n", strerror(errno));
            goto loop_err;
        }

        n = fread(context, entry_len, 1, fp_in);
        if (n != 1) {
            selinux_log("Invalid context string field\n");
            goto loop_err;
        }

        if (context[entry_len - 1] != '\0') {
            selinux_log("Context string not NULL-terminated\n");
            goto loop_err;
        }

        // Read regex string
        n = fread(&entry_len, sizeof(entry_len), 1, fp_in);
        if (n != 1 || entry_len == 0) {
            selinux_log("Invalid regex string length field\n");
            goto loop_err;
        }

        regex = malloc(entry_len);
        if (!regex) {
            selinux_log("Failed to malloc: %s\n", strerror(errno));
            goto loop_err;
        }

        n = fread(regex, entry_len, 1, fp_in);
        if (n != 1) {
            selinux_log("Invalid regex string field\n");
            goto loop_err;
        }

        if (regex[entry_len - 1] != '\0') {
            selinux_log("Regex string not NULL-terminated\n");
            goto loop_err;
        }

        // Read mode
        if (sizeof(mode_t) > sizeof(uint32_t)
                || version >= SELINUX_COMPILED_FCONTEXT_MODE) {
            n = fread(&mode, sizeof(uint32_t), 1, fp_in);
        } else {
            n = fread(&mode, sizeof(mode_t), 1, fp_in);
        }
        if (n != 1) {
            selinux_log("Invalid mode value field\n");
            goto loop_err;
        }
        mode_string = mode_to_string(mode);
        if (mode != 0 && !mode_string) {
            selinux_log("Invalid mode value\n");
            goto loop_err;
        }

        // Skip stem ID
        if (fseek(fp_in, sizeof(int32_t), SEEK_CUR) != 0) {
            selinux_log("Invalid stem ID field\n");
            goto loop_err;
        }

        // Skip meta chars
        if (fseek(fp_in, sizeof(uint32_t), SEEK_CUR) != 0) {
            selinux_log("Invalid meta chars field\n");
            goto loop_err;
        }

        if (version >= SELINUX_COMPILED_FCONTEXT_PREFIX_LEN) {
            // Skip prefix length
            if (fseek(fp_in, sizeof(uint32_t), SEEK_CUR) != 0) {
                selinux_log("Invalid prefix length field\n");
                goto loop_err;
            }
        }

        // Skip PCRE stuff
        if (skip_pcre(shim, fp_in) < 0) {
            goto loop_err;
        }

        // Write regex
        if (fwrite(regex, strlen(regex), 1, fp_out) != 1
                || fputc(' ', fp_out) == EOF) {
            selinux_log("Failed to write regex string\n");
            goto loop_err;
        }

        // Write mode
        if (mode_string) {
            if (fwrite(mode_string, strlen(mode_string), 1, fp_out) != 1
                    || fputc(' ', fp_out) == EOF) {
                selinux_log("Failed to write mode string\n");
                goto loop_err;
            }
        }

        // Write context
        if (fwrite(context, strlen(context), 1, fp_out) != 1
                || fputc('\n', fp_out) == EOF) {
            selinux_log("Failed to write context string\n");
            goto loop_err;
        }

        free(context);
        free(regex);
        continue;

loop_err:
        free(context);
        free(regex);
        return -1;
    }

    return 0;
}

int decompile(struct pcre_shim *shim,
              const char *source_file, const char *target_file)
{
    int ret;
    int fd = -1;
    char *tmp = NULL;
    FILE *fp_in = NULL;
    FILE *fp_out = NULL;
    struct stat sb;

    fp_in = fopen(source_file, "rbe");
    if (!fp_in) {
        selinux_log("%s: Failed to open for reading: %s\n",
                    source_file, strerror(errno));
        goto err;
    }

    if (fstat(fileno(fp_in), &sb) < 0) {
        selinux_log("%s: Failed to stat file: %s\n",
                    source_file, strerror(errno));
        goto err;
    }

    if (asprintf(&tmp, "%s.XXXXXX", target_file) < 0) {
        goto err;
    }

    fd = mkstemp(tmp);
    if (fd < 0) {
        selinux_log("%s: Failed to create temporary file: %s\n",
                    tmp, strerror(errno));
        goto err;
    }

    if (fchmod(fd, sb.st_mode) < 0) {
        selinux_log("%s: Failed to chmod file: %s\n",
                    tmp, strerror(errno));
        goto err_unlink;
    }

    fp_out = fdopen(fd, "we");
    if (!fp_out) {
        selinux_log("%s: Failed to open for writing: %s\n",
                    tmp, strerror(errno));
        goto err_unlink;
    }

    // fclose() will close the file descriptor
    fd = -1;

    if (extract(shim, fp_in, fp_out) < 0) {
        selinux_log("Failed to decompile/extract file.\n");
        goto err_unlink;
    }

    if (rename(tmp, target_file) < 0) {
        selinux_log("%s: Failed to rename to target: %s\n",
                    tmp, strerror(errno));
        goto err_unlink;
    }

    ret = 0;

out:
    if (fp_in) {
        fclose(fp_in);
    }
    if (fp_out) {
        fclose(fp_out);
    }
    if (fd >= 0) {
        close(fd);
    }

    free(tmp);

    return ret;

err_unlink:
    unlink(tmp);

err:
    ret = -1;
    goto out;
}
