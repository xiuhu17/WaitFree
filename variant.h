//Position<Ts...> 从 Ts... 找到 T类型的下标 
template<int id, typename U, typename T, typename ...Ts>
struct Position {
    constexpr static int pos = is_same<U, T>::value ? id : Position<id + 1, U, Ts...>::pos;
};
template<int id, typename U, typename T>
struct Position<id, U, T> {
    constexpr static int pos = id;
};

//MaxSize<Ts...>  Ts... 最大的一个
template<typename  T, typename ...Ts>
struct MaxSize {
    constexpr static int size = sizeof(T) > MaxSize<Ts...>::size ? sizeof(T) : MaxSize<Ts...>::size;
};
template<typename  T>
struct MaxSize<T> {
    constexpr static int  size = sizeof(T);
};

//Type_element<idx,Ts...> 返回下标为 idx 的类型
template<int idx, typename T, typename ...Ts>
struct Type_element {
    using Type = typename  Type_element<idx - 1, Ts...>::Type;
};
template<typename T, typename ...Ts>
struct Type_element<0, T, Ts...> {
    using Type = T;
};


//创建析构函数
template<class T>
void destroy(char* data) {
    reinterpret_cast<T*>(data)->~T();
}
template<class ...Ts>
struct Variant {
    int type = -1;
    char data[MaxSize<Ts...>::size] = { 0 };
    using destroy_func_t = void(*)(char*);//析构函数类型
    // 析构函数 的数组 
    constexpr static destroy_func_t destroy_func[] = { destroy<Ts>... };
    Variant() {};
    // 同类型的拷贝构造
    Variant(const Variant<Ts...>& rhs) {
        type = rhs.type;
        memcpy(data, rhs.data, MaxSize<Ts...>::size);
    }
   //因为是构造函数，就不需要调用析构函数了
    template<typename T>
    Variant(T&& rhs) {
        type = Position<0, T, Ts...>::pos;
        *reinterpret_cast<T*>(data) = forward<T>(rhs);
    }
    //同类型的拷贝
    void operator =(const Variant<Ts...>& rhs) {
        type = rhs.type;
        memcpy(data, rhs.data, MaxSize<Ts...>::size);
    }
    // 完美转发
    template<typename T>
    void operator = (T&& rhs) {
        if (type != -1) {
            destroy_func[type](data);
        }
        type = Position<0, T, Ts...>::pos;
        // 去掉带引用的类型
        using rm_ref = typename remove_reference<T>::type;
        memset(data, 0, sizeof(rm_ref));
        *reinterpret_cast<rm_ref*>(data) = forward<T>(rhs);
    }
    //根据类型返回
    template<typename T>
    T& get() {
        return *reinterpret_cast<T*>(data);
    }
    //根据下标返回
    template<int id>
    auto& get() {
        using T = typename Type_element<id, Ts...>::Type;
        return *reinterpret_cast<T*>(data);
    }
    //析构
    ~Variant() {
        if (type != -1) {
            destroy_func[type](data);
        }
    }
};
struct line
{
    int x1, y1, x2, y2;
};
