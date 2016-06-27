/*
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 * Copyright (C) 2014 Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mntent.h"
#if 0
#include <bits/uClibc_mutex.h>
#else
#include <pthread.h>
#define __UCLIBC_MUTEX_STATIC(M,I)  static pthread_mutex_t M = I
#define __UCLIBC_MUTEX_LOCK(M)      pthread_mutex_lock(&(M))
#define __UCLIBC_MUTEX_UNLOCK(M)    pthread_mutex_unlock(&(M))
#endif

#ifdef __ANDROID__
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

__UCLIBC_MUTEX_STATIC(mylock, PTHREAD_MUTEX_INITIALIZER);



/* Reentrant version of getmntent.  */
struct mntent *getmntent_r (FILE *filep,
	struct mntent *mnt, char *buff, int bufsize)
{
	static const char sep[] = " \t\n";

	char *cp, *ptrptr;

	if (!filep || !mnt || !buff)
	    return NULL;

	/* Loop on the file, skipping comment lines. - FvK 03/07/93 */
	while ((cp = fgets(buff, bufsize, filep)) != NULL) {
		if (buff[0] == '#' || buff[0] == '\n')
			continue;
		break;
	}

	/* At the EOF, the buffer should be unchanged. We should
	 * check the return value from fgets ().
	 */
	if (cp == NULL)
		return NULL;

	ptrptr = 0;
	mnt->mnt_fsname = strtok_r(buff, sep, &ptrptr);
	if (mnt->mnt_fsname == NULL)
		return NULL;

	mnt->mnt_dir = strtok_r(NULL, sep, &ptrptr);
	if (mnt->mnt_dir == NULL)
		return NULL;

	mnt->mnt_type = strtok_r(NULL, sep, &ptrptr);
	if (mnt->mnt_type == NULL)
		return NULL;

	mnt->mnt_opts = strtok_r(NULL, sep, &ptrptr);
	if (mnt->mnt_opts == NULL)
		mnt->mnt_opts = "";

	cp = strtok_r(NULL, sep, &ptrptr);
	mnt->mnt_freq = (cp != NULL) ? atoi(cp) : 0;

	cp = strtok_r(NULL, sep, &ptrptr);
	mnt->mnt_passno = (cp != NULL) ? atoi(cp) : 0;

	return mnt;
}
#if 0
libc_hidden_def(getmntent_r)
#endif

struct mntent *getmntent(FILE * filep)
{
    struct mntent *tmp;
    static char *buff = NULL;
    static struct mntent mnt;
    __UCLIBC_MUTEX_LOCK(mylock);

    if (!buff) {
#ifdef __cplusplus
            buff = (char *) malloc(BUFSIZ);
#else
            buff = malloc(BUFSIZ);
#endif
		if (!buff)
		    abort();
    }

    tmp = getmntent_r(filep, &mnt, buff, BUFSIZ);
    __UCLIBC_MUTEX_UNLOCK(mylock);
    return(tmp);
}

int addmntent(FILE * filep, const struct mntent *mnt)
{
	if (fseek(filep, 0, SEEK_END) < 0)
		return 1;

	return (fprintf (filep, "%s %s %s %s %d %d\n", mnt->mnt_fsname, mnt->mnt_dir,
		 mnt->mnt_type, mnt->mnt_opts, mnt->mnt_freq, mnt->mnt_passno) < 0 ? 1 : 0);
}

char *hasmntopt(const struct mntent *mnt, const char *opt)
{
	return strstr(mnt->mnt_opts, opt);
}

FILE *setmntent(const char *name, const char *mode)
{
	return fopen(name, mode);
}
#if 0
libc_hidden_def(setmntent)
#endif

int endmntent(FILE * filep)
{
	if (filep != NULL)
		fclose(filep);
	return 1;
}
#if 0
libc_hidden_def(endmntent)
#endif
