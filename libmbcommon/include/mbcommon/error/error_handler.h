/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#pragma once

#include <type_traits>

#include <cassert>

#include "mbcommon/common.h"
#include "mbcommon/error/error.h"
#include "mbcommon/error/error_list.h"

namespace mb
{

/*!
 * Helper for testing applicability of, and applying, handlers for ErrorInfo
 * types.
 */
template <typename HandlerT>
class MB_EXPORT ErrorHandlerTraits
    : public ErrorHandlerTraits<decltype(
            &std::remove_reference_t<HandlerT>::operator())>
{
};

/*!
 * Specialization functions of the form 'Error (const ErrT &)'.
 */
template <typename ErrT>
class MB_EXPORT ErrorHandlerTraits<Error (&)(ErrT &)>
{
public:
    static bool applies_to(const ErrorInfoBase &e)
    {
        return e.template is_a<ErrT>();
    }

    template <typename HandlerT>
    static Error apply(HandlerT &&h, std::unique_ptr<ErrorInfoBase> e)
    {
        assert(applies_to(*e) && "Applying incorrect handler");
        return h(static_cast<ErrT &>(*e));
    }
};

/*!
 * Specialization functions of the form 'void (const ErrT &)'.
 */
template <typename ErrT>
class MB_EXPORT ErrorHandlerTraits<void (&)(ErrT &)>
{
public:
    static bool applies_to(const ErrorInfoBase &e)
    {
        return e.template is_a<ErrT>();
    }

    template <typename HandlerT>
    static Error apply(HandlerT &&h, std::unique_ptr<ErrorInfoBase> e)
    {
        assert(applies_to(*e) && "Applying incorrect handler");
        h(static_cast<ErrT &>(*e));
        return Error::success();
    }
};

/*!
 * Specialization for functions of the form 'Error (std::unique_ptr<ErrT>)'.
 */
template <typename ErrT>
class MB_EXPORT ErrorHandlerTraits<Error (&)(std::unique_ptr<ErrT>)>
{
public:
    static bool applies_to(const ErrorInfoBase &e)
    {
        return e.template is_a<ErrT>();
    }

    template <typename HandlerT>
    static Error apply(HandlerT &&h, std::unique_ptr<ErrorInfoBase> e)
    {
        assert(applies_to(*e) && "Applying incorrect handler");
        std::unique_ptr<ErrT> sub_e(static_cast<ErrT *>(e.release()));
        return h(std::move(sub_e));
    }
};

/*!
 * Specialization for functions of the form 'void (std::unique_ptr<ErrT>)'.
 */
template <typename ErrT>
class MB_EXPORT ErrorHandlerTraits<void (&)(std::unique_ptr<ErrT>)>
{
public:
    static bool applies_to(const ErrorInfoBase &e)
    {
        return e.template is_a<ErrT>();
    }

    template <typename HandlerT>
    static Error apply(HandlerT &&h, std::unique_ptr<ErrorInfoBase> e)
    {
        assert(applies_to(*e) && "Applying incorrect handler");
        std::unique_ptr<ErrT> sub_e(static_cast<ErrT *>(e.release()));
        h(std::move(sub_e));
        return Error::success();
    }
};

/*!
 * Specialization for member functions of the form 'RetT (ErrT &)'.
 */
template <typename C, typename RetT, typename ErrT>
class MB_EXPORT ErrorHandlerTraits<RetT (C::*)(ErrT &)>
    : public ErrorHandlerTraits<RetT (&)(ErrT &)>
{
};

/*!
 * Specialization for member functions of the form 'RetT (ErrT &) const'.
 */
template <typename C, typename RetT, typename ErrT>
class MB_EXPORT ErrorHandlerTraits<RetT (C::*)(ErrT &) const>
    : public ErrorHandlerTraits<RetT (&)(ErrT &)>
{
};

/*!
 * Specialization for member functions of the form 'RetT (const ErrT &)'.
 */
template <typename C, typename RetT, typename ErrT>
class MB_EXPORT ErrorHandlerTraits<RetT (C::*)(const ErrT &)>
    : public ErrorHandlerTraits<RetT (&)(ErrT &)>
{
};

/*!
 * Specialization for member functions of the form 'RetT (const ErrT &) const'.
 */
template <typename C, typename RetT, typename ErrT>
class MB_EXPORT ErrorHandlerTraits<RetT (C::*)(const ErrT &) const>
    : public ErrorHandlerTraits<RetT (&)(ErrT &)>
{
};

/*!
 * Specialization for member functions of the form
 * 'RetT (std::unique_ptr<ErrT>)'.
 */
template <typename C, typename RetT, typename ErrT>
class MB_EXPORT ErrorHandlerTraits<RetT (C::*)(std::unique_ptr<ErrT>)>
    : public ErrorHandlerTraits<RetT (&)(std::unique_ptr<ErrT>)>
{
};

/*!
 * Specialization for member functions of the form
 * 'RetT (std::unique_ptr<ErrT>) const'.
 */
template <typename C, typename RetT, typename ErrT>
class MB_EXPORT ErrorHandlerTraits<RetT (C::*)(std::unique_ptr<ErrT>) const>
    : public ErrorHandlerTraits<RetT (&)(std::unique_ptr<ErrT>)>
{
};

MB_EXPORT inline Error handle_error_impl(std::unique_ptr<ErrorInfoBase> payload)
{
    return {std::move(payload)};
}

template <typename HandlerT, typename... HandlerTs>
MB_EXPORT Error handle_error_impl(std::unique_ptr<ErrorInfoBase> payload,
                                  HandlerT &&handler, HandlerTs &&... handlers)
{
    if (ErrorHandlerTraits<HandlerT>::applies_to(*payload)) {
        return ErrorHandlerTraits<HandlerT>::apply(
                std::forward<HandlerT>(handler), std::move(payload));
    }
    return handle_error_impl(std::move(payload),
                             std::forward<HandlerTs>(handlers)...);
}

/*!
 * Pass the ErrorInfo(s) contained in E to their respective handlers. Any
 * unhandled errors (or Errors returned by handlers) are re-concatenated and
 * returned.
 * Because this function returns an error, its result must also be checked
 * or returned.
 */
template <typename... HandlerTs>
MB_EXPORT Error handle_errors(Error e, HandlerTs &&... hs)
{
    if (!e) {
        return Error::success();
    }

    auto payload = e.take_payload();

    if (payload->is_a<ErrorList>()) {
        ErrorList &list = static_cast<ErrorList &>(*payload);
        Error r;

        for (auto &p : list._payloads) {
            r = ErrorList::join(std::move(r), handle_error_impl(
                    std::move(p), std::forward<HandlerTs>(hs)...));
        }

        return r;
    }

    return handle_error_impl(std::move(payload), std::forward<HandlerTs>(hs)...);
}

}
