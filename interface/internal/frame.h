/**
 * @file internal/frame.h
 * @author github.com/luncliff (luncliff@gmail.com)
 * @brief Header to adjust the difference of coroutine frame between compilers
 * 
 * @copyright CC BY 4.0
 */
#pragma once
#ifndef _COROUTINE_
#define _COROUTINE_
#define _EXPERIMENTAL_RESUMABLE_

#if _STL_COMPILER_PREPROCESSOR
#include <memory>
#include <new>
#if _HAS_EXCEPTIONS
#include <exception>
#endif // _HAS_EXCEPTIONS
#endif // _STL_COMPILER_PREPROCESSOR

#include <cstddef>
#include <cstdint>
#include <type_traits>

struct _Portable_coro_prefix;

bool _Portable_coro_done(_Portable_coro_prefix* _Handle);
void _Portable_coro_resume(_Portable_coro_prefix* _Handle);
void _Portable_coro_destroy(_Portable_coro_prefix* _Handle);

_Portable_coro_prefix* _Portable_coro_from_promise(void* _PromAddr,
                                                   ptrdiff_t _PromSize);
void* _Portable_coro_get_promise(_Portable_coro_prefix* _Handle,
                                 ptrdiff_t _PromSize);

namespace std {

// 17.12.3, coroutine handle
template <typename _PromiseT = void>
struct coroutine_handle;

// STRUCT TEMPLATE coroutine_handle
template <>
struct coroutine_handle<void> {
    // 17.12.3.1, construct
    coroutine_handle() noexcept = default;

    // 17.12.3.1, reset
    coroutine_handle(std::nullptr_t) noexcept : _Ptr{nullptr} {
    }
    coroutine_handle& operator=(nullptr_t) noexcept {
        _Ptr = nullptr;
        return *this;
    }

    // 17.12.3.3, observers
    explicit operator bool() const noexcept {
        return _Ptr != nullptr;
    }
    bool done() const noexcept {
        return _Portable_coro_done(_Ptr);
    }

    // 17.12.3.4, resumption
    void resume() noexcept(false) {
        return _Portable_coro_resume(_Ptr);
    }
    void operator()() noexcept(false) {
        return _Portable_coro_resume(_Ptr);
    }
    void destroy() noexcept {
        return _Portable_coro_destroy(_Ptr);
    }

    // 17.12.3.2, export
    void* address() const noexcept {
        return _Ptr;
    }
    // 17.12.3.2, import
    static coroutine_handle from_address(void* _Addr) noexcept {
        coroutine_handle _Result{};
        _Result._Ptr = reinterpret_cast<_Portable_coro_prefix*>(_Addr);
        return _Result;
    }

  private:
    _Portable_coro_prefix* _Ptr = nullptr;
};

template <typename _PromiseT>
struct coroutine_handle : public coroutine_handle<void> {
    // 17.12.3.1, construct
    using coroutine_handle<void>::coroutine_handle;
    // 17.12.3.1, reset
    coroutine_handle& operator=(nullptr_t) noexcept {
        this->_Ptr = nullptr;
        return *this;
    }

    // 17.12.3.5, promise access
    _PromiseT& promise() const noexcept {
        void* _Addr = _Portable_coro_get_promise(this->_Ptr, sizeof(_PromiseT));
        _PromiseT* _Prom = reinterpret_cast<_PromiseT*>(_Addr);
        return *_Prom;
    }

    // 17.12.3.2, import
    static coroutine_handle from_address(void* _Addr) noexcept {
        coroutine_handle _Result{};
        _Result._Ptr = reinterpret_cast<_Portable_coro_prefix*>(_Addr);
        return _Result;
    }

    // 17.12.3.1, construct
    static coroutine_handle from_promise(_PromiseT& _Prom) noexcept {
        auto* _Addr = _Portable_coro_from_promise(&_Prom, sizeof(_PromiseT));
        return coroutine_handle::from_address(_Addr);
    }
};

// 17.12.3.6, comparison operators
// C3615: cannot result in a constant expression

inline /*constexpr*/ bool
operator==(const coroutine_handle<void> _Left,
           const coroutine_handle<void> _Right) noexcept {
    return _Left.address() == _Right.address();
}
inline /*constexpr*/ bool
operator!=(const coroutine_handle<void> _Left,
           const coroutine_handle<void> _Right) noexcept {
    return !(_Left == _Right);
}
inline /*constexpr*/ bool
operator<(const coroutine_handle<void> _Left,
          const coroutine_handle<void> _Right) noexcept {
    return _Left.address() < _Right.address();
}
inline /*constexpr*/ bool
operator>(const coroutine_handle<void> _Left,
          const coroutine_handle<void> _Right) noexcept {
    return _Right < _Left;
}
inline /*constexpr*/ bool
operator<=(const coroutine_handle<void> _Left,
           const coroutine_handle<void> _Right) noexcept {
    return !(_Left > _Right);
}
inline /*constexpr*/ bool
operator>=(const coroutine_handle<void> _Left,
           const coroutine_handle<void> _Right) noexcept {
    return !(_Left < _Right);
}

template <class T>
struct hash;

// 17.12.3.7, hash support
template <class _PromiseT>
struct hash<coroutine_handle<_PromiseT>> {
    // deprecated in C++17
    using argument_type = coroutine_handle<_PromiseT>;
    // deprecated in C++17
    using result_type = size_t;

