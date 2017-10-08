/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#pragma once

#include "mbcommon/error/error_info.h"

// Custom error class with a default base class and some random 'info' attached.
class CustomError : public mb::ErrorInfo<CustomError>
{
public:
    static char ID;

    CustomError(int info);

    int get_info() const;

    std::string message() const override;

protected:
    // This error is subclassed below, but we can't use inheriting constructors
    // yet, so we can't propagate the constructors through ErrorInfo. Instead
    // we have to have a default constructor and have the subclass initialize
    // all fields.
    CustomError() : _info(0)
    {
    }

    int _info;
};

// Custom error class with a custom base class and some additional random
// 'info'.
class CustomSubError : public mb::ErrorInfo<CustomSubError, CustomError>
{
public:
    static char ID;

    CustomSubError(int info, int extra_info);

    int get_extra_info() const;

    std::string message() const override;

protected:
    int _extra_info;
};
