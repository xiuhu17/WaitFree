template <typename, template <typename...> class>
struct is_specialization: std::false_type {};

template <template <typename...> class Template, typename... Args>
struct is_specialization <Template<Args...>, Template>: std::true_type {};