    _NODISCARD result_type operator()(argument_type const& _Handle) const
        noexcept {
        return hash<void*>()(_Handle.address());
    }
};

// 17.12.4, no-op coroutines
struct noop_coroutine_promise {};

template <>
struct coroutine_handle<noop_coroutine_promise>
    : public coroutine_handle<void> {

    // 17.12.4.2.1, observers
    constexpr explicit operator bool() const noexcept {
        return true;
    }
    constexpr bool done() const noexcept {
        return false;
    }

    // 17.12.4.2.2, resumption
    constexpr void operator()() const noexcept {
        return;
    }
    constexpr void resume() const noexcept {
        return;
    }
    constexpr void destroy() const noexcept {
        return;
    }

    // 17.12.4.2.3, promise access
    noop_coroutine_promise& promise() const noexcept {
        static noop_coroutine_promise _Prom{};
        return _Prom;
    }

    // 17.12.4.2.4, address
    // A noop_coroutine_handle's ptr is always a non-null pointer value
    coroutine_handle() noexcept
        : coroutine_handle<void>{from_address(&this->promise())} {
    }

    // C3615: cannot result in a constant expression
    // constexpr void* address() const noexcept;
};

// STRUCT noop_coroutine_handle
using noop_coroutine_handle = coroutine_handle<noop_coroutine_promise>;

noop_coroutine_handle noop_coroutine() noexcept;

// 17.12.5, trivial awaitables

// STRUCT suspend_never
class suspend_never {
  public:
    constexpr bool await_ready() const noexcept {
        return true;
    }
    constexpr void await_resume() const noexcept {
    }
    constexpr void await_suspend(coroutine_handle<void>) const noexcept {
    }
};

// STRUCT suspend_always
class suspend_always {
  public:
    constexpr bool await_ready() const noexcept {
        return false;
    }
    constexpr void await_resume() const noexcept {
    }
    constexpr void await_suspend(coroutine_handle<void>) const noexcept {
    }
};

namespace experimental {

template <class _Ret, class = void>
struct _Coroutine_traits_sfinae {};

template <class _Ret>
struct _Coroutine_traits_sfinae<_Ret, void_t<typename _Ret::promise_type>> {
    using promise_type = typename _Ret::promise_type;
};

template <typename _Ret, typename... _Ts>
struct coroutine_traits : _Coroutine_traits_sfinae<_Ret> {};

// _Resumable_helper_traits class isolates front-end from public surface naming changes

template <typename _Ret, typename... _Ts>
struct _Resumable_helper_traits {
    using _Traits = coroutine_traits<_Ret, _Ts...>;
    using _PromiseT = typename _Traits::promise_type;
    using _Handle_type = coroutine_handle<_PromiseT>;

    static _PromiseT* _Promise_from_frame(void* _Addr) noexcept {
        auto& prom = _Handle_type::from_address(_Addr).promise();
        return &prom;
    }

    static _Handle_type _Handle_from_frame(void* _Addr) noexcept {
        return _Handle_type::from_promise(*_Promise_from_frame(_Addr));
    }

    static void _Set_exception(void* _Addr) {
        _Promise_from_frame(_Addr)->set_exception(_STD current_exception());
    }

    static void _ConstructPromise(void* _Addr, void* _Resume_addr,
                                  int _HeapElision) {
        *reinterpret_cast<void**>(_Addr) = _Resume_addr;
        *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(_Addr) +
                                     sizeof(void*)) =
            2u + (_HeapElision ? 0u : 0x10000u);
        auto _Prom = _Promise_from_frame(_Addr);
        ::new (static_cast<void*>(_Prom)) _PromiseT();
    }

    static void _DestructPromise(void* _Addr) {
        _Promise_from_frame(_Addr)->~_PromiseT();
    }
};

} // namespace experimental

// 17.12.2, coroutine traits

// STRUCT TEMPLATE coroutine_traits
template <typename R, typename... _Ts>
using coroutine_traits = experimental::coroutine_traits<R, _Ts...>;

} // namespace std

#endif // _COROUTINE_