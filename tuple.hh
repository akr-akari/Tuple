#ifndef Z_AKR_TUPLE_HH
#define Z_AKR_TUPLE_HH

#include <stdexcept>
#include <type_traits>
#include <utility>

namespace akr
{
    template<class... T>
    struct Tuple;

    template<class T>
    struct TupleHelper final
    {
        private:
        template<template<class...> class U, class... V>
        struct IsTupleHelper
        {
        };

        template<template<class...> class U, class... V>
        struct IsTupleHelper<U, U<V...>> final
        {
            public:
            inline static constexpr std::size_t Count = sizeof...(V);
        };

        public:
        struct Extract final
        {
            private:
            template<template<class...> class U, class V, class... W>
            struct ExtractHelper;

            template<template<class...> class U, class V, class... W>
            struct ExtractHelper<U, U<V, W...>> final
            {
                public:
                using FirstType = V;

                using TupleBase = U<W...>;
            };

            public:
            using FirstType = typename ExtractHelper<Tuple, std::decay_t<T>>::FirstType;

            using TupleBase = typename ExtractHelper<Tuple, std::decay_t<T>>::TupleBase;
        };

        public:
        inline static constexpr bool IsTuple = std::is_final_v<IsTupleHelper<Tuple, std::decay_t<T>>>;

        public:
        static consteval auto CountOf() noexcept -> std::size_t
        {
            return IsTupleHelper<Tuple, std::decay_t<T>>::Count;
        }
    };

    template<>
    struct Tuple<>
    {
        public:
        static constexpr auto Count = 0;

        public:
        template<class T, class U>
        requires(TupleHelper<T>::IsTuple)
        friend constexpr auto operator&(T&& lhs, U&& rhs)
            noexcept(std::is_nothrow_constructible_v<T, const T&>
                     && (std::is_nothrow_constructible_v<std::decay_t<U>, U&&>))
        {
            return std::forward<T>(lhs) + Tuple<>::Create(std::forward<U>(rhs));
        }

        template<class T, class U>
        requires(TupleHelper<T>::IsTuple && TupleHelper<U>::IsTuple)
        friend constexpr auto operator+(T&& lhs, U&& rhs)
            noexcept(std::is_nothrow_constructible_v<T, const T&>
                     && (std::is_nothrow_constructible_v<std::decay_t<U>, U&&>))
        {
            return Tuple<>::combine<0, TupleHelper<T>::CountOf(), 0, TupleHelper<U>::CountOf()>(
                std::forward<T>(lhs), std::forward<U>(rhs));
        }

        public:
        template<class... T>
        static constexpr auto Concat(T&&... values)
            noexcept((... && std::is_nothrow_constructible_v<std::decay_t<T>, T&&>))
        {
            return (... + toTuple(std::forward<T>(values)));
        }

        template<class... T>
        static constexpr auto Create(T&&... values)
            noexcept((... && std::is_nothrow_constructible_v<std::decay_t<T>, T&&>))
        {
            return Tuple<std::decay_t<T>...>(std::forward<T>(values)...);
        }

        protected:
        template<class F>
        constexpr void ForEach(const F&)       noexcept
        {
        }

        template<class F>
        constexpr void ForEach(const F&) const noexcept
        {
        }

        private:
        template<std::size_t B1, std::size_t E1, std::size_t B2, std::size_t E2, class T1, class T2, class... V>
        static constexpr auto combine(T1&& tuple1 [[maybe_unused]], T2&& tuple2 [[maybe_unused]], V&&... values)
            noexcept(std::is_nothrow_constructible_v<std::decay_t<T1>, T1&&>
                     && std::is_nothrow_constructible_v<std::decay_t<T2>, T2&&>)
        {
            if constexpr (B1 != E1)
            {
                return Tuple<>::combine<B1 + 1, E1, B2, E2>(
                    std::forward<T1>(tuple1), std::forward<T2>(tuple2), std::forward<V>(values)...,
                    tuple1.template IndexOf<B1>());
            }
            else if constexpr (B2 != E2)
            {
                return Tuple<>::combine<B1, E1, B2 + 1, E2>(
                    std::forward<T1>(tuple1), std::forward<T2>(tuple2), std::forward<V>(values)...,
                    tuple2.template IndexOf<B2>());
            }
            else
            {
                return Tuple<>::Create(std::forward<V>(values)...);
            }
        }

        template<class T>
        static constexpr auto toTuple(T&& value)
            noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T&&>)
        {
            if constexpr (!TupleHelper<T>::IsTuple)
            {
                return Tuple<>::Create(std::forward<T>(value));
            }
            else
            {
                return std::forward<T>(value);
            }
        };
    };

    template<class T, class... U>
    struct Tuple<T, U...>: Tuple<U...>
    {
        template<class... V>
        friend struct Tuple;

        private:
        template<class V>
        using TupleBase = std::conditional_t<std::is_rvalue_reference_v<V&&>,
            typename TupleHelper<V>::Extract::TupleBase&&, const typename TupleHelper<V>::Extract::TupleBase&>;

        template<class V>
        using FirstType = std::conditional_t<std::is_rvalue_reference_v<V&&>,
            typename TupleHelper<V>::Extract::FirstType&&, const typename TupleHelper<V>::Extract::FirstType&>;

        protected:
        T value {};

        public:
        static constexpr auto Count = sizeof...(U) + 1;

        public:
        constexpr Tuple() = default;

        template<class V>
        requires(!TupleHelper<V>::IsTuple)
        explicit constexpr Tuple(V&& value_)
            noexcept(std::is_nothrow_constructible_v<T, V&&>):
            value { std::forward<V>(value_) }
        {
        }

        template<class V>
        requires( TupleHelper<V>::IsTuple && Count == TupleHelper<V>::CountOf())
        constexpr Tuple(V&& value_)
            noexcept(std::is_nothrow_constructible_v<T, FirstType<V&&>>
                     && std::is_nothrow_constructible_v<Tuple<U...>, TupleBase<V&&>>):
            Tuple<U...>(static_cast<TupleBase<V&&>>(value_)),
            value { static_cast<FirstType<V&&>>(value_.value) }
        {
        }

        template<class V, class... W>
        requires(sizeof...(W) != 0)
        explicit constexpr Tuple(V&& value_, W&&... values)
            noexcept(std::is_nothrow_constructible_v<T, V&&>
                     && (... && std::is_nothrow_constructible_v<U, W&&>)):
            Tuple<U...>(std::forward<W>(values)...),
            value { std::forward<V>(value_) }
        {
        }

        public:
        template<class V>
        requires( TupleHelper<V>::IsTuple && Count == TupleHelper<V>::CountOf())
        constexpr auto operator=(V&& rhs)
            noexcept(std::is_nothrow_assignable_v<T, FirstType<V&&>>
                     && std::is_nothrow_assignable_v<Tuple<U...>, TupleBase<V&&>>) -> Tuple&
        {
            auto&& lhs = *this;

            static_cast<Tuple<U...>&>(lhs) = static_cast<TupleBase<V&&>>(rhs);

            value = { static_cast<FirstType<V&&>>(rhs.value) };

            return lhs;
        }

        public:
        template<class F>
        constexpr void ForEach(const F& func)
            noexcept(noexcept(func(value)) && noexcept(Tuple<U...>::ForEach(func)))
        {
            func(value);

            if constexpr (sizeof...(U) != 0)
            {
                Tuple<U...>::ForEach(func);
            }
        }

        template<class F>
        constexpr void IndexBy(std::size_t index, const F& func)
        {
            [&]<size_t... I_>(std::index_sequence<I_...>) constexpr
            {
                if (index < Count)
                {
                    #ifdef __clang__
                    #pragma clang diagnostic push
                    #pragma clang diagnostic ignored "-Wunused-value"
                    #endif
                    (..., (I_ == index && (func(IndexOf<I_>()), false)));
                    #ifdef __clang__
                    #pragma clang diagnostic pop
                    #endif
                }
                else
                {
                    throw std::out_of_range("index out of range.");
                }
            }
            (std::index_sequence_for<T, U...>{});
        }

        template<std::size_t I>
        requires(I < Count)
        constexpr auto IndexOf()       noexcept ->       auto&
        {
            if constexpr (I == 0)
            {
                return value;
            }
            else
            {
                return Tuple<U...>::template IndexOf<I - 1>();
            }
        }

        public:
        template<class F>
        constexpr void ForEach(const F& func) const
            noexcept(noexcept(func(value)) && noexcept(Tuple<U...>::ForEach(func)))
        {
            func(value);

            if constexpr (sizeof...(U) != 0)
            {
                Tuple<U...>::ForEach(func);
            }
        }

        template<class F>
        constexpr void IndexBy(std::size_t index, const F& func) const
        {
            [&]<size_t... I_>(std::index_sequence<I_...>) constexpr
            {
                if (index < Count)
                {
                    #ifdef __clang__
                    #pragma clang diagnostic push
                    #pragma clang diagnostic ignored "-Wunused-value"
                    #endif
                    (..., (I_ == index && (func(IndexOf<I_>()), false)));
                    #ifdef __clang__
                    #pragma clang diagnostic pop
                    #endif
                }
                else
                {
                    throw std::out_of_range("index out of range.");
                }
            }
            (std::index_sequence_for<T, U...>{});
        }

        template<std::size_t I>
        requires(I < Count)
        constexpr auto IndexOf() const noexcept -> const auto&
        {
            if constexpr (I == 0)
            {
                return value;
            }
            else
            {
                return Tuple<U...>::template IndexOf<I - 1>();
            }
        }
    };

    template<class V>
    explicit Tuple(V&& value_)                -> Tuple<std::decay_t<V>>;

    template<class V, class... W>
    explicit Tuple(V&& value_, W&&... values) -> Tuple<std::decay_t<V>, std::decay_t<W>...>;
}

