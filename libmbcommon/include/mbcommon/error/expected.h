/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#pragma once

#include <memory>
#include <type_traits>

#include <cassert>

#include "mbcommon/common.h"
#include "mbcommon/error/error.h"
#include "mbcommon/error/error_info_base.h"

namespace mb
{

/*!
 * Tagged union holding either a T or a Error.
 *
 * This class parallels ErrorOr, but replaces error_code with Error. Since
 * Error cannot be copied, this class replaces get_error() with take_error().
 * It also adds an bool error_is_a<ErrT>() method for testing the error class
 * type.
 */
template <class T>
class MB_EXPORT MB_NO_DISCARD Expected
{
    template <class OtherT>
    friend class Expected;

    static const bool is_ref = std::is_reference<T>::value;

    using wrap = std::reference_wrapper<std::remove_reference_t<T>>;

    using error_type = std::unique_ptr<ErrorInfoBase>;

public:
    using storage_type = std::conditional_t<is_ref, wrap, T>;
    using value_type = T;

private:
    using reference = std::remove_reference_t<T> &;
    using const_reference = const std::remove_reference_t<T> &;
    using pointer = std::remove_reference_t<T> *;
    using const_pointer = const std::remove_reference_t<T> *;

public:
    /*!
     * Create an Expected<T> error value from the given Error.
     */
    Expected(Error err)
        : _has_error(true)
    {
        assert(err && "Cannot create Expected<T> from Error success value.");
        new (get_error_storage()) error_type(err.take_payload());
    }

    /*!
     * Forbid to convert from Error::success() implicitly, this avoids having
     * Expected<T> foo() { return Error::success(); } which compiles otherwise
     * but triggers the assertion above.
     */
    Expected(ErrorSuccess) = delete;

    /*!
     * Create an Expected<T> success value from the given OtherT value, which
     * must be convertible to T.
     */
    template <typename OtherT>
    Expected(OtherT &&val,
             typename std::enable_if_t<std::is_convertible<OtherT, T>::value>
                     * = nullptr)
        : _has_error(false)
    {
        new (get_storage()) storage_type(std::forward<OtherT>(val));
    }

    /*!
     * Move construct an Expected<T> value.
     */
    Expected(Expected &&other)
    {
        move_construct(std::move(other));
    }

    /*!
     * Move construct an Expected<T> value from an Expected<OtherT>, where OtherT
     * must be convertible to T.
     */
    template <class OtherT>
    Expected(Expected<OtherT> &&other,
             typename std::enable_if_t<std::is_convertible<OtherT, T>::value>
                     * = nullptr)
    {
        move_construct(std::move(other));
    }

    /*!
     * Move construct an Expected<T> value from an Expected<OtherT>, where OtherT
     * isn't convertible to T.
     */
    template <class OtherT>
    explicit Expected(
            Expected<OtherT> &&other,
            typename std::enable_if_t<!std::is_convertible<OtherT, T>::value>
                    * = nullptr)
    {
        move_construct(std::move(other));
    }

    /*!
     * Move-assign from another Expected<T>.
     */
    Expected & operator=(Expected &&other)
    {
        move_assign(std::move(other));
        return *this;
    }

    /*!
     * Destroy an Expected<T>.
     */
    ~Expected()
    {
        if (!_has_error) {
            get_storage()->~storage_type();
        } else {
            get_error_storage()->~error_type();
        }
    }

    /*!
     * \brief Return false if there is an error.
     */
    explicit operator bool()
    {
        return !_has_error;
    }

    /*!
     * \brief Returns a reference to the stored T value.
     */
    reference get()
    {
        return *get_storage();
    }

    /*!
     * \brief Returns a const reference to the stored T value.
     */
    const_reference get() const
    {
        return const_cast<Expected<T> *>(this)->get();
    }

    /*!
     * \brief Check that this Expected<T> is an error of type ErrT.
     */
    template <typename ErrT> bool error_is_a() const
    {
        return _has_error && (*get_error_storage())->template is_a<ErrT>();
    }

    /*!
     * \brief Take ownership of the stored error.
     * After calling this the Expected<T> is in an indeterminate state that can
     * only be safely destructed. No further calls (beside the destructor) should
     * be made on the Expected<T> value.
     */
    Error take_error()
    {
        return _has_error
                ? Error(std::move(*get_error_storage()))
                : Error::success();
    }

