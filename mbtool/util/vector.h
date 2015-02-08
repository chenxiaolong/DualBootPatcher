/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#pragma once

#define VECTOR(TYPE, NAME)                                                                      \
    struct NAME {                                                                               \
        TYPE *list;                                                                             \
        size_t len;                                                                             \
        size_t capacity;                                                                        \
    }

#define VECTOR_PUSH_BACK(VECTOR, VALUE, ERROR)                                                  \
    do {                                                                                        \
        if ((VECTOR)->len == 0 || (VECTOR)->len == (VECTOR)->capacity) {                        \
            (VECTOR)->capacity = ((VECTOR)->len == 0) ? 1 : (VECTOR)->capacity << 1;            \
            void *temp = realloc((VECTOR)->list, (VECTOR)->capacity * sizeof((VALUE)));         \
            if (temp) {                                                                         \
                (VECTOR)->list = temp;                                                          \
            } else {                                                                            \
                goto ERROR;                                                                     \
            }                                                                                   \
        }                                                                                       \
                                                                                                \
        (VECTOR)->list[(VECTOR)->len] = (VALUE);                                                \
        ++(VECTOR)->len;                                                                        \
    } while (0)

#define VECTOR_POP_VALUE(VECTOR, VALUE, ERROR) \
    do {                                                                                        \
        int index = -1;                                                                         \
                                                                                                \
        for (size_t i = 0; i < (VECTOR)->len; ++i) {                                            \
            if ((VECTOR)->list[i] == (VALUE)) {                                                 \
                index = i;                                                                      \
                break;                                                                          \
            }                                                                                   \
        }                                                                                       \
                                                                                                \
        if (index < 0) {                                                                        \
            goto ERROR;                                                                         \
        }                                                                                       \
                                                                                                \
        memmove((VECTOR)->list + index, (VECTOR)->list + index + 1,                             \
                ((VECTOR)->len - index - 1) * sizeof((VALUE)));                                 \
        --(VECTOR)->len;                                                                        \
                                                                                                \
        if ((VECTOR)->len <= ((VECTOR)->capacity >> 1)) {                                       \
            (VECTOR)->capacity >>= 1;                                                           \
            void *temp = realloc((VECTOR)->list, (VECTOR)->capacity * sizeof((VALUE)));         \
            if (temp) {                                                                         \
                (VECTOR)->list = temp;                                                          \
            } else if ((VECTOR)->capacity == 0) {                                               \
                (VECTOR)->list = NULL;                                                          \
            } else {                                                                            \
                goto ERROR;                                                                     \
            }                                                                                   \
        }                                                                                       \
    } while (0)
