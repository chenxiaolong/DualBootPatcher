/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#ifdef _WIN32
#  if defined(MB_DYNAMIC_LINK)
#    if defined(MB_LIBRARY)
#      define MB_EXPORT __declspec(dllexport)
#    else
#      define MB_EXPORT __declspec(dllimport)
#    endif
#  else
#    define MB_EXPORT
#  endif
#endif

#ifndef MB_EXPORT
#  if defined(__GNUC__)
#    define MB_EXPORT __attribute__ ((visibility ("default")))
#  else
#    define MB_EXPORT
#  endif
#endif

#ifdef _MSC_VER
#  pragma warning(disable:4251) // class ... needs to have dll-interface ...
#endif

#ifdef _MSC_VER
#  define strdup _strdup
#endif

#ifdef __cplusplus
#  define MB_BEGIN_C_DECLS extern "C" {
#  define MB_END_C_DECLS }
#else
#  define MB_BEGIN_C_DECLS
#  define MB_END_C_DECLS
#endif

#ifdef __cplusplus
#  define MB_NO_RETURN [[noreturn]]
#endif

#if defined(__GNUC__) || defined(__clang__)
#  if _WIN32
#    define MB_PRINTF_FORMAT __MINGW_PRINTF_FORMAT
#    define MB_SCANF_FORMAT __MINGW_SCANF_FORMAT
#  else
#    define MB_PRINTF_FORMAT printf
#    define MB_SCANF_FORMAT scanf
#  endif
#  define MB_PRINTF(fmt_arg, var_arg) \
    __attribute__((format(MB_PRINTF_FORMAT, fmt_arg, var_arg)))
#  define MB_SCANF(fmt_arg, var_arg) \
    __attribute__((format(MB_SCANF_FORMAT, fmt_arg, var_arg)))
#  define MB_UNUSED __attribute__((unused))
#  ifndef MB_NO_RETURN
#    define MB_NO_RETURN __attribute__((noreturn))
#  endif
#  if __cplusplus > 201402L
#    error Remove MB_NO_DISCARD after switching to C++17
#    error Remove MB_NO_DISCARD_FUNC after switching to C++17
#  elif defined(__clang__)
#    define MB_NO_DISCARD [[clang::warn_unused_result]]
#    define MB_NO_DISCARD_FUNC [[clang::warn_unused_result]]
#  elif defined(__GNUC__)
     // GCC does not support warn_unused_result on types
#    define MB_NO_DISCARD
#    define MB_NO_DISCARD_FUNC [[gnu::warn_unused_result]]
#  endif
#elif defined(_MSC_VER)
#  ifndef MB_NO_RETURN
#    define MB_NO_RETURN __declspec(noreturn)
#  endif
#  if __cplusplus > 201402L
#    error Remove MB_NO_DISCARD after switching to C++17
#    error Remove MB_NO_DISCARD_FUNC after switching to C++17
#  else
#    define MB_NO_DISCARD _Check_return_
#    define MB_NO_DISCARD_FUNC _Check_return_
#  endif
#endif

#ifndef MB_PRINTF
#  define MB_PRINTF(fmtarg, firstvararg)
#endif
#ifndef MB_SCANF
#  define MB_SCANF(fmtarg, firstvararg)
#endif
#ifndef MB_UNUSED
#  define MB_UNUSED
#endif
#ifndef MB_NO_RETURN
#  define MB_NO_RETURN
#endif

#if defined(__GNUC__)
#  define MB_BUILTIN_UNREACHABLE __builtin_unreachable()
#elif defined(_MSC_VER)
#  define MB_BUILTIN_UNREACHABLE __assume(false)
#endif

#ifndef NDEBUG
#  define MB_UNREACHABLE(FMT, ...) \
       ::mb_unreachable(__FILE__, __LINE__, __func__, FMT, __VA_ARGS__)
#elif defined(MB_BUILTIN_UNREACHABLE)
#  define MB_UNREACHABLE(FMT, ...) MB_BUILTIN_UNREACHABLE
#else
#  define MB_UNREACHABLE(FMT, ...) ::mb_unreachable(NULL, 0, NULL, NULL)
#endif

