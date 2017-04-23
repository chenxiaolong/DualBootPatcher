/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _INCLUDE_SYS__SYSTEM_PROPERTIES_H
#define _INCLUDE_SYS__SYSTEM_PROPERTIES_H

#include <sys/cdefs.h>
#include <stdint.h>

#ifndef _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#error you should #include <mbutil/external/system_properties.h> instead
#endif

#include "mbutil/external/system_properties.h"

__BEGIN_DECLS

#define MB_ENABLE_COMPAT_PROPERTIES 1

#define PROP_SERVICE_NAME "property_service"
#define PROP_FILENAME "/dev/__properties__"

#define PROP_MSG_SETPROP 1
#define PROP_MSG_SETPROP2 0x00020001

#define PROP_SUCCESS 0
#define PROP_ERROR_READ_CMD 0x0004
#define PROP_ERROR_READ_DATA 0x0008
#define PROP_ERROR_READ_ONLY_PROPERTY 0x000B
#define PROP_ERROR_INVALID_NAME 0x0010
#define PROP_ERROR_INVALID_VALUE 0x0014
#define PROP_ERROR_PERMISSION_DENIED 0x0018
#define PROP_ERROR_INVALID_CMD 0x001B
#define PROP_ERROR_HANDLE_CONTROL_MESSAGE 0x0020
#define PROP_ERROR_SET_FAILED 0x0024

/*
** Map the property area from the specified filename.  This
** method is for testing only.
*/
int mb__system_property_set_filename(const char *filename);

/*
** Initialize the area to be used to store properties.  Can
** only be done by a single process that has write access to
** the property area.
*/
int mb__system_property_area_init();

/* Read the global serial number of the system properties
**
** Called to predict if a series of cached mb__system_property_find
** objects will have seen mb__system_property_serial values change.
** But also aids the converse, as changes in the global serial can
** also be used to predict if a failed mb__system_property_find
** could in-turn now find a new object; thus preventing the
** cycles of effort to poll mb__system_property_find.
**
** Typically called at beginning of a cache cycle to signal if _any_ possible
** changes have occurred since last. If there is, one may check each individual
** mb__system_property_serial to confirm dirty, or mb__system_property_find
** to check if the property now exists. If a call to mb__system_property_add
** or mb__system_property_update has completed between two calls to
** mb__system_property_area_serial then the second call will return a larger
** value than the first call. Beware of race conditions as changes to the
** properties are not atomic, the main value of this call is to determine
** whether the expensive mb__system_property_find is worth retrying to see if
** a property now exists.
**
** Returns the serial number on success, -1 on error.
*/
uint32_t mb__system_property_area_serial();

/* Add a new system property.  Can only be done by a single
** process that has write access to the property area, and
** that process must handle sequencing to ensure the property
** does not already exist and that only one property is added
** or updated at a time.
**
** Returns 0 on success, -1 if the property area is full.
*/
int mb__system_property_add(const char *name, unsigned int namelen, const char *value, unsigned int valuelen);

/* Update the value of a system property returned by
** mb__system_property_find.  Can only be done by a single process
** that has write access to the property area, and that process
** must handle sequencing to ensure that only one property is
** updated at a time.
**
** Returns 0 on success, -1 if the parameters are incorrect.
*/
int mb__system_property_update(prop_info *pi, const char *value, unsigned int len);

/* Read the serial number of a system property returned by
** mb__system_property_find.
**
** Returns the serial number on success, -1 on error.
*/
uint32_t mb__system_property_serial(const prop_info* pi);

/* Initialize the system properties area in read only mode.
 * Should be done by all processes that need to read system
 * properties.
 *
 * Returns 0 on success, -1 otherwise.
 */
int mb__system_properties_init();

/* Deprecated: use mb__system_property_wait instead. */
uint32_t mb__system_property_wait_any(uint32_t old_serial);

#if MB_ENABLE_COMPAT_PROPERTIES
/*  Compatibility functions to support using an old init with a new libc,
 ** mostly for the OTA updater binary.  These can be deleted once OTAs from
 ** a pre-K release no longer needed to be supported. */
const prop_info *mb__system_property_find_compat(const char *name);
int mb__system_property_read_compat(const prop_info *pi, char *name, char *value);
int mb__system_property_foreach_compat(
        void (*propfn)(const prop_info *pi, void *cookie),
        void *cookie);
#endif

__END_DECLS

#endif
