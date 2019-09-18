#pragma once

#include <memory>
#include <utility>

namespace moonflower {

template <typename... Ts> struct overload : Ts... { using Ts::operator()...; };
template <typename... Ts> overload(Ts...) -> overload<Ts...>;

template <typename T>
class recursive_wrapper {
public:
    recursive_wrapper() = default;
    recursive_wrapper(recursive_wrapper&&) = default;
    recursive_wrapper(const recursive_wrapper& o) : data(std::make_unique<T>(*o.data)) {}
    recursive_wrapper& operator=(recursive_wrapper&&) = default;
    recursive_wrapper& operator=(const recursive_wrapper& o) { *data = *o.data; }

    recursive_wrapper(T t) : data(std::make_unique<T>(std::move(t))) {}

    T& get() { return *data; }
    const T& get() const { return *data; }

private:
    std::unique_ptr<T> data;
};

}
