#pragma once
#include <memory>

template <typename T>
class box
{
    std::unique_ptr<T> _impl;

public:
    // Automatic construction from a `T`, not a `T*`.
    box() : _impl(new T()) {}
    box(T &&obj) : _impl(new T(std::move(obj))) {}
    box(const T &obj) : _impl(new T(obj)) {}

    // Copy constructor copies `T`.
    box(const box &other) : box(*other._impl) {}
    box &operator=(const box &other)
    {
        *_impl = *other._impl;
        return *this;
    }

    // unique_ptr destroys `T` for us.
    ~box() = default;

    // Access propagates constness.
    T &operator*() { return *_impl; }
    const T &operator*() const { return *_impl; }

    T *operator->() { return _impl.get(); }
    const T *operator->() const { return _impl.get(); }
};