/*!
 * This function calls abort(), and prints the optional message to stderr.
 * Use the MB_UNREACHABLE macro (that adds location info), instead of
 * calling this function directly.
 */
MB_NO_RETURN MB_EXPORT
// stdio.h might not have been included yet, which would make
// __MINGW_PRINTF_FORMAT undefined on mingw
#if !defined(_WIN32) || defined(__MINGW_PRINTF_FORMAT)
MB_PRINTF(4, 5)
#endif
void mb_unreachable(const char *file, unsigned long line,
                    const char *func, const char *fmt, ...);

// C++-only macros and functions
#ifdef __cplusplus

// pimpl macros (inspired by Qt)
template <typename T>
static inline T * mb_get_ptr_helper(T *ptr)
{
    return ptr;
}

template <typename T>
static inline typename T::pointer mb_get_ptr_helper(const T &p)
{
    return p.get();
}

#define MB_DECLARE_PRIVATE(CLASS) \
    inline CLASS ## Private * _priv_func() { \
        return reinterpret_cast<CLASS ## Private *>(mb_get_ptr_helper(_priv_ptr)); \
    } \
    inline const CLASS ## Private* _priv_func() const { \
        return reinterpret_cast<const CLASS ## Private *>(mb_get_ptr_helper(_priv_ptr)); \
    } \
    friend class CLASS ## Private;

#define MB_DECLARE_PUBLIC(CLASS) \
    inline CLASS * _pub_func() { \
        return static_cast<CLASS *>(_pub_ptr); \
    } \
    inline const CLASS* _pub_func() const { \
        return static_cast<const CLASS *>(_pub_ptr); \
    } \
    friend class CLASS;

#define MB_PRIVATE(CLASS) CLASS ## Private * const priv = _priv_func()
#define MB_PUBLIC(CLASS) CLASS * const pub = _pub_func()


// Constructor and assignment macros
#define MB_DISABLE_COPY_CONSTRUCTOR(CLASS) \
    CLASS(CLASS const &) = delete;
#define MB_DISABLE_COPY_ASSIGNMENT(CLASS) \
    CLASS & operator=(CLASS const &) = delete;
#define MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(CLASS) \
    MB_DISABLE_COPY_CONSTRUCTOR(CLASS) \
    MB_DISABLE_COPY_ASSIGNMENT(CLASS)

#define MB_DISABLE_MOVE_CONSTRUCTOR(CLASS) \
    CLASS(CLASS &&) = delete;
#define MB_DISABLE_MOVE_ASSIGNMENT(CLASS) \
    CLASS & operator=(CLASS &&) = delete;
#define MB_DISABLE_MOVE_CONSTRUCT_AND_ASSIGN(CLASS) \
    MB_DISABLE_MOVE_CONSTRUCTOR(CLASS) \
    MB_DISABLE_MOVE_ASSIGNMENT(CLASS)

#define MB_DISABLE_DEFAULT_CONSTRUCTOR(CLASS) \
    CLASS() = delete;

#define MB_DEFAULT_COPY_CONSTRUCTOR(CLASS) \
    CLASS(CLASS const &) = default;
#define MB_DEFAULT_COPY_ASSIGNMENT(CLASS) \
    CLASS & operator=(CLASS const &) = default;
#define MB_DEFAULT_COPY_CONSTRUCT_AND_ASSIGN(CLASS) \
    MB_DEFAULT_COPY_CONSTRUCTOR(CLASS) \
    MB_DEFAULT_COPY_ASSIGNMENT(CLASS)

#define MB_DEFAULT_MOVE_CONSTRUCTOR(CLASS) \
    CLASS(CLASS &&) = default;
#define MB_DEFAULT_MOVE_ASSIGNMENT(CLASS) \
    CLASS & operator=(CLASS &&) = default;
#define MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(CLASS) \
    MB_DEFAULT_MOVE_CONSTRUCTOR(CLASS) \
    MB_DEFAULT_MOVE_ASSIGNMENT(CLASS)

#endif
