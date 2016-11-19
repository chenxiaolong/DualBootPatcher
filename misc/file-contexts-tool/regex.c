#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "regex.h"
#include "label_file.h"

int regex_prepare_data(struct pcre_shim *shim, struct regex_data ** regex,
                       char const * pattern_string,
                       struct regex_error_data * errordata)
{
    memset(errordata, 0, sizeof(struct regex_error_data));

    *regex = regex_data_create(shim);
    if (!(*regex)) {
        return -1;
    }

    if (shim->use_pcre2) {
        (*regex)->p2_regex =
                shim->p2.pcre2_compile((PCRE2_SPTR) pattern_string,
                                       PCRE2_ZERO_TERMINATED,
                                       PCRE2_DOTALL,
                                       &errordata->p2_error_code,
                                       &errordata->p2_error_offset, NULL);
        if (!(*regex)->p2_regex) {
            goto err;
        }

        (*regex)->p2_match_data =
                shim->p2.pcre2_match_data_create_from_pattern(
                        (*regex)->p2_regex, NULL);
        if (!(*regex)->p2_match_data) {
            goto err;
        }
    } else {
        (*regex)->p1_regex =
                shim->p1.pcre_compile(pattern_string, PCRE_DOTALL,
                                      &errordata->p1_error_buffer,
                                      &errordata->p1_error_offset, NULL);
        if (!(*regex)->p1_regex) {
            goto err;
        }

        (*regex)->p1_sd = shim->p1.pcre_study(
                (*regex)->p1_regex, 0, &errordata->p1_error_buffer);
        if (!(*regex)->p1_sd && errordata->p1_error_buffer) {
            goto err;
        }
        (*regex)->p1_extra_owned = !!(*regex)->p1_sd;
    }

    return 0;

err:
    regex_data_free(shim, *regex);
    *regex = NULL;
    return -1;
}

char const * regex_version(struct pcre_shim *shim)
{
    if (shim->use_pcre2) {
        static int initialized = 0;
        static char * version_string = NULL;
        size_t version_string_len;
        if (!initialized) {
            version_string_len =
                    shim->p2.pcre2_config(PCRE2_CONFIG_VERSION, NULL);
            version_string = (char*) malloc(version_string_len);
            if (!version_string) {
                return NULL;
            }
            shim->p2.pcre2_config(PCRE2_CONFIG_VERSION, version_string);
            initialized = 1;
        }
        return version_string;
    } else {
        return shim->p1.pcre_version();
    }
}

int regex_writef(struct pcre_shim *shim, struct regex_data * regex, FILE * fp)
{
    int rc;
    size_t len;

    if (shim->use_pcre2) {
        PCRE2_UCHAR * bytes;
        PCRE2_SIZE to_write;

#ifndef NO_PERSISTENTLY_STORED_PATTERNS
        /* encode the patter for serialization */
        rc = shim->p2.pcre2_serialize_encode(
                (const pcre2_code **) &regex->p2_regex, 1, &bytes, &to_write, NULL);
        if (rc != 1) {
            return -1;
        }

#else
        (void) regex; // silence unused parameter warning
        to_write = 0;
#endif
        /* write serialized pattern's size */
        len = fwrite(&to_write, sizeof(uint32_t), 1, fp);
        if (len != 1) {
#ifndef NO_PERSISTENTLY_STORED_PATTERNS
            shim->p2.pcre2_serialize_free(bytes);
#endif
            return -1;
        }

#ifndef NO_PERSISTENTLY_STORED_PATTERNS
        /* write serialized pattern */
        len = fwrite(bytes, 1, to_write, fp);
        if (len != to_write) {
            shim->p2.pcre2_serialize_free(bytes);
            return -1;
        }
        shim->p2.pcre2_serialize_free(bytes);
#endif
    } else {
        uint32_t to_write;
        size_t size;
        pcre_extra * sd = regex->p1_extra_owned ? regex->p1_sd : &regex->p1_lsd;

        /* determine the size of the pcre data in bytes */
        rc = shim->p1.pcre_fullinfo(
                regex->p1_regex, NULL, PCRE_INFO_SIZE, &size);
        if (rc < 0) {
            return -1;
        }

        /* write the number of bytes in the pcre data */
        to_write = size;
        len = fwrite(&to_write, sizeof(uint32_t), 1, fp);
        if (len != 1) {
            return -1;
        }

        /* write the actual pcre data as a char array */
        len = fwrite(regex->p1_regex, 1, to_write, fp);
        if (len != to_write) {
            return -1;
        }

        /* determine the size of the pcre study info */
        rc = shim->p1.pcre_fullinfo(
                regex->p1_regex, sd, PCRE_INFO_STUDYSIZE, &size);
        if (rc < 0) {
            return -1;
        }

        /* write the number of bytes in the pcre study data */
        to_write = size;
        len = fwrite(&to_write, sizeof(uint32_t), 1, fp);
        if (len != 1) {
            return -1;
        }

        /* write the actual pcre study data as a char array */
        len = fwrite(sd->study_data, 1, to_write, fp);
        if (len != to_write) {
            return -1;
        }
    }
    return 0;
}

