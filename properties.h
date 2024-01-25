#pragma once

#include <functional>
#include <iostream>
#include <cassert>

template<typename T>
class Property {
public:
    Property() {}
    operator const T& () const {
        // Call override getter if we have it
        if (getter) return getter();
        return get();
    }
    const T& operator = (const T& other) {
        // Call override setter if we have it
        if (setter) return setter(other);
        return set(other);
    }
    bool operator == (const T& other) const {
        // Static cast makes sure our getter operator is called, so we could use overrides if those are in place
        return static_cast<const T&>(*this) == other;
    }
    // Use this to always get without overrides, useful for use with overriding implementations
    const T& get() const {
        return t;
    }
    // Use this to always set without overrides, useful for use with overriding implementations
    const T& set(const T& other) {
        return t = other;
    }
    // Assign getter and setter to these properties
    std::function<const T& ()> getter;
    std::function<const T& (const T&)> setter;
private:
    T t;
};

