#pragma once

#include "Error.h"
#include <variant>
#include <stdexcept>
#include <utility>

namespace yibo {

template<typename T, typename E = Error>
class [[nodiscard]] Result {
public:
    static Result Ok(T value) {
        return Result(std::move(value), true);
    }

    static Result Err(E error) {
        return Result(std::move(error), false);
    }

    bool IsOk() const { return m_is_ok; }
    bool IsErr() const { return !m_is_ok; }

    T& Value() {
        if (!m_is_ok) {
            throw std::logic_error("Called Value() on Err result");
        }
        return std::get<T>(m_data);
    }

    const T& Value() const {
        if (!m_is_ok) {
            throw std::logic_error("Called Value() on Err result");
        }
        return std::get<T>(m_data);
    }

    E& Error() {
        if (m_is_ok) {
            throw std::logic_error("Called Error() on Ok result");
        }
        return std::get<E>(m_data);
    }

    const E& Error() const {
        if (m_is_ok) {
            throw std::logic_error("Called Error() on Ok result");
        }
        return std::get<E>(m_data);
    }

    T ValueOr(T default_value) const {
        if (m_is_ok) {
            return std::get<T>(m_data);
        }
        return default_value;
    }

    template<typename Func>
    auto AndThen(Func&& func) -> decltype(func(std::declval<T&>())) {
        using ReturnType = decltype(func(std::declval<T&>()));
        if (IsOk()) {
            return func(Value());
        }
        return ReturnType::Err(Error());
    }

    template<typename Func>
    auto Map(Func&& func) -> Result<decltype(func(std::declval<T&>())), E> {
        using ReturnType = decltype(func(std::declval<T&>()));
        if (IsOk()) {
            return Result<ReturnType, E>::Ok(func(Value()));
        }
        return Result<ReturnType, E>::Err(Error());
    }

    template<typename Func>
    Result<T, E> OrElse(Func&& func) {
        if (IsErr()) {
            return func(Error());
        }
        return Result<T, E>::Ok(Value());
    }

private:
    Result(T value, bool is_ok)
        : m_data(std::move(value)), m_is_ok(is_ok) {}

    Result(E error, bool is_ok)
        : m_data(std::move(error)), m_is_ok(is_ok) {}

    std::variant<T, E> m_data;
    bool m_is_ok;
};

template<typename E>
class [[nodiscard]] Result<void, E> {
public:
    static Result Ok() {
        return Result(true);
    }

    static Result Err(E error) {
        return Result(std::move(error));
    }

    bool IsOk() const { return m_is_ok; }
    bool IsErr() const { return !m_is_ok; }

    void Value() const {
        if (!m_is_ok) {
            throw std::logic_error("Called Value() on Err result");
        }
    }

    E& Error() {
        if (m_is_ok) {
            throw std::logic_error("Called Error() on Ok result");
        }
        return std::get<E>(m_data);
    }

    const E& Error() const {
        if (m_is_ok) {
            throw std::logic_error("Called Error() on Ok result");
        }
        return std::get<E>(m_data);
    }

private:
    Result(bool is_ok) : m_data(std::monostate{}), m_is_ok(is_ok) {}
    Result(E error) : m_data(std::move(error)), m_is_ok(false) {}

    std::variant<std::monostate, E> m_data;
    bool m_is_ok;
};

} // namespace yibo