struct regex_data * regex_data_create(struct pcre_shim *shim)
{
    (void) shim;

    struct regex_data * dummy =
            (struct regex_data *) malloc(sizeof(struct regex_data));
    if (dummy) {
        memset(dummy, 0, sizeof(struct regex_data));
    }
    return dummy;
}

void regex_data_free(struct pcre_shim *shim, struct regex_data * regex)
{
    if (regex) {
        if (shim->use_pcre2) {
            if (regex->p2_regex) {
                shim->p2.pcre2_code_free(regex->p2_regex);
            }
            if (regex->p2_match_data) {
                shim->p2.pcre2_match_data_free(regex->p2_match_data);
            }
        } else {
            if (regex->p1_regex) {
                shim->p1.pcre_free(regex->p1_regex);
            }
            if (regex->p1_extra_owned && regex->p1_sd) {
                shim->p1.pcre_free_study(regex->p1_sd);
            }
        }
        free(regex);
    }
}

void regex_format_error(struct pcre_shim *shim,
                        struct regex_error_data const * error_data,
                        char * buffer, size_t buf_size)
{
    unsigned the_end_length = buf_size > 4 ? 4 : buf_size;
    char * ptr = &buffer[buf_size - the_end_length];
    int rc = 0;
    size_t pos = 0;
    if (!buffer || !buf_size) {
        return;
    }
    rc = snprintf(buffer, buf_size, "REGEX back-end error: ");
    if (rc < 0) {
        /* If snprintf fails it constitutes a logical error that needs
         * fixing.
         */
        abort();
    }

    pos += rc;
    if (pos >= buf_size) {
        goto truncated;
    }

    if (shim->use_pcre2) {
        if (error_data->p2_error_offset > 0) {
            rc = snprintf(buffer + pos, buf_size - pos, "At offset %zu: ",
                          error_data->p2_error_offset);
            if (rc < 0) {
                abort();
            }
        }
    } else {
        if (error_data->p1_error_offset > 0) {
            rc = snprintf(buffer + pos, buf_size - pos, "At offset %d: ",
                          error_data->p1_error_offset);
            if (rc < 0) {
                abort();
            }
        }
    }

    pos += rc;
    if (pos >= buf_size) {
        goto truncated;
    }

    if (shim->use_pcre2) {
        rc = shim->p2.pcre2_get_error_message(error_data->p2_error_code,
                                              (PCRE2_UCHAR *)(buffer + pos),
                                              buf_size - pos);
        if (rc == PCRE2_ERROR_NOMEMORY) {
            goto truncated;
        }
    } else {
        rc = snprintf(buffer + pos, buf_size - pos, "%s",
                      error_data->p1_error_buffer);
        if (rc < 0) {
            abort();
        }

        if ((size_t) rc < strlen(error_data->p1_error_buffer)) {
            goto truncated;
        }
    }

    return;

truncated:
    /* replace end of string with "..." to indicate that it was truncated */
    switch (the_end_length) {
        /* no break statements, fall-through is intended */
        case 4:
            *ptr++ = '.';
        case 3:
            *ptr++ = '.';
        case 2:
            *ptr++ = '.';
        case 1:
            *ptr++ = '\0';
        default:
            break;
    }
    return;
}
