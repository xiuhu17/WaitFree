template <typename T, typename... Ts>
class Tuple: public Tuple<Ts... > {
    public:
        using parent_type = Tuple<Ts...>;
        using curr_type = T;
        T val;
        Tuple() = default;
        Tuple(T arg, Ts... args) : parent_type(args...), val(arg) {}
        T get_val() {
          return val;  
        }
};

template <typename T>
class Tuple<T> {
    public:
        using curr_type = T;
        T val;
        Tuple() = default;
        Tuple(T arg){
            val = arg;
        }
        T get_val() {
          return val;  
        }
};

template <int idx, typename Tuple_>
struct helper {
    using Type = typename helper<idx - 1, typename Tuple_::parent_type>::Type;
};

template <typename Tuple_>
struct helper<0, Tuple_> {
    using Type = Tuple_;
};


template <int idx, typename Tuple_>
auto get(Tuple_ input) {
    auto grab = static_cast<typename helper<idx, Tuple_>::Type>(input);
    return grab.get_val();
}