#ifdef  D_AKR_TEST
#include <cstring>

namespace akr::test
{
    AKR_TEST(Tuple,
    {
        static_assert(noexcept(Tuple<>::Create(std::declval<int>())));

        static_assert(noexcept(Tuple(std::declval<int>())));
        static_assert(noexcept(Tuple(std::declval<Tuple<int>>())));

        struct TestX
        {
        };

        TestX testx;
        Tuple<TestX> tuplex;

        struct Test1
        {
            Test1() = default;

            Test1(const TestX& ) noexcept(true);
            Test1(      TestX&&) noexcept(true);

            Test1(const Test1& ) noexcept(true);
            Test1(      Test1&&) noexcept(true);

            auto operator=(const Test1& ) noexcept(true) -> Test1&;
            auto operator=(      Test1&&) noexcept(true) -> Test1&;
        };

        Test1 test1;
        Tuple<Test1> tuple1;
        static_assert(noexcept(Tuple<Test1>(test1)));
        static_assert(noexcept(Tuple<Test1>(std::declval<Test1>())));

        static_assert(noexcept(Tuple<Test1>(tuple1)));
        static_assert(noexcept(Tuple<Test1>(std::declval<Tuple<Test1>>())));

        static_assert(noexcept(Tuple<int, Test1>(std::declval<int>(), test1)));
        static_assert(noexcept(Tuple<int, Test1>(std::declval<int>(), std::declval<Test1>())));

        static_assert(noexcept(tuple1.operator=(tuple1)));
        static_assert(noexcept(tuple1.operator=(std::declval<Tuple<Test1>>())));

        static_assert(noexcept(Tuple<Test1>(testx)));
        static_assert(noexcept(Tuple<Test1>(std::declval<TestX>())));

        static_assert(noexcept(Tuple<Test1>(tuplex)));
        static_assert(noexcept(Tuple<Test1>(std::declval<Tuple<TestX>>())));

        static_assert(noexcept(Tuple<int, Test1>(std::declval<int>(), testx)));
        static_assert(noexcept(Tuple<int, Test1>(std::declval<int>(), std::declval<TestX>())));

        static_assert(noexcept(tuple1.operator=(tuplex)));
        static_assert(noexcept(tuple1.operator=(std::declval<Tuple<TestX>>())));

        static_assert(noexcept(tuple1 & test1));
        static_assert(noexcept(tuple1 & std::declval<Test1>()));

        static_assert(noexcept(tuple1 + tuple1));
        static_assert(noexcept(tuple1 + std::declval<Tuple<Test1>>()));
        static_assert(noexcept(std::declval<Tuple<Test1>>() + std::declval<Tuple<Test1>>()));

        static_assert(noexcept(Tuple<>::Create(test1)));
        static_assert(noexcept(Tuple<>::Create(std::declval<Test1>())));

        static_assert(noexcept(Tuple<>::Create(std::declval<int>(), test1)));
        static_assert(noexcept(Tuple<>::Create(std::declval<int>(), std::declval<Test1>())));

        static_assert(noexcept(Tuple<>::Concat(tuple1, test1)));
        static_assert(noexcept(Tuple<>::Concat(tuple1, std::declval<Test1>())));
        static_assert(noexcept(Tuple<>::Concat(tuple1, std::declval<Tuple<Test1>>())));

        struct Test2
        {
            Test2() = default;

            Test2(const TestX& ) noexcept(false);
            Test2(      TestX&&) noexcept(false);

            Test2(const Test2& ) noexcept(false);
            Test2(      Test2&&) noexcept(false);

            auto operator=(const Test2& ) noexcept(false) -> Test2&;
            auto operator=(      Test2&&) noexcept(false) -> Test2&;
        };

        Test2 test2;
        Tuple<Test2> tuple2;
        static_assert(!noexcept(Tuple<Test2>(test2)));
        static_assert(!noexcept(Tuple<Test2>(std::declval<Test2>())));

        static_assert(!noexcept(Tuple<Test2>(tuple2)));
        static_assert(!noexcept(Tuple<Test2>(std::declval<Tuple<Test2>>())));

        static_assert(!noexcept(Tuple<int, Test2>(std::declval<int>(), test2)));
        static_assert(!noexcept(Tuple<int, Test2>(std::declval<int>(), std::declval<Test2>())));

        static_assert(!noexcept(tuple2.operator=(tuple2)));
        static_assert(!noexcept(tuple2.operator=(std::declval<Tuple<Test2>>())));

        static_assert(!noexcept(Tuple<Test2>(testx)));
        static_assert(!noexcept(Tuple<Test2>(std::declval<TestX>())));

        static_assert(!noexcept(Tuple<Test2>(tuplex)));
        static_assert(!noexcept(Tuple<Test2>(std::declval<Tuple<TestX>>())));

        static_assert(!noexcept(Tuple<int, Test2>(std::declval<int>(), testx)));
        static_assert(!noexcept(Tuple<int, Test2>(std::declval<int>(), std::declval<TestX>())));

        static_assert(!noexcept(tuple2.operator=(tuplex)));
        static_assert(!noexcept(tuple2.operator=(std::declval<Tuple<TestX>>())));

        static_assert(!noexcept(tuple2 & test2));
        static_assert(!noexcept(tuple2 & std::declval<Test2>()));

        static_assert(!noexcept(tuple2 + tuple2));
        static_assert(!noexcept(tuple2 + std::declval<Tuple<Test2>>()));
        static_assert(!noexcept(std::declval<Tuple<Test2>>() + std::declval<Tuple<Test2>>()));

        static_assert(!noexcept(Tuple<>::Create(test2)));
        static_assert(!noexcept(Tuple<>::Create(std::declval<Test2>())));

        static_assert(!noexcept(Tuple<>::Create(std::declval<int>(), test2)));
        static_assert(!noexcept(Tuple<>::Create(std::declval<int>(), std::declval<Test2>())));

        static_assert(!noexcept(Tuple<>::Concat(tuple2, test2)));
        static_assert(!noexcept(Tuple<>::Concat(tuple2, std::declval<Test2>())));
        static_assert(!noexcept(Tuple<>::Concat(tuple2, std::declval<Tuple<Test2>>())));

        struct Test3
        {
            Test3() = default;

            Test3(const TestX& ) noexcept(true );
            Test3(      TestX&&) noexcept(false);

            Test3(const Test3& ) noexcept(true );
            Test3(      Test3&&) noexcept(false);

            auto operator=(const Test3& ) noexcept(true ) -> Test3&;
            auto operator=(      Test3&&) noexcept(false) -> Test3&;
        };

        Test3 test3;
        Tuple<Test3> tuple3;
        static_assert( noexcept(Tuple<Test3>(test3)));
        static_assert(!noexcept(Tuple<Test3>(std::declval<Test3>())));

        static_assert( noexcept(Tuple<Test3>(tuple3)));
        static_assert(!noexcept(Tuple<Test3>(std::declval<Tuple<Test3>>())));

        static_assert( noexcept(Tuple<int, Test3>(std::declval<int>(), test3)));
        static_assert(!noexcept(Tuple<int, Test3>(std::declval<int>(), std::declval<Test3>())));

        static_assert( noexcept(tuple3.operator=(tuple3)));
        static_assert(!noexcept(tuple3.operator=(std::declval<Tuple<Test3>>())));

        static_assert( noexcept(Tuple<Test3>(testx)));
        static_assert(!noexcept(Tuple<Test3>(std::declval<TestX>())));

        static_assert( noexcept(Tuple<Test3>(tuplex)));
        static_assert(!noexcept(Tuple<Test3>(std::declval<Tuple<TestX>>())));

        static_assert( noexcept(Tuple<int, Test3>(std::declval<int>(), testx)));
        static_assert(!noexcept(Tuple<int, Test3>(std::declval<int>(), std::declval<TestX>())));

        static_assert(!noexcept(tuple3.operator=(tuplex)));
        static_assert(!noexcept(tuple3.operator=(std::declval<Tuple<TestX>>())));

        static_assert( noexcept(tuple3 & test3));
        static_assert(!noexcept(tuple3 & std::declval<Test3>()));

        static_assert( noexcept(tuple3 + tuple3));
        static_assert(!noexcept(tuple3 + std::declval<Tuple<Test3>>()));
        static_assert(!noexcept(std::declval<Tuple<Test3>>() + std::declval<Tuple<Test3>>()));

        static_assert( noexcept(Tuple<>::Create(test3)));
        static_assert(!noexcept(Tuple<>::Create(std::declval<Test3>())));

        static_assert( noexcept(Tuple<>::Create(std::declval<int>(), test3)));
        static_assert(!noexcept(Tuple<>::Create(std::declval<int>(), std::declval<Test3>())));

        static_assert( noexcept(Tuple<>::Concat(tuple3, test3)));
        static_assert(!noexcept(Tuple<>::Concat(tuple3, std::declval<Test3>())));
        static_assert(!noexcept(Tuple<>::Concat(tuple3, std::declval<Tuple<Test3>>())));

        static_assert( noexcept(std::declval<Tuple<int>>().
                                ForEach(std::declval<decltype([](auto&&) noexcept(true ) {})>())));

        static_assert(!noexcept(std::declval<Tuple<int>>().
                                ForEach(std::declval<decltype([](auto&&) noexcept(false) {})>())));

        auto t0 = Tuple();
        static_assert(std::is_same_v<Tuple<>, decltype(t0)>);
        static_assert(t0.Count == 0);

        auto t1 = Tuple(1);
        static_assert(t1.Count == 1);
        static_assert(std::is_same_v<Tuple<int>, decltype(t1)>);
        static_assert(std::is_same_v<int&, decltype(t1.IndexOf<0>())>);
        assert(t1.IndexOf<0>() == 1);

        auto t11 = t1;
        static_assert(t11.Count == 1);
        static_assert(std::is_same_v<Tuple<int>, decltype(t11)>);
        static_assert(std::is_same_v<int&, decltype(t11.IndexOf<0>())>);
        assert(t11.IndexOf<0>() == 1);

        t11.IndexOf<0>() = -1;
        assert(t11.IndexOf<0>() == -1);
        t11.IndexBy(0, [](auto&& e) { e = 1; });
        assert(t11.IndexOf<0>() == 1);
        t11.ForEach([](auto&& e) { e = 0; });
        assert(t11.IndexOf<0>() == 0);

        t11 = t1;
        static_assert(t11.Count == 1);
        static_assert(std::is_same_v<Tuple<int>, decltype(t11)>);
        static_assert(std::is_same_v<int&, decltype(t11.IndexOf<0>())>);
        assert(t11.IndexOf<0>() == 1);

        auto t111 = Tuple<long long>(1);
        static_assert(t111.Count == 1);
        static_assert(std::is_same_v<Tuple<long long>, decltype(t111)>);
        static_assert(std::is_same_v<long long&, decltype(t111.IndexOf<0>())>);
        assert(t111.IndexOf<0>() == 1);

        auto t1111 = Tuple<long long>(t1);
        static_assert(t1111.Count == 1);
        static_assert(std::is_same_v<Tuple<long long>, decltype(t1111)>);
        static_assert(std::is_same_v<long long&, decltype(t1111.IndexOf<0>())>);
        assert(t1111.IndexOf<0>() == 1);

        t1111.IndexOf<0>() = -1;
        assert(t1111.IndexOf<0>() == -1);
        t1111.IndexBy(0, [](auto&& e) { e = 1; });
        assert(t1111.IndexOf<0>() == 1);
        t1111.ForEach([](auto&& e) { e = 0; });
        assert(t1111.IndexOf<0>() == 0);

        t1111 = t1;
        static_assert(t1111.Count == 1);
        static_assert(std::is_same_v<Tuple<long long>, decltype(t1111)>);
        static_assert(std::is_same_v<long long&, decltype(t1111.IndexOf<0>())>);
        assert(t1111.IndexOf<0>() == 1);

        auto t2 = Tuple(1, 2);
        static_assert(t2.Count == 2);
        static_assert(std::is_same_v<Tuple<int, int>, decltype(t2)>);
        static_assert(std::is_same_v<int&, decltype(t2.IndexOf<0>())>);
        assert(t2.IndexOf<0>() == 1);
        static_assert(std::is_same_v<int&, decltype(t2.IndexOf<1>())>);
        assert(t2.IndexOf<1>() == 2);

        auto t22 = t2;
        static_assert(t22.Count == 2);
        static_assert(std::is_same_v<Tuple<int, int>, decltype(t22)>);
        static_assert(std::is_same_v<int&, decltype(t22.IndexOf<0>())>);
        assert(t22.IndexOf<0>() == 1);
        static_assert(std::is_same_v<int&, decltype(t22.IndexOf<1>())>);
        assert(t22.IndexOf<1>() == 2);

        t22.IndexOf<0>() = -1;
        assert(t22.IndexOf<0>() == -1);
        t22.IndexOf<1>() = -2;
        assert(t22.IndexOf<1>() == -2);
        t22.IndexBy(0, [](auto&& e) { e = 1; });
        assert(t22.IndexOf<0>() == 1);
        t22.IndexBy(1, [](auto&& e) { e = 2; });
        assert(t22.IndexOf<1>() == 2);
        t22.ForEach([](auto&& e) { e = 0; });
        assert(t22.IndexOf<0>() == 0);
        assert(t22.IndexOf<1>() == 0);

        t22 = t2;
        static_assert(t22.Count == 2);
        static_assert(std::is_same_v<Tuple<int, int>, decltype(t22)>);
        static_assert(std::is_same_v<int&, decltype(t22.IndexOf<0>())>);
        assert(t22.IndexOf<0>() == 1);
        static_assert(std::is_same_v<int&, decltype(t22.IndexOf<1>())>);
        assert(t22.IndexOf<1>() == 2);

        using T22 = decltype(Tuple(1ll, 2ll));

        auto t222 = T22(1, 2);
        static_assert(t222.Count == 2);
        static_assert(std::is_same_v<Tuple<long long, long long>, decltype(t222)>);
        static_assert(std::is_same_v<long long&, decltype(t222.IndexOf<0>())>);
        assert(t222.IndexOf<0>() == 1);
        static_assert(std::is_same_v<long long&, decltype(t222.IndexOf<1>())>);
        assert(t222.IndexOf<1>() == 2);

        auto t2222 = T22(t2);
        static_assert(t2222.Count == 2);
        static_assert(std::is_same_v<Tuple<long long, long long>, decltype(t2222)>);
        static_assert(std::is_same_v<long long&, decltype(t2222.IndexOf<0>())>);
        assert(t2222.IndexOf<0>() == 1);
        static_assert(std::is_same_v<long long&, decltype(t2222.IndexOf<1>())>);
        assert(t2222.IndexOf<1>() == 2);

        t2222.IndexOf<0>() = -1;
        assert(t2222.IndexOf<0>() == -1);
        t2222.IndexOf<1>() = -2;
        assert(t2222.IndexOf<1>() == -2);
        t2222.IndexBy(0, [](auto&& e) { e = 1; });
        assert(t2222.IndexOf<0>() == 1);
        t2222.IndexBy(1, [](auto&& e) { e = 2; });
        assert(t2222.IndexOf<1>() == 2);
        t2222.ForEach([](auto&& e) { e = 0; });
        assert(t2222.IndexOf<0>() == 0);
        assert(t2222.IndexOf<1>() == 0);

        t2222 = t2;
        static_assert(t2222.Count == 2);
        static_assert(std::is_same_v<Tuple<long long, long long>, decltype(t2222)>);
        static_assert(std::is_same_v<long long&, decltype(t2222.IndexOf<0>())>);
        assert(t2222.IndexOf<0>() == 1);
        static_assert(std::is_same_v<long long&, decltype(t2222.IndexOf<1>())>);
        assert(t2222.IndexOf<1>() == 2);

        auto t3 = Tuple(1, 2, 3);
        static_assert(t3.Count == 3);
        static_assert(std::is_same_v<Tuple<int, int, int>, decltype(t3)>);
        static_assert(std::is_same_v<int&, decltype(t3.IndexOf<0>())>);
        assert(t3.IndexOf<0>() == 1);
        static_assert(std::is_same_v<int&, decltype(t3.IndexOf<1>())>);
        assert(t3.IndexOf<1>() == 2);
        static_assert(std::is_same_v<int&, decltype(t3.IndexOf<2>())>);
        assert(t3.IndexOf<2>() == 3);

        auto t33 = t3;
        static_assert(t33.Count == 3);
        static_assert(std::is_same_v<Tuple<int, int, int>, decltype(t33)>);
        static_assert(std::is_same_v<int&, decltype(t33.IndexOf<0>())>);
        assert(t33.IndexOf<0>() == 1);
        static_assert(std::is_same_v<int&, decltype(t33.IndexOf<1>())>);
        assert(t33.IndexOf<1>() == 2);
        static_assert(std::is_same_v<int&, decltype(t33.IndexOf<2>())>);
        assert(t33.IndexOf<2>() == 3);

        t33.IndexOf<0>() = -1;
        assert(t33.IndexOf<0>() == -1);
        t33.IndexOf<1>() = -2;
        assert(t33.IndexOf<1>() == -2);
        t33.IndexOf<2>() = -3;
        assert(t33.IndexOf<2>() == -3);
        t33.IndexBy(0, [](auto&& e) { e = 1; });
        assert(t33.IndexOf<0>() == 1);
        t33.IndexBy(1, [](auto&& e) { e = 2; });
        assert(t33.IndexOf<1>() == 2);
        t33.IndexBy(2, [](auto&& e) { e = 3; });
        assert(t33.IndexOf<2>() == 3);
        t33.ForEach([](auto&& e) { e = 0; });
        assert(t33.IndexOf<0>() == 0);
        assert(t33.IndexOf<1>() == 0);
        assert(t33.IndexOf<2>() == 0);

        t33 = t3;
        static_assert(t33.Count == 3);
        static_assert(std::is_same_v<Tuple<int, int, int>, decltype(t33)>);
        static_assert(std::is_same_v<int&, decltype(t33.IndexOf<0>())>);
        assert(t33.IndexOf<0>() == 1);
        static_assert(std::is_same_v<int&, decltype(t33.IndexOf<1>())>);
        assert(t33.IndexOf<1>() == 2);
        static_assert(std::is_same_v<int&, decltype(t33.IndexOf<2>())>);
        assert(t33.IndexOf<2>() == 3);

        using T33 = decltype(Tuple(1ll, 2ll, 3ll));

        auto t333 = T33(1, 2, 3);
        static_assert(t333.Count == 3);
        static_assert(std::is_same_v<Tuple<long long, long long, long long>, decltype(t333)>);
        static_assert(std::is_same_v<long long&, decltype(t333.IndexOf<0>())>);
        assert(t333.IndexOf<0>() == 1);
        static_assert(std::is_same_v<long long&, decltype(t333.IndexOf<1>())>);
        assert(t333.IndexOf<1>() == 2);
        static_assert(std::is_same_v<long long&, decltype(t333.IndexOf<2>())>);
        assert(t333.IndexOf<2>() == 3);

        auto t3333 = T33(t3);
        static_assert(t3333.Count == 3);
        static_assert(std::is_same_v<Tuple<long long, long long, long long>, decltype(t3333)>);
        static_assert(std::is_same_v<long long&, decltype(t3333.IndexOf<0>())>);
        assert(t3333.IndexOf<0>() == 1);
        static_assert(std::is_same_v<long long&, decltype(t3333.IndexOf<1>())>);
        assert(t3333.IndexOf<1>() == 2);
        static_assert(std::is_same_v<long long&, decltype(t3333.IndexOf<2>())>);
        assert(t3333.IndexOf<2>() == 3);

        t3333.IndexOf<0>() = -1;
        assert(t3333.IndexOf<0>() == -1);
        t3333.IndexOf<1>() = -2;
        assert(t3333.IndexOf<1>() == -2);
        t3333.IndexOf<2>() = -3;
        assert(t3333.IndexOf<2>() == -3);
        t3333.IndexBy(0, [](auto&& e) { e = 1; });
        assert(t3333.IndexOf<0>() == 1);
        t3333.IndexBy(1, [](auto&& e) { e = 2; });
        assert(t3333.IndexOf<1>() == 2);
        t3333.IndexBy(2, [](auto&& e) { e = 3; });
        assert(t3333.IndexOf<2>() == 3);
        t3333.ForEach([](auto&& e) { e = 0; });
        assert(t3333.IndexOf<0>() == 0);
        assert(t3333.IndexOf<1>() == 0);
        assert(t3333.IndexOf<2>() == 0);

        t3333 = t3;
        static_assert(t3333.Count == 3);
        static_assert(std::is_same_v<Tuple<long long, long long, long long>, decltype(t3333)>);
        static_assert(std::is_same_v<long long&, decltype(t3333.IndexOf<0>())>);
        assert(t3333.IndexOf<0>() == 1);
        static_assert(std::is_same_v<long long&, decltype(t3333.IndexOf<1>())>);
        assert(t3333.IndexOf<1>() == 2);
        static_assert(std::is_same_v<long long&, decltype(t3333.IndexOf<2>())>);
        assert(t3333.IndexOf<2>() == 3);

        using T4 = decltype(Tuple(true, 'A', 123, 3.14, "ABC", nullptr));

        auto t4 = Tuple(true, 'A', 123, 3.14, "ABC", nullptr);
        static_assert(t4.Count == 6);
        static_assert(std::is_same_v<T4, decltype(t4)>);
        static_assert(std::is_same_v<bool&, decltype(t4.IndexOf<0>())>);
        assert(t4.IndexOf<0>() == true);
        static_assert(std::is_same_v<char&, decltype(t4.IndexOf<1>())>);
        assert(t4.IndexOf<1>() == 'A');
        static_assert(std::is_same_v<int&, decltype(t4.IndexOf<2>())>);
        assert(t4.IndexOf<2>() == 123);
        static_assert(std::is_same_v<double&, decltype(t4.IndexOf<3>())>);
        assert(t4.IndexOf<3>() == 3.14);
        static_assert(std::is_same_v<const char*&, decltype(t4.IndexOf<4>())>);
        assert(!std::strcmp(t4.IndexOf<4>(), "ABC"));
        static_assert(std::is_same_v<std::nullptr_t&, decltype(t4.IndexOf<5>())>);
        assert(t4.IndexOf<5>() == nullptr);

        auto t44 = t4;
        static_assert(t44.Count == 6);
        static_assert(std::is_same_v<T4, decltype(t44)>);
        static_assert(std::is_same_v<bool&, decltype(t44.IndexOf<0>())>);
        assert(t44.IndexOf<0>() == true);
        static_assert(std::is_same_v<char&, decltype(t44.IndexOf<1>())>);
        assert(t44.IndexOf<1>() == 'A');
        static_assert(std::is_same_v<int&, decltype(t44.IndexOf<2>())>);
        assert(t44.IndexOf<2>() == 123);
        static_assert(std::is_same_v<double&, decltype(t44.IndexOf<3>())>);
        assert(t44.IndexOf<3>() == 3.14);
        static_assert(std::is_same_v<const char*&, decltype(t44.IndexOf<4>())>);
        assert(!std::strcmp(t44.IndexOf<4>(), "ABC"));
        static_assert(std::is_same_v<std::nullptr_t&, decltype(t44.IndexOf<5>())>);
        assert(t44.IndexOf<5>() == nullptr);

        t44.IndexOf<0>() = false;
        assert(t44.IndexOf<0>() == false);
        t44.IndexOf<1>() = 'a';
        assert(t44.IndexOf<1>() == 'a');
        t44.IndexOf<2>() = -123;
        assert(t44.IndexOf<2>() == -123);
        t44.IndexOf<3>() = -3.14;
        assert(t44.IndexOf<3>() == -3.14);
        t44.IndexOf<4>() = "abc";
        assert(!std::strcmp(t44.IndexOf<4>(), "abc"));
        t44.IndexOf<5>() = nullptr;
        assert(t44.IndexOf<5>() == nullptr);

        t44 = t4;
        static_assert(t44.Count == 6);
        static_assert(std::is_same_v<T4, decltype(t44)>);
        static_assert(std::is_same_v<bool&, decltype(t44.IndexOf<0>())>);
        assert(t44.IndexOf<0>() == true);
        static_assert(std::is_same_v<char&, decltype(t44.IndexOf<1>())>);
        assert(t44.IndexOf<1>() == 'A');
        static_assert(std::is_same_v<int&, decltype(t44.IndexOf<2>())>);
        assert(t44.IndexOf<2>() == 123);
        static_assert(std::is_same_v<double&, decltype(t44.IndexOf<3>())>);
        assert(t44.IndexOf<3>() == 3.14);
        static_assert(std::is_same_v<const char*&, decltype(t44.IndexOf<4>())>);
        assert(!std::strcmp(t44.IndexOf<4>(), "ABC"));
        static_assert(std::is_same_v<std::nullptr_t&, decltype(t44.IndexOf<5>())>);
        assert(t44.IndexOf<5>() == nullptr);

        using T44 = decltype(Tuple(1, 2, 3ll, 3.14, std::string(), nullptr));

        auto t444 = T44(true, 'A', 123, 3.14, "ABC", nullptr);
        static_assert(t444.Count == 6);
        static_assert(std::is_same_v<T44, decltype(t444)>);
        static_assert(std::is_same_v<int&, decltype(t444.IndexOf<0>())>);
        assert(t444.IndexOf<0>() == true);
        static_assert(std::is_same_v<int&, decltype(t444.IndexOf<1>())>);
        assert(t444.IndexOf<1>() == 'A');
        static_assert(std::is_same_v<long long&, decltype(t444.IndexOf<2>())>);
        assert(t444.IndexOf<2>() == 123);
        static_assert(std::is_same_v<double&, decltype(t444.IndexOf<3>())>);
        assert(t444.IndexOf<3>() == 3.14);
        static_assert(std::is_same_v<std::string&, decltype(t444.IndexOf<4>())>);
        assert(t444.IndexOf<4>() == "ABC");
        static_assert(std::is_same_v<std::nullptr_t&, decltype(t444.IndexOf<5>())>);
        assert(t444.IndexOf<5>() == nullptr);

        auto t4444 = T44(t4);
        static_assert(t4444.Count == 6);
        static_assert(std::is_same_v<T44, decltype(t4444)>);
        static_assert(std::is_same_v<int&, decltype(t4444.IndexOf<0>())>);
        assert(t4444.IndexOf<0>() == true);
        static_assert(std::is_same_v<int&, decltype(t4444.IndexOf<1>())>);
        assert(t4444.IndexOf<1>() == 'A');
        static_assert(std::is_same_v<long long&, decltype(t4444.IndexOf<2>())>);
        assert(t4444.IndexOf<2>() == 123);
        static_assert(std::is_same_v<double&, decltype(t4444.IndexOf<3>())>);
        assert(t4444.IndexOf<3>() == 3.14);
        static_assert(std::is_same_v<std::string&, decltype(t4444.IndexOf<4>())>);
        assert(t4444.IndexOf<4>() == "ABC");
        static_assert(std::is_same_v<std::nullptr_t&, decltype(t4444.IndexOf<5>())>);
        assert(t4444.IndexOf<5>() == nullptr);

        t4444.IndexOf<0>() = false;
        assert(t4444.IndexOf<0>() == false);
        t4444.IndexOf<1>() = 'a';
        assert(t4444.IndexOf<1>() == 'a');
        t4444.IndexOf<2>() = -123;
        assert(t4444.IndexOf<2>() == -123);
        t4444.IndexOf<3>() = -3.14;
        assert(t4444.IndexOf<3>() == -3.14);
        t4444.IndexOf<4>() = "abc";
        assert(t4444.IndexOf<4>() =="abc");
        t4444.IndexOf<5>() = nullptr;
        assert(t4444.IndexOf<5>() == nullptr);

        t4444 = t4;
        static_assert(t4444.Count == 6);
        static_assert(std::is_same_v<T44, decltype(t4444)>);
        static_assert(std::is_same_v<int&, decltype(t4444.IndexOf<0>())>);
        assert(t4444.IndexOf<0>() == true);
        static_assert(std::is_same_v<int&, decltype(t4444.IndexOf<1>())>);
        assert(t4444.IndexOf<1>() == 'A');
        static_assert(std::is_same_v<long long&, decltype(t4444.IndexOf<2>())>);
        assert(t4444.IndexOf<2>() == 123);
        static_assert(std::is_same_v<double&, decltype(t4444.IndexOf<3>())>);
        assert(t4444.IndexOf<3>() == 3.14);
        static_assert(std::is_same_v<std::string&, decltype(t4444.IndexOf<4>())>);
        assert(t4444.IndexOf<4>() == "ABC");
        static_assert(std::is_same_v<std::nullptr_t&, decltype(t4444.IndexOf<5>())>);
        assert(t4444.IndexOf<5>() == nullptr);

        auto u0 = Tuple<>::Create();
        static_assert(std::is_same_v<Tuple<>, decltype(u0)>);
        static_assert(u0.Count == 0);

        auto u1 = Tuple<>::Create(1);
        static_assert(u1.Count == 1);
        static_assert(std::is_same_v<Tuple<int>, decltype(u1)>);
        static_assert(std::is_same_v<int&, decltype(u1.IndexOf<0>())>);
        assert(u1.IndexOf<0>() == 1);

        auto u11 = u1;
        static_assert(u11.Count == 1);
        static_assert(std::is_same_v<Tuple<int>, decltype(u11)>);
        static_assert(std::is_same_v<int&, decltype(u11.IndexOf<0>())>);
        assert(u11.IndexOf<0>() == 1);

        u11.IndexOf<0>() = -1;
        assert(u11.IndexOf<0>() == -1);
        u11.IndexBy(0, [](auto&& e) { e = 1; });
        assert(u11.IndexOf<0>() == 1);
        u11.ForEach([](auto&& e) { e = 0; });
        assert(u11.IndexOf<0>() == 0);

        u11 = u1;
        static_assert(u11.Count == 1);
        static_assert(std::is_same_v<Tuple<int>, decltype(u11)>);
        static_assert(std::is_same_v<int&, decltype(u11.IndexOf<0>())>);
        assert(u11.IndexOf<0>() == 1);

        Tuple<long long> u111 = Tuple<>::Create(1);
        static_assert(u111.Count == 1);
        static_assert(std::is_same_v<Tuple<long long>, decltype(u111)>);
        static_assert(std::is_same_v<long long&, decltype(u111.IndexOf<0>())>);
        assert(u111.IndexOf<0>() == 1);

        Tuple<long long> u1111 = u1;
        static_assert(u1111.Count == 1);
        static_assert(std::is_same_v<Tuple<long long>, decltype(u1111)>);
        static_assert(std::is_same_v<long long&, decltype(u1111.IndexOf<0>())>);
        assert(u1111.IndexOf<0>() == 1);

        u1111.IndexOf<0>() = -1;
        assert(u1111.IndexOf<0>() == -1);
        u1111.IndexBy(0, [](auto&& e) { e = 1; });
        assert(u1111.IndexOf<0>() == 1);
        u1111.ForEach([](auto&& e) { e = 0; });
        assert(u1111.IndexOf<0>() == 0);

        u1111 = u1;
        static_assert(u1111.Count == 1);
        static_assert(std::is_same_v<Tuple<long long>, decltype(u1111)>);
        static_assert(std::is_same_v<long long&, decltype(u1111.IndexOf<0>())>);
        assert(u1111.IndexOf<0>() == 1);

        auto u2 = Tuple<>::Create(1, 2);
        static_assert(u2.Count == 2);
        static_assert(std::is_same_v<Tuple<int, int>, decltype(u2)>);
        static_assert(std::is_same_v<int&, decltype(u2.IndexOf<0>())>);
        assert(u2.IndexOf<0>() == 1);
        static_assert(std::is_same_v<int&, decltype(u2.IndexOf<1>())>);
        assert(u2.IndexOf<1>() == 2);

        auto u22 = u2;
        static_assert(u22.Count == 2);
        static_assert(std::is_same_v<Tuple<int, int>, decltype(u22)>);
        static_assert(std::is_same_v<int&, decltype(u22.IndexOf<0>())>);
        assert(u22.IndexOf<0>() == 1);
        static_assert(std::is_same_v<int&, decltype(u22.IndexOf<1>())>);
        assert(u22.IndexOf<1>() == 2);

        u22.IndexOf<0>() = -1;
        assert(u22.IndexOf<0>() == -1);
        u22.IndexOf<1>() = -2;
        assert(u22.IndexOf<1>() == -2);
        u22.IndexBy(0, [](auto&& e) { e = 1; });
        assert(u22.IndexOf<0>() == 1);
        u22.IndexBy(1, [](auto&& e) { e = 2; });
        assert(u22.IndexOf<1>() == 2);
        u22.ForEach([](auto&& e) { e = 0; });
        assert(u22.IndexOf<0>() == 0);
        assert(u22.IndexOf<1>() == 0);

        u22 = u2;
        static_assert(u22.Count == 2);
        static_assert(std::is_same_v<Tuple<int, int>, decltype(u22)>);
        static_assert(std::is_same_v<int&, decltype(u22.IndexOf<0>())>);
        assert(u22.IndexOf<0>() == 1);
        static_assert(std::is_same_v<int&, decltype(u22.IndexOf<1>())>);
        assert(u22.IndexOf<1>() == 2);

        using U22 = decltype(Tuple(1ll, 2ll));

        auto u222 = U22(Tuple<>::Create(1, 2));
        static_assert(u222.Count == 2);
        static_assert(std::is_same_v<Tuple<long long, long long>, decltype(u222)>);
        static_assert(std::is_same_v<long long&, decltype(u222.IndexOf<0>())>);
        assert(u222.IndexOf<0>() == 1);
        static_assert(std::is_same_v<long long&, decltype(u222.IndexOf<1>())>);
        assert(u222.IndexOf<1>() == 2);

        auto u2222 = U22(u2);
        static_assert(u2222.Count == 2);
        static_assert(std::is_same_v<Tuple<long long, long long>, decltype(u2222)>);
        static_assert(std::is_same_v<long long&, decltype(u2222.IndexOf<0>())>);
        assert(u2222.IndexOf<0>() == 1);
        static_assert(std::is_same_v<long long&, decltype(u2222.IndexOf<1>())>);
        assert(u2222.IndexOf<1>() == 2);

        u2222.IndexOf<0>() = -1;
        assert(u2222.IndexOf<0>() == -1);
        u2222.IndexOf<1>() = -2;
        assert(u2222.IndexOf<1>() == -2);
        u2222.IndexBy(0, [](auto&& e) { e = 1; });
        assert(u2222.IndexOf<0>() == 1);
        u2222.IndexBy(1, [](auto&& e) { e = 2; });
        assert(u2222.IndexOf<1>() == 2);
        u2222.ForEach([](auto&& e) { e = 0; });
        assert(u2222.IndexOf<0>() == 0);
        assert(u2222.IndexOf<1>() == 0);

        u2222 = u2;
        static_assert(u2222.Count == 2);
        static_assert(std::is_same_v<Tuple<long long, long long>, decltype(u2222)>);
        static_assert(std::is_same_v<long long&, decltype(u2222.IndexOf<0>())>);
        assert(u2222.IndexOf<0>() == 1);
        static_assert(std::is_same_v<long long&, decltype(u2222.IndexOf<1>())>);
        assert(u2222.IndexOf<1>() == 2);

        auto u3 = Tuple<>::Create(1, 2, 3);
        static_assert(u3.Count == 3);
        static_assert(std::is_same_v<Tuple<int, int, int>, decltype(u3)>);
        static_assert(std::is_same_v<int&, decltype(u3.IndexOf<0>())>);
        assert(u3.IndexOf<0>() == 1);
        static_assert(std::is_same_v<int&, decltype(u3.IndexOf<1>())>);
        assert(u3.IndexOf<1>() == 2);
        static_assert(std::is_same_v<int&, decltype(u3.IndexOf<2>())>);
        assert(u3.IndexOf<2>() == 3);

        auto u33 = u3;
        static_assert(u33.Count == 3);
        static_assert(std::is_same_v<Tuple<int, int, int>, decltype(u33)>);
        static_assert(std::is_same_v<int&, decltype(u33.IndexOf<0>())>);
        assert(u33.IndexOf<0>() == 1);
        static_assert(std::is_same_v<int&, decltype(u33.IndexOf<1>())>);
        assert(u33.IndexOf<1>() == 2);
        static_assert(std::is_same_v<int&, decltype(u33.IndexOf<2>())>);
        assert(u33.IndexOf<2>() == 3);

        u33.IndexOf<0>() = -1;
        assert(u33.IndexOf<0>() == -1);
        u33.IndexOf<1>() = -2;
        assert(u33.IndexOf<1>() == -2);
        u33.IndexOf<2>() = -3;
        assert(u33.IndexOf<2>() == -3);
        u33.IndexBy(0, [](auto&& e) { e = 1; });
        assert(u33.IndexOf<0>() == 1);
        u33.IndexBy(1, [](auto&& e) { e = 2; });
        assert(u33.IndexOf<1>() == 2);
        u33.IndexBy(2, [](auto&& e) { e = 3; });
        assert(u33.IndexOf<2>() == 3);
        u33.ForEach([](auto&& e) { e = 0; });
        assert(u33.IndexOf<0>() == 0);
        assert(u33.IndexOf<1>() == 0);
        assert(u33.IndexOf<2>() == 0);

        u33 = u3;
        static_assert(u33.Count == 3);
        static_assert(std::is_same_v<Tuple<int, int, int>, decltype(u33)>);
        static_assert(std::is_same_v<int&, decltype(u33.IndexOf<0>())>);
        assert(u33.IndexOf<0>() == 1);
        static_assert(std::is_same_v<int&, decltype(u33.IndexOf<1>())>);
        assert(u33.IndexOf<1>() == 2);
        static_assert(std::is_same_v<int&, decltype(u33.IndexOf<2>())>);
        assert(u33.IndexOf<2>() == 3);

        using U33 = decltype(Tuple(1ll, 2ll, 3ll));

        auto u333 = U33(Tuple<>::Create(1, 2, 3));
        static_assert(u333.Count == 3);
        static_assert(std::is_same_v<Tuple<long long, long long, long long>, decltype(u333)>);
        static_assert(std::is_same_v<long long&, decltype(u333.IndexOf<0>())>);
        assert(u333.IndexOf<0>() == 1);
        static_assert(std::is_same_v<long long&, decltype(u333.IndexOf<1>())>);
        assert(u333.IndexOf<1>() == 2);
        static_assert(std::is_same_v<long long&, decltype(u333.IndexOf<2>())>);
        assert(u333.IndexOf<2>() == 3);

        auto u3333 = U33(u3);
        static_assert(u3333.Count == 3);
        static_assert(std::is_same_v<Tuple<long long, long long, long long>, decltype(u3333)>);
        static_assert(std::is_same_v<long long&, decltype(u3333.IndexOf<0>())>);
        assert(u3333.IndexOf<0>() == 1);
        static_assert(std::is_same_v<long long&, decltype(u3333.IndexOf<1>())>);
        assert(u3333.IndexOf<1>() == 2);
        static_assert(std::is_same_v<long long&, decltype(u3333.IndexOf<2>())>);
        assert(u3333.IndexOf<2>() == 3);

        u3333.IndexOf<0>() = -1;
        assert(u3333.IndexOf<0>() == -1);
        u3333.IndexOf<1>() = -2;
        assert(u3333.IndexOf<1>() == -2);
        u3333.IndexOf<2>() = -3;
        assert(u3333.IndexOf<2>() == -3);
        u3333.IndexBy(0, [](auto&& e) { e = 1; });
        assert(u3333.IndexOf<0>() == 1);
        u3333.IndexBy(1, [](auto&& e) { e = 2; });
        assert(u3333.IndexOf<1>() == 2);
        u3333.IndexBy(2, [](auto&& e) { e = 3; });
        assert(u3333.IndexOf<2>() == 3);
        u3333.ForEach([](auto&& e) { e = 0; });
        assert(u3333.IndexOf<0>() == 0);
        assert(u3333.IndexOf<1>() == 0);
        assert(u3333.IndexOf<2>() == 0);

        u3333 = u3;
        static_assert(u3333.Count == 3);
        static_assert(std::is_same_v<Tuple<long long, long long, long long>, decltype(u3333)>);
        static_assert(std::is_same_v<long long&, decltype(u3333.IndexOf<0>())>);
        assert(u3333.IndexOf<0>() == 1);
        static_assert(std::is_same_v<long long&, decltype(u3333.IndexOf<1>())>);
        assert(u3333.IndexOf<1>() == 2);
        static_assert(std::is_same_v<long long&, decltype(u3333.IndexOf<2>())>);
        assert(u3333.IndexOf<2>() == 3);

        using U4 = decltype(Tuple(true, 'A', 123, 3.14, "ABC", nullptr));

        auto u4 = Tuple<>::Create(true, 'A', 123, 3.14, "ABC", nullptr);
        static_assert(u4.Count == 6);
        static_assert(std::is_same_v<U4, decltype(u4)>);
        static_assert(std::is_same_v<bool&, decltype(u4.IndexOf<0>())>);
        assert(u4.IndexOf<0>() == true);
        static_assert(std::is_same_v<char&, decltype(u4.IndexOf<1>())>);
        assert(u4.IndexOf<1>() == 'A');
        static_assert(std::is_same_v<int&, decltype(u4.IndexOf<2>())>);
        assert(u4.IndexOf<2>() == 123);
        static_assert(std::is_same_v<double&, decltype(u4.IndexOf<3>())>);
        assert(u4.IndexOf<3>() == 3.14);
        static_assert(std::is_same_v<const char*&, decltype(u4.IndexOf<4>())>);
        assert(!std::strcmp(u4.IndexOf<4>(), "ABC"));
        static_assert(std::is_same_v<std::nullptr_t&, decltype(u4.IndexOf<5>())>);
        assert(u4.IndexOf<5>() == nullptr);

        auto u44 = u4;
        static_assert(u44.Count == 6);
        static_assert(std::is_same_v<U4, decltype(u44)>);
        static_assert(std::is_same_v<bool&, decltype(u44.IndexOf<0>())>);
        assert(u44.IndexOf<0>() == true);
        static_assert(std::is_same_v<char&, decltype(u44.IndexOf<1>())>);
        assert(u44.IndexOf<1>() == 'A');
        static_assert(std::is_same_v<int&, decltype(u44.IndexOf<2>())>);
        assert(u44.IndexOf<2>() == 123);
        static_assert(std::is_same_v<double&, decltype(u44.IndexOf<3>())>);
        assert(u44.IndexOf<3>() == 3.14);
        static_assert(std::is_same_v<const char*&, decltype(u44.IndexOf<4>())>);
        assert(!std::strcmp(u44.IndexOf<4>(), "ABC"));
        static_assert(std::is_same_v<std::nullptr_t&, decltype(u44.IndexOf<5>())>);
        assert(u44.IndexOf<5>() == nullptr);

        u44.IndexOf<0>() = false;
        assert(u44.IndexOf<0>() == false);
        u44.IndexOf<1>() = 'a';
        assert(u44.IndexOf<1>() == 'a');
        u44.IndexOf<2>() = -123;
        assert(u44.IndexOf<2>() == -123);
        u44.IndexOf<3>() = -3.14;
        assert(u44.IndexOf<3>() == -3.14);
        u44.IndexOf<4>() = "abc";
        assert(!std::strcmp(u44.IndexOf<4>(), "abc"));
        u44.IndexOf<5>() = nullptr;
        assert(u44.IndexOf<5>() == nullptr);

        u44 = u4;
        static_assert(u44.Count == 6);
        static_assert(std::is_same_v<U4, decltype(u44)>);
        static_assert(std::is_same_v<bool&, decltype(u44.IndexOf<0>())>);
        assert(u44.IndexOf<0>() == true);
        static_assert(std::is_same_v<char&, decltype(u44.IndexOf<1>())>);
        assert(u44.IndexOf<1>() == 'A');
        static_assert(std::is_same_v<int&, decltype(u44.IndexOf<2>())>);
        assert(u44.IndexOf<2>() == 123);
        static_assert(std::is_same_v<double&, decltype(u44.IndexOf<3>())>);
        assert(u44.IndexOf<3>() == 3.14);
        static_assert(std::is_same_v<const char*&, decltype(u44.IndexOf<4>())>);
        assert(!std::strcmp(u44.IndexOf<4>(), "ABC"));
        static_assert(std::is_same_v<std::nullptr_t&, decltype(u44.IndexOf<5>())>);
        assert(u44.IndexOf<5>() == nullptr);

        using U44 = decltype(Tuple(1, 2, 3ll, 3.14, std::string(), nullptr));

        auto u444 = U44(Tuple<>::Create(true, 'A', 123, 3.14, "ABC", nullptr));
        static_assert(u444.Count == 6);
        static_assert(std::is_same_v<U44, decltype(u444)>);
        static_assert(std::is_same_v<int&, decltype(u444.IndexOf<0>())>);
        assert(u444.IndexOf<0>() == true);
        static_assert(std::is_same_v<int&, decltype(u444.IndexOf<1>())>);
        assert(u444.IndexOf<1>() == 'A');
        static_assert(std::is_same_v<long long&, decltype(u444.IndexOf<2>())>);
        assert(u444.IndexOf<2>() == 123);
        static_assert(std::is_same_v<double&, decltype(u444.IndexOf<3>())>);
        assert(u444.IndexOf<3>() == 3.14);
        static_assert(std::is_same_v<std::string&, decltype(u444.IndexOf<4>())>);
        assert(u444.IndexOf<4>() == "ABC");
        static_assert(std::is_same_v<std::nullptr_t&, decltype(u444.IndexOf<5>())>);
        assert(u444.IndexOf<5>() == nullptr);

        auto u4444 = U44(u4);
        static_assert(u4444.Count == 6);
        static_assert(std::is_same_v<U44, decltype(u4444)>);
        static_assert(std::is_same_v<int&, decltype(u4444.IndexOf<0>())>);
        assert(u4444.IndexOf<0>() == true);
        static_assert(std::is_same_v<int&, decltype(u4444.IndexOf<1>())>);
        assert(u4444.IndexOf<1>() == 'A');
        static_assert(std::is_same_v<long long&, decltype(u4444.IndexOf<2>())>);
        assert(u4444.IndexOf<2>() == 123);
        static_assert(std::is_same_v<double&, decltype(u4444.IndexOf<3>())>);
        assert(u4444.IndexOf<3>() == 3.14);
        static_assert(std::is_same_v<std::string&, decltype(u4444.IndexOf<4>())>);
        assert(u4444.IndexOf<4>() == "ABC");
        static_assert(std::is_same_v<std::nullptr_t&, decltype(u4444.IndexOf<5>())>);
        assert(u4444.IndexOf<5>() == nullptr);

        u4444.IndexOf<0>() = false;
        assert(u4444.IndexOf<0>() == false);
        u4444.IndexOf<1>() = 'a';
        assert(u4444.IndexOf<1>() == 'a');
        u4444.IndexOf<2>() = -123;
        assert(u4444.IndexOf<2>() == -123);
        u4444.IndexOf<3>() = -3.14;
        assert(u4444.IndexOf<3>() == -3.14);
        u4444.IndexOf<4>() = "abc";
        assert(u4444.IndexOf<4>() =="abc");
        u4444.IndexOf<5>() = nullptr;
        assert(u4444.IndexOf<5>() == nullptr);

        u4444 = u4;
        static_assert(u4444.Count == 6);
        static_assert(std::is_same_v<U44, decltype(u4444)>);
        static_assert(std::is_same_v<int&, decltype(u4444.IndexOf<0>())>);
        assert(u4444.IndexOf<0>() == true);
        static_assert(std::is_same_v<int&, decltype(u4444.IndexOf<1>())>);
        assert(u4444.IndexOf<1>() == 'A');
        static_assert(std::is_same_v<long long&, decltype(u4444.IndexOf<2>())>);
        assert(u4444.IndexOf<2>() == 123);
        static_assert(std::is_same_v<double&, decltype(u4444.IndexOf<3>())>);
        assert(u4444.IndexOf<3>() == 3.14);
        static_assert(std::is_same_v<std::string&, decltype(u4444.IndexOf<4>())>);
        assert(u4444.IndexOf<4>() == "ABC");
        static_assert(std::is_same_v<std::nullptr_t&, decltype(u4444.IndexOf<5>())>);
        assert(u4444.IndexOf<5>() == nullptr);

        auto s1 = std::string("1");
        auto r1 = Tuple<      std::string& >(          s1 );
        assert(r1.IndexOf<0>() == "1");
        r1.IndexOf<0>() = "-1";
        assert(r1.IndexOf<0>() == "-1");
        assert(s1 == "-1");

        auto sx = std::string("X");
        auto rx = Tuple<      std::string  >(          sx );
        assert(rx.IndexOf<0>() == "X");
        r1 = rx;
        assert(r1.IndexOf<0>() == "X");
        assert(s1 == "X");

        auto s2 = std::string("1");
        auto r2 = Tuple<const std::string& >(          s2 );
        assert(r2.IndexOf<0>() == "1");

        auto s3 = std::string("1");
        auto r3 = Tuple<      std::string&&>(std::move(s3));
        assert(r3.IndexOf<0>() == "1");
        r3.IndexOf<0>() = "-1";
        assert(r3.IndexOf<0>() == "-1");
        assert(s3 == "-1");

        auto ta = Tuple() & true & 'A' & 123 & 3.14 & "ABC" & nullptr;
        static_assert(ta.Count == 6);
        static_assert(std::is_same_v<T4, decltype(ta)>);
        static_assert(std::is_same_v<bool&, decltype(ta.IndexOf<0>())>);
        assert(ta.IndexOf<0>() == true);
        static_assert(std::is_same_v<char&, decltype(ta.IndexOf<1>())>);
        assert(ta.IndexOf<1>() == 'A');
        static_assert(std::is_same_v<int&, decltype(ta.IndexOf<2>())>);
        assert(ta.IndexOf<2>() == 123);
        static_assert(std::is_same_v<double&, decltype(ta.IndexOf<3>())>);
        assert(ta.IndexOf<3>() == 3.14);
        static_assert(std::is_same_v<const char*&, decltype(ta.IndexOf<4>())>);
        assert(!std::strcmp(ta.IndexOf<4>(), "ABC"));
        static_assert(std::is_same_v<std::nullptr_t&, decltype(ta.IndexOf<5>())>);
        assert(ta.IndexOf<5>() == nullptr);

        auto tb = Tuple() + Tuple(true) + Tuple('A') + Tuple(123) + Tuple(3.14) + Tuple("ABC") + Tuple(nullptr);
        static_assert(tb.Count == 6);
        static_assert(std::is_same_v<T4, decltype(tb)>);
        static_assert(std::is_same_v<bool&, decltype(tb.IndexOf<0>())>);
        assert(tb.IndexOf<0>() == true);
        static_assert(std::is_same_v<char&, decltype(tb.IndexOf<1>())>);
        assert(tb.IndexOf<1>() == 'A');
        static_assert(std::is_same_v<int&, decltype(tb.IndexOf<2>())>);
        assert(tb.IndexOf<2>() == 123);
        static_assert(std::is_same_v<double&, decltype(tb.IndexOf<3>())>);
        assert(tb.IndexOf<3>() == 3.14);
        static_assert(std::is_same_v<const char*&, decltype(tb.IndexOf<4>())>);
        assert(!std::strcmp(tb.IndexOf<4>(), "ABC"));
        static_assert(std::is_same_v<std::nullptr_t&, decltype(tb.IndexOf<5>())>);
        assert(tb.IndexOf<5>() == nullptr);

        auto tc = Tuple<>::Concat(Tuple(), true, 'A', 123, 3.14, "ABC", nullptr);
        static_assert(tc.Count == 6);
        static_assert(std::is_same_v<T4, decltype(tc)>);
        static_assert(std::is_same_v<bool&, decltype(tc.IndexOf<0>())>);
        assert(tc.IndexOf<0>() == true);
        static_assert(std::is_same_v<char&, decltype(tc.IndexOf<1>())>);
        assert(tc.IndexOf<1>() == 'A');
        static_assert(std::is_same_v<int&, decltype(tc.IndexOf<2>())>);
        assert(tc.IndexOf<2>() == 123);
        static_assert(std::is_same_v<double&, decltype(tc.IndexOf<3>())>);
        assert(tc.IndexOf<3>() == 3.14);
        static_assert(std::is_same_v<const char*&, decltype(tc.IndexOf<4>())>);
        assert(!std::strcmp(tc.IndexOf<4>(), "ABC"));
        static_assert(std::is_same_v<std::nullptr_t&, decltype(tc.IndexOf<5>())>);
        assert(tc.IndexOf<5>() == nullptr);

        auto td = Tuple<>::Concat(Tuple(), Tuple(true, 'A', 123), Tuple(3.14, "ABC", nullptr));
        static_assert(td.Count == 6);
        static_assert(std::is_same_v<T4, decltype(td)>);
        static_assert(std::is_same_v<bool&, decltype(td.IndexOf<0>())>);
        assert(td.IndexOf<0>() == true);
        static_assert(std::is_same_v<char&, decltype(td.IndexOf<1>())>);
        assert(td.IndexOf<1>() == 'A');
        static_assert(std::is_same_v<int&, decltype(td.IndexOf<2>())>);
        assert(td.IndexOf<2>() == 123);
        static_assert(std::is_same_v<double&, decltype(td.IndexOf<3>())>);
        assert(td.IndexOf<3>() == 3.14);
        static_assert(std::is_same_v<const char*&, decltype(td.IndexOf<4>())>);
        assert(!std::strcmp(td.IndexOf<4>(), "ABC"));
        static_assert(std::is_same_v<std::nullptr_t&, decltype(td.IndexOf<5>())>);
        assert(td.IndexOf<5>() == nullptr);
    });
}
#endif//D_AKR_TEST

#endif//Z_AKR_TUPLE_HH
