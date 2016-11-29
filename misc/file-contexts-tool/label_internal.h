/*
 * This file describes the internal interface used by the labeler
 * for calling the user-supplied memory allocation, validation,
 * and locking routine.
 *
 * Author : Eamon Walsh <ewalsh@epoch.ncsc.mil>
 */

#pragma once

#include <stdlib.h>
#include <stdarg.h>

/*
 * Labeling internal structures
 */
struct selabel_lookup_rec {
	char * ctx_raw;
};

struct selabel_handle {
	/* supports backend-specific state information */
	void *data;
};

/*
 * The read_spec_entries function may be used to
 * replace sscanf to read entries from spec files.
 */
extern int read_spec_entries(char *line_buf, const char **errbuf, int num_args, ...);
