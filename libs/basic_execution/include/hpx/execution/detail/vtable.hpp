//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXECUTION_DETAIL_VTABLE_HPP
#define HPX_EXECUTION_DETAIL_VTABLE_HPP

#include <hpx/config.hpp>
#include <hpx/datastructures/detail/pack.hpp>
#include <hpx/tag_invoke.hpp>
#include <type_traits>

namespace hpx { namespace execution { namespace detail {

    template <typename CPO>
    struct vtable_entry;

    template <typename R, typename CPO, typename... Ts, bool NoExcept>
    struct vtable_entry<R(CPO, Ts...) noexcept(NoExcept)>
    {
        using function_type = R (*)(CPO, void*, Ts...) noexcept(NoExcept);

        template <typename... Us>
        HPX_FORCEINLINE R tag_invoke(CPO cpo, void* ptr, Us&&... us) const
            noexcept(NoExcept)
        {
            return func_(cpo, ptr, std::forward<Us>(us)...);
        }

        template <typename T, bool>
        struct invoke;

        template <typename T>
        struct invoke<T, true>
        {
            static HPX_FORCEINLINE R call(
                CPO cpo, void* ptr, Ts... ts) noexcept(NoExcept)
            {
                return std::move(cpo)(
                    std::move(*static_cast<T*>(ptr)), std::forward<Ts>(ts)...);
            }
        };

        template <typename T>
        struct invoke<T, false>
        {
            static HPX_FORCEINLINE R call(
                CPO cpo, void* ptr, Ts... ts) noexcept(NoExcept)
            {
                return std::move(cpo)(
                    *static_cast<T*>(ptr), std::forward<Ts>(ts)...);
            }
        };

        template <typename T>
        static constexpr function_type create_func() noexcept
        {
            return &invoke<T, is_tag_invocable<CPO, T&&, Ts...>::value>::call;
        }

    private:
        template <typename... CPOs>
        friend struct vtable;

        constexpr vtable_entry(function_type func) noexcept
          : func_(func)
        {
        }

        function_type func_;
    };

    template <typename... CPOs>
    struct vtable final : vtable_entry<CPOs>...
    {
        template <typename T>
        static vtable const* create() noexcept
        {
            static constexpr vtable vtable_(static_cast<T*>(nullptr));
            return std::addressof(vtable_);
        }

        using vtable_entry<CPOs>::tag_invoke...;

    private:
        template <typename T>
        constexpr vtable(T*) noexcept
          : vtable_entry<CPOs>(vtable_entry<CPOs>::template create_func<T>())...
        {
        }
    };

    template <typename CPO, typename... CPOs>
    struct extract_cpo;

    template <typename CPO, typename R, typename... Ts, typename... CPOs>
    struct extract_cpo<CPO, R(CPO, Ts...), CPOs...>
    {
        using type = R;
    };

    template <typename CPO, typename R, typename Tag, typename... Ts,
        typename... CPOs>
    struct extract_cpo<CPO, R(Tag, Ts...), CPOs...> : extract_cpo<CPO, CPOs...>
    {
    };

    template <typename CPO>
    struct extract_cpo<CPO>
    {
    };

    struct destruct_t
    {
        template <typename T>
        constexpr auto operator()(T t) const
            noexcept(is_tag_invocable_nothrow<destruct_t, T>::value)
                -> hpx::tag_invoke_result_t<destruct_t, T>
        {
            return hpx::tag_invoke(*this, std::forward<T>(t));
        }

        template <typename T>
        constexpr void operator()(T& t) const
        {
            t.~T();
            delete std::addressof(t);
        }
    };

    inline constexpr destruct_t destruct = {};

    template <typename... CPOs>
    struct vtable_base
    {
        using vtable_type = vtable<CPOs..., void(detail::destruct_t)>;

        template <typename Object>
        explicit vtable_base(Object* object) noexcept
          : vtable_(vtable_type::template create<Object>())
          , object_(object)
        {
        }

        vtable_base(vtable_base const&) = delete;
        vtable_base& operator=(vtable_base const&) = delete;

        vtable_base(vtable_base&& other) noexcept
          : vtable_(other.vtable_)
          , object_(other.object_)
        {
            other.object_ = nullptr;
        }

        vtable_base& operator=(vtable_base&& other)
        {
            if (object_)
                this->vtable_->tag_invoke(detail::destruct, object_);

            vtable_ = other.vtable_;
            object_ = other.object_;
            other.object_ = nullptr;

            return *this;
        }

        ~vtable_base()
        {
            if (object_)
            {
                this->vtable_->tag_invoke(detail::destruct, object_);
                object_ = nullptr;
            }
        }

        template <typename CPO, typename... Ts,
            typename R = typename extract_cpo<CPO, CPOs...>::type>
        HPX_FORCEINLINE R tag_invoke(CPO cpo, Ts&&... ts)
        {
            return vtable_->tag_invoke(cpo, object_, std::forward<Ts>(ts)...);
        }

        vtable_type const* vtable_;
        void* object_;
    };

}}}    // namespace hpx::execution::detail

#endif
