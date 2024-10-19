#include <concepts>
#include <utility>
#include <iostream>
#include <vector>
#include <type_traits>
#include <memory>

template <typename... Ts>
constexpr bool is_all_trivial_v = false;

template <>
constexpr bool is_all_trivial_v<> = true;
template <typename T, typename... Ts>
constexpr bool is_all_trivial_v<T, Ts...> = std::is_trivial_v<T> && is_all_trivial_v<Ts...>;

template <int64_t id, typename U, typename T, typename... Ts>
struct Position {
    constexpr static auto pos = std::is_same_v<U, T> ? id : Position<id + 1, U, Ts...>::pos;
};

template <int64_t id, typename U, typename T>
struct Position<id, U, T> {
    constexpr static auto pos = std::is_same_v<U, T> ? id : -1;
};

template <typename find_type, typename... Ts>
constexpr auto find_idx_by_type = Position<0, find_type, Ts...>::pos;

template <typename... Ts>
struct Variant_storage;

template <>
struct Variant_storage<> {};

template <typename Head, typename... Ts>
struct Variant_storage<Head, Ts...> {
    union {
        Head _value;
        Variant_storage<Ts...> _next;
    };
    Variant_storage(){};
    ~Variant_storage() noexcept {};
    Variant_storage(Variant_storage &&) = default;
    Variant_storage(const Variant_storage &) = default;
    Variant_storage &operator=(Variant_storage &&) = default;
    Variant_storage &operator=(const Variant_storage &) = default;
};

template <size_t id, typename head, typename... Ts>
auto &get_variant_value(Variant_storage<head, Ts...> &v) {
    if constexpr (id == 0) {
        return v._value;
    } else {
        return get_variant_value<id - 1>(v._next);
    }
};

template <size_t id, typename head, typename... Ts>
void destroy_variant_value(Variant_storage<head, Ts...> &v) {
    if constexpr (id == 0) {
        v._value.~head();
    } else {
        destroy_variant_value<id - 1>(v._next);
    }
}

template <typename T, typename head, typename... Ts>
void assign_variant(Variant_storage<head, Ts...> &v, T &&rhs) {
    using type = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<type, head>) {
        *new (&v._value) type = std::forward<T>(rhs);
    } else {
        assign_variant(v._next, std::forward<T>(rhs));
    }
}

template <typename... Ts>
struct Variant {
    constexpr static auto _is_all_trivial = is_all_trivial_v<Ts...>;
    constexpr static auto _size = sizeof...(Ts);
    constexpr static auto null_type = -1;
    int64_t _type_idx{null_type};
    Variant_storage<Ts...> _storage;
    Variant(){};
    Variant(const Variant<Ts...> &rhs) = default;
    Variant &operator=(const Variant<Ts...> &rhs) = default;
    Variant(Variant<Ts...> &&rhs) = default;
    Variant &operator=(Variant<Ts...> &&rhs) = default;

    template <typename T>
        requires(!std::is_same_v<Variant, std::remove_cvref_t<T>>)
    Variant(T &&rhs) {
        assign_variant(_storage, std::forward<T>(rhs));
        _type_idx = find_idx_by_type<T, Ts...>;
    }

    template <typename T>
        requires(!std::is_same_v<Variant, std::remove_cvref_t<T>>)
    auto &operator=(T &&rhs) {
        _destroy();
        assign_variant(_storage, std::forward<T>(rhs));
        _type_idx = find_idx_by_type<T, Ts...>;
        return *this;
    }

    ~Variant() {
        if constexpr (!_is_all_trivial) {
            _destroy();
        }
    }

    template <size_t idx>
    auto &get() {
        return get_variant_value<idx>(_storage);
    }
    template <typename T>
    auto &get() {
        constexpr auto id = find_idx_by_type<T, Ts...>;
        static_assert(id != -1);
        return get<id>();
    }

private:
    template <typename T>
    static void _destroy_variant_value(void *vptr) {
        auto &v = *static_cast<Variant_storage<Ts...> *>(vptr);
        destroy_variant_value<find_idx_by_type<T, Ts...>>(v);
    }
    void _destroy() {
        if (_type_idx != null_type) {
            destroy_func[_type_idx](&_storage);
        }
    }

    using destroy_func_type = void (*)(void *);
    constexpr static destroy_func_type destroy_func[] = {_destroy_variant_value<Ts>...};
};

int main() {
    {
        Variant_storage<int, double, std::string> v;
        static_assert(std::is_same_v<int, decltype(v._value)>);
        static_assert(std::is_same_v<double, decltype(v._next._value)>);
        static_assert(std::is_same_v<std::string, decltype(v._next._next._value)>);
    }

    {
        struct A {
            int x, y, z;
        };

        static_assert(is_all_trivial_v<int, double, char, A>);
        static_assert(!is_all_trivial_v<int, std::string>);
    }

    {
        Variant_storage<int, double, std::string> v;

        static_assert(std::is_same_v<decltype(get_variant_value<0>(v)), int &>);
        static_assert(std::is_same_v<decltype(get_variant_value<1>(v)), double &>);
        static_assert(std::is_same_v<decltype(get_variant_value<2>(v)), std::string &>);
        get_variant_value<0>(v) = 42;
        std::cout << get_variant_value<0>(v) << "\n";
    }
    {
        struct A {
            ~A() {
                std::cout << "~A()\n";
            }
        };
        Variant_storage<int, double, A> v;
        destroy_variant_value<2>(v);
    }
    {
        Variant_storage<int, double, std::string> v;
        assign_variant(v, 42);
        std::cout << get_variant_value<0>(v) << "\n";
        assign_variant(v, 3.14);
        std::cout << get_variant_value<1>(v) << "\n";
    }
    {
        Variant<int, std::vector<int>, double> v;
        v = 42;
        std::cout << v.get<0>() << "\n";
        v = 3.14;
        std::cout << v.get<2>() << "\n";
        std::vector<int> vec{1, 2, 3, 4};
        v = vec;
        std::cout << v.get<1>().size() << "\n";
        for (auto x : v.get<1>()) {
            std::cout << x << "\n";
        }
    }

    {
        struct A {
            ~A() {
                std::cout << "~A()\n";
            }
        };
        struct B {
            ~B() {
                std::cout << "~B()\n";
            }
        };
        Variant<int, A, B> v1 = A{};
        Variant<int, A, B> v2 = B{};
        v2 = v1;
    }

    {
        struct A {
            A() {
                std::cout << "A()\n";
            }
            ~A() {
                std::cout << "~A()\n";
            }
        };
        Variant<std::unique_ptr<A>, int, double> v;
        std::unique_ptr<A> ptr = std::make_unique<A>();
        v = std::move(ptr);
        v = 42;
        std::cout << v.get<int>() << "\n";
    }
}