    /*!
     * \brief Returns a pointer to the stored T value.
     */
    pointer operator->()
    {
        return to_pointer(get_storage());
    }

    /*!
     * \brief Returns a const pointer to the stored T value.
     */
    const_pointer operator->() const
    {
        return to_pointer(get_storage());
    }

    /*!
     * \brief Returns a reference to the stored T value.
     */
    reference operator*()
    {
        return *get_storage();
    }

    /*!
     * \brief Returns a const reference to the stored T value.
     */
    const_reference operator*() const
    {
        return *get_storage();
    }

private:
    template <class T1>
    static bool compare_this_if_same_type(const T1 &a, const T1 &b)
    {
        return &a == &b;
    }

    template <class T1, class T2>
    static bool compare_this_if_same_type(const T1 &a, const T2 &b)
    {
        (void) a;
        (void) b;
        return false;
    }

    template <class OtherT>
    void move_construct(Expected<OtherT> &&other)
    {
        _has_error = other._has_error;

        if (!_has_error) {
            new (get_storage()) storage_type(std::move(*other.get_storage()));
        } else {
            new (get_error_storage()) error_type(std::move(*other.get_error_storage()));
        }
    }

    template <class OtherT>
    void move_assign(Expected<OtherT> &&other)
    {
        if (compare_this_if_same_type(*this, other)) {
            return;
        }

        this->~Expected();
        new (this) Expected(std::move(other));
    }

    pointer to_pointer(pointer val)
    {
        return val;
    }

    const_pointer to_pointer(const_pointer val) const
    {
        return val;
    }

    pointer to_pointer(wrap *val)
    {
        return &val->get();
    }

    const_pointer to_pointer(const wrap *val) const
    {
        return &val->get();
    }

    storage_type * get_storage()
    {
        assert(!_has_error && "Cannot get value when an error exists!");
        return reinterpret_cast<storage_type *>(&_storage);
    }

    const storage_type * get_storage() const
    {
        assert(!_has_error && "Cannot get value when an error exists!");
        return reinterpret_cast<const storage_type *>(&_storage);
    }

    error_type * get_error_storage()
    {
        assert(_has_error && "Cannot get error when a value exists!");
        return reinterpret_cast<error_type *>(&_storage);
    }

    const error_type * get_error_storage() const
    {
        assert(_has_error && "Cannot get error when a value exists!");
        return reinterpret_cast<const error_type *>(&_storage);
    }

    std::aligned_union_t<0, storage_type, error_type> _storage;
    bool _has_error : 1;
};

/*!
 * Specialization of Expected for void
 */
template <>
class MB_EXPORT MB_NO_DISCARD Expected<void>
{
    template <class OtherT>
    friend class Expected;

    using error_type = std::unique_ptr<ErrorInfoBase>;

public:
    /*!
     * Create an Expected<void> with no error.
     */
    Expected() = default;

    /*!
     * Create an Expected<void> error value from the given Error.
     */
    Expected(Error err)
    {
        assert(err && "Cannot create Expected<void> from Error success value.");
        _error = err.take_payload();
    }

    /*!
     * Forbid to convert from Error::success() implicitly, this avoids having
     * Expected<void> foo() { return Error::success(); } which compiles otherwise
     * but triggers the assertion above.
     */
    Expected(ErrorSuccess) = delete;

    /*!
     * Move construct an Expected<void> value.
     */
    Expected(Expected &&other)
        : _error(std::move(other._error))
    {
    }

    /*!
     * Move-assign from another Expected<void>.
     */
    Expected & operator=(Expected &&other)
    {
        if (this != &other) {
            this->~Expected();
            new (this) Expected(std::move(other));
        }
        return *this;
    }

    /*!
     * Destroy an Expected<void>.
     */
    ~Expected() = default;

    /*!
     * \brief Return false if there is an error.
     */
    explicit operator bool()
    {
        return !_error;
    }

    /*!
     * \brief Check that this Expected<void> is an error of type ErrT.
     */
    template <typename ErrT> bool error_is_a() const
    {
        return _error && _error->template is_a<ErrT>();
    }

    /*!
     * \brief Take ownership of the stored error.
     * After calling this the Expected<void> is in an indeterminate state that can
     * only be safely destructed. No further calls (beside the destructor) should
     * be made on the Expected<void> value.
     */
    Error take_error()
    {
        return _error
                ? Error(std::move(_error))
                : Error::success();
    }

private:
    error_type _error;
};

}
