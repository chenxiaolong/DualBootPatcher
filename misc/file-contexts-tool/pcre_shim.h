#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
 * PCRE 1 forward declarations and types
 */

typedef struct pcre pcre;

// Can't use forward declaration since we need a stack-allocated version of this
// structure

typedef struct pcre_extra {
    unsigned long int flags;
    void *study_data;
    unsigned long int match_limit;
    void *callout_data;
    const unsigned char *tables;
    unsigned long int match_limit_recursion;
    unsigned char **mark;
    void *executable_jit;
} pcre_extra;

// Various needed defines

#define PCRE_INFO_SIZE              1
#define PCRE_INFO_STUDYSIZE         10
#define PCRE_DOTALL                 0x00000004


/*
 * PCRE 2 forward declarations and types
 */

typedef struct pcre2_general_context pcre2_general_context;
typedef struct pcre2_compile_context pcre2_compile_context;
typedef struct pcre2_code pcre2_code;
typedef struct pcre2_match_data pcre2_match_data;

// Various needed typedefs

typedef uint8_t PCRE2_UCHAR;
typedef const PCRE2_UCHAR *PCRE2_SPTR;
typedef size_t PCRE2_SIZE;

// Various needed defines

#define PCRE2_ZERO_TERMINATED       (~(PCRE2_SIZE) 0)
#define PCRE2_DOTALL                0x00000020u
#define PCRE2_CONFIG_VERSION        11
#define PCRE2_ERROR_NOMEMORY        (-48)


// NOTE: Please keep the ordering the same as in pcre.h and pcre2.h

struct pcre1_shim
{
    void
    (*pcre_free)(void *);

    pcre *
    (*pcre_compile)(const char *,
                    int,
                    const char **,
                    int *,
                    const unsigned char *);

    int
    (*pcre_fullinfo)(const pcre *,
                     const pcre_extra *,
                     int,
                     void *);

    pcre_extra *
    (*pcre_study)(const pcre *,
                  int,
                  const char **);

    void
    (*pcre_free_study)(pcre_extra *);

    const char *
    (*pcre_version)(void);
};

struct pcre2_shim
{
    int
    (*pcre2_config)(uint32_t,
                    void *);

    pcre2_code *
    (*pcre2_compile)(PCRE2_SPTR,
                     PCRE2_SIZE,
                     uint32_t,
                     int *,
                     PCRE2_SIZE *,
                     pcre2_compile_context *);

    void
    (*pcre2_code_free)(pcre2_code *);

    pcre2_match_data *
    (*pcre2_match_data_create_from_pattern)(const pcre2_code *,
                                            pcre2_general_context *);

    void
    (*pcre2_match_data_free)(pcre2_match_data *);

    int32_t
    (*pcre2_serialize_encode)(const pcre2_code **,
                              int32_t,
                              uint8_t **,
                              PCRE2_SIZE *,
                              pcre2_general_context *);

    void
    (*pcre2_serialize_free)(uint8_t *);

    int
    (*pcre2_get_error_message)(int,
                               PCRE2_UCHAR *,
                               PCRE2_SIZE);
};

struct pcre_shim
{
    void *handle;
    bool use_pcre2;

    struct pcre1_shim p1;
    struct pcre2_shim p2;
};

int pcre_shim_load(struct pcre_shim *shim, const char *path);
int pcre_shim_unload(struct pcre_shim *shim);

#ifdef __cplusplus
}
#endif
