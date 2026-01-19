#pragma once

#include <optional>
#include <utility>

namespace yibo {

template<typename T>
class Optional {
public:
    Optional() : m_value(std::nullopt) {}
    Optional(T value) : m_value(std::move(value)) {}
    Optional(std::nullopt_t) : m_value(std::nullopt) {}

    bool HasValue() const { return m_value.has_value(); }

    T& Value() { return m_value.value(); }
    const T& Value() const { return m_value.value(); }

    T ValueOr(T default_value) const {
        return m_value.value_or(std::move(default_value));
    }

    template<typename Func>
    auto Map(Func&& func) -> Optional<decltype(func(std::declval<T&>()))> {
        using ReturnType = decltype(func(std::declval<T&>()));
        if (HasValue()) {
            return Optional<ReturnType>(func(Value()));
        }
        return std::nullopt;
    }

    template<typename Func>
    auto FlatMap(Func&& func) -> decltype(func(std::declval<T&>())) {
        if (HasValue()) {
            return func(Value());
        }
        return std::nullopt;
    }

    template<typename Pred>
    Optional<T> Filter(Pred&& pred) {
        if (HasValue() && pred(Value())) {
            return *this;
        }
        return std::nullopt;
    }

private:
    std::optional<T> m_value;
};

} // namespace yibo
