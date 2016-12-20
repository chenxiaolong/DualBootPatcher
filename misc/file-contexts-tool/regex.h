#pragma once

#include <stdio.h>

#include "pcre_shim.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    REGEX_MATCH,
    REGEX_MATCH_PARTIAL,
    REGEX_NO_MATCH,
    REGEX_ERROR = -1,
};

struct regex_error_data {
    // For PCRE2
    int p2_error_code;
    PCRE2_SIZE p2_error_offset;

    // For PCRE1
    char const * p1_error_buffer;
    int p1_error_offset;
};

struct regex_data {
    // For PCRE2
    pcre2_code * p2_regex; /* compiled regular expression */
    pcre2_match_data * p2_match_data; /* match data block required for the compiled pattern in regex2 */

    // For PCRE1
    pcre *p1_regex; /* compiled regular expression */
    int p1_extra_owned;
    union {
        pcre_extra *p1_sd; /* pointer to extra compiled stuff */
        pcre_extra p1_lsd; /* used to hold the mmap'd version */
    };
};

/**
 * regex_verison returns the version string of the underlying regular
 * regular expressions library. In the case of PCRE it just returns the
 * result of pcre_version(). In the case of PCRE2, the very first time this
 * function is called it allocates a buffer large enough to hold the version
 * string and reads the PCRE2_CONFIG_VERSION option to fill the buffer.
 * The allocated buffer will linger in memory until the calling process is being
 * reaped.
 *
 * It may return NULL on error.
 */
char const * regex_version(struct pcre_shim *shim);
/**
 * This constructor function allocates a buffer for a regex_data structure.
 * The buffer is being initialized with zeroes.
 */
struct regex_data * regex_data_create(struct pcre_shim *shim);
/**
 * This complementary destructor function frees the a given regex_data buffer.
 * It also frees any non NULL member pointers with the appropriate pcreX_X_free
 * function. For PCRE this function respects the extra_owned field and frees
 * the pcre_extra data conditionally. Calling this function on a NULL pointer is
 * save.
 */
void regex_data_free(struct pcre_shim *shim, struct regex_data * regex);
/**
 * This function compiles the regular expression. Additionally, it prepares
 * data structures required by the different underlying engines. For PCRE
 * it calls pcre_study to generate optional data required for optimized
 * execution of the compiled pattern. In the case of PCRE2, it allocates
 * a pcre2_match_data structure of appropriate size to hold all possible
 * matches created by the pattern.
 *
 * @arg regex If successful, the structure returned through *regex was allocated
 *            with regex_data_create and must be freed with regex_data_free.
 * @arg pattern_string The pattern string that is to be compiled.
 * @arg errordata A pointer to a regex_error_data structure must be passed
 *                to this function. This structure depends on the underlying
 *                implementation. It can be passed to regex_format_error
 *                to generate a human readable error message.
 * @retval 0 on success
 * @retval -1 on error
 */
int regex_prepare_data(struct pcre_shim *shim, struct regex_data ** regex,
                       char const * pattern_string,
                       struct regex_error_data * errordata);
/**
 * This function stores a precompiled regular expression to a file.
 * In the case of PCRE, it just dumps the binary representation of the
 * precomplied pattern into a file. In the case of PCRE2, it uses the
 * serialization function provided by the library.
 *
 * @arg regex The precomplied regular expression data.
 * @arg fp A file stream specifying the output file.
 */
int regex_writef(struct pcre_shim *shim, struct regex_data * regex, FILE * fp);
/**
 * This function takes the error data returned by regex_prepare_data and turns
 * it in to a human readable error message.
 * If the buffer given to hold the error message is to small it truncates the
 * message and indicates the truncation with an ellipsis ("...") at the end of
 * the buffer.
 *
 * @arg error_data Error data as returned by regex_prepare_data.
 * @arg buffer String buffer to hold the formated error string.
 * @arg buf_size Total size of the given bufer in bytes.
 */
void regex_format_error(struct pcre_shim *shim,
                        struct regex_error_data const * error_data,
                        char * buffer, size_t buf_size);

#ifdef __cplusplus
}
#endif
