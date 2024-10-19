template <std::size_t idx, typename T, typename... Ts>
struct Type_element {
    using Type = typename Type_element<idx - 1, Ts...>::Type;
};
template <typename T, typename... Ts>
struct Type_element<0, T, Ts...> {
    using Type = T;
};

template <int idx, typename... Ts>
using Type_element_t = typename Type_element<idx, Ts...>::Type;

template <typename T, std::size_t idx>
struct Item {
    T value;
};
template <typename Index, typename... Ts>
struct TupleImpl;
template <std::size_t... idxs, typename... Ts>
struct TupleImpl<std::index_sequence<idxs...>, Ts...> : Item<Ts, idxs>... {
    constexpr static auto args_size = sizeof...(Ts);
    TupleImpl<std::index_sequence<idxs...>, Ts...>(const Ts &...args) :
        Item<Ts, idxs>{args}... {
    }
    template <std::size_t idx>
    auto &get() {
        static_assert(0 <= idx && idx < args_size);
        return Item<Type_element_t<idx, Ts...>, idx>::value;
    }
};

template <typename... Ts>
struct Tuple : TupleImpl<std::make_index_sequence<sizeof...(Ts)>, Ts...> {
    Tuple(const Ts &...args) :
        TupleImpl<std::make_index_sequence<sizeof...(Ts)>, Ts...>(args...){};
};

int main() {
    Tuple<int, int, double> t(1, 2, 3.14);
    std::cout << t.get<0>() << "\n";
    t.get<2>() = 114.514;
    std::cout << t.get<2>() << "\n";
}
