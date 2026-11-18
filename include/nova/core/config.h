#pragma once

#include "nova/core/error.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace nova::core {

class ConfigValue {
public:
    enum class Type : int {
        Null = 0,
        Bool = 1,
        Int = 2,
        Double = 3,
        String = 4,
        Array = 5,
        Object = 6,
    };

    ConfigValue() : type_(Type::Null) {}
    ConfigValue(bool v) : type_(Type::Bool), bool_(v) {}
    ConfigValue(int64_t v) : type_(Type::Int), int_(v) {}
    ConfigValue(double v) : type_(Type::Double), double_(v) {}
    ConfigValue(std::string v) : type_(Type::String), string_(std::move(v)) {}
    explicit ConfigValue(std::vector<ConfigValue> v)
        : type_(Type::Array), array_(std::make_shared<std::vector<ConfigValue>>(std::move(v))) {}
    explicit ConfigValue(std::map<std::string, ConfigValue> v)
        : type_(Type::Object),
          object_(std::make_shared<std::map<std::string, ConfigValue>>(std::move(v))) {}

    [[nodiscard]] Type type() const noexcept { return type_; }
    [[nodiscard]] bool is_null() const noexcept { return type_ == Type::Null; }
    [[nodiscard]] bool is_bool() const noexcept { return type_ == Type::Bool; }
    [[nodiscard]] bool is_int() const noexcept { return type_ == Type::Int; }
    [[nodiscard]] bool is_double() const noexcept { return type_ == Type::Double; }
    [[nodiscard]] bool is_string() const noexcept { return type_ == Type::String; }
    [[nodiscard]] bool is_array() const noexcept { return type_ == Type::Array; }
    [[nodiscard]] bool is_object() const noexcept { return type_ == Type::Object; }

    [[nodiscard]] bool as_bool() const noexcept { return bool_; }
    [[nodiscard]] int64_t as_int() const noexcept { return int_; }
    [[nodiscard]] double as_double() const noexcept { return double_; }
    [[nodiscard]] const std::string& as_string() const noexcept { return string_; }
    [[nodiscard]] const std::vector<ConfigValue>& as_array() const noexcept { return *array_; }
    [[nodiscard]] const std::map<std::string, ConfigValue>& as_object() const noexcept { return *object_; }

    // Unchecked scalar accessors. These do NOT perform runtime type validation:
    // calling an accessor whose stored type does not match silently returns a
    // default-initialized value (false, 0, 0.0, or "") with no error signal.
    // Callers must verify the stored type with is_*() before using these
    // accessors. When the stored type is uncertain, prefer the try_as_*()
    // accessors below, which return std::nullopt / nullptr on type mismatch.

    // Safe, nullable accessors. Return a populated value only when the stored
    // type matches; otherwise return std::nullopt / nullptr. Never dereference
    // a null shared_ptr and never throw.
    [[nodiscard]] std::optional<bool> try_as_bool() const noexcept {
        return is_bool() ? std::optional<bool>(bool_) : std::nullopt;
    }
    [[nodiscard]] std::optional<int64_t> try_as_int() const noexcept {
        return is_int() ? std::optional<int64_t>(int_) : std::nullopt;
    }
    [[nodiscard]] std::optional<double> try_as_double() const noexcept {
        return is_double() ? std::optional<double>(double_) : std::nullopt;
    }
    [[nodiscard]] const std::string* try_as_string() const noexcept {
        return is_string() ? &string_ : nullptr;
    }
    [[nodiscard]] const std::vector<ConfigValue>* try_as_array() const noexcept {
        return is_array() ? array_.get() : nullptr;
    }
    [[nodiscard]] const std::map<std::string, ConfigValue>* try_as_object() const noexcept {
        return is_object() ? object_.get() : nullptr;
    }

private:
    Type type_;
    bool bool_ = false;
    int64_t int_ = 0;
    double double_ = 0.0;
    std::string string_;
    std::shared_ptr<std::vector<ConfigValue>> array_;
    std::shared_ptr<std::map<std::string, ConfigValue>> object_;
};

class Config {
public:
    using Map = std::map<std::string, ConfigValue>;

    Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    static Config& instance() noexcept;

    [[nodiscard]] bool load_from_file(std::string_view path);
    [[nodiscard]] bool load_from_string(std::string_view json);

    [[nodiscard]] bool has(std::string_view key) const noexcept;
    // Returns a reference to the stored ConfigValue. The reference refers to
    // internal Config storage; the mutex is released when this accessor
    // returns, so the reference must not be retained across concurrent
    // mutations (set/remove/clear) that erase or clear the referenced entry.
    [[nodiscard]] const ConfigValue& get(std::string_view key) const;
    [[nodiscard]] const ConfigValue& get(std::string_view key, const ConfigValue& default_value) const noexcept;

    template <typename T>
    [[nodiscard]] T get_as(std::string_view key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = values_.find(std::string(key));
        if (it == values_.end()) {
            throw NovaError(ErrorCode::InvalidArgument,
                "Configuration key not found: " + std::string(key));
        }
        return get_as_impl<T>(it->second);
    }

    template <typename T>
    [[nodiscard]] T get_as(std::string_view key, const T& default_value) const noexcept {
        try {
            return get_as<T>(key);
        } catch (const NovaError&) {
            return default_value;
        }
    }

    // Non-throwing lookup alternatives.
    //
    // try_get returns a reference to the stored ConfigValue, or std::nullopt
    // when the key is absent. It never throws and never copies the value.
    //
    // The returned reference refers to internal Config storage. The mutex is
    // released when this accessor returns, so the reference must not be
    // retained across concurrent mutations (set/remove/clear) that erase or
    // clear the referenced entry.
    [[nodiscard]] std::optional<std::reference_wrapper<const ConfigValue>>
    try_get(std::string_view key) const;

    // try_get_as returns the converted value, or std::nullopt when the key is
    // absent or the stored type does not match T. It never uses exceptions as
    // normal control flow. Constructing/copying T may still throw (e.g. bad_alloc).
    template <typename T>
    [[nodiscard]] std::optional<T> try_get_as(std::string_view key) const;

    void set(std::string_view key, ConfigValue value);
    void remove(std::string_view key);
    void clear();

    [[nodiscard]] std::string to_json() const;

private:
    [[nodiscard]] static ConfigValue parse_value(std::string_view& input);
    template <typename T>
    [[nodiscard]] static T get_as_impl(const ConfigValue& v);

    std::map<std::string, ConfigValue> values_;
    mutable std::mutex mutex_;
};

template <>
[[nodiscard]] inline bool Config::get_as_impl<bool>(const ConfigValue& v) {
    if (!v.is_bool()) throw NovaError(ErrorCode::InvalidArgument, "Configuration key type mismatch");
    return v.as_bool();
}

template <>
[[nodiscard]] inline int64_t Config::get_as_impl<int64_t>(const ConfigValue& v) {
    if (!v.is_int()) throw NovaError(ErrorCode::InvalidArgument, "Configuration key type mismatch");
    return v.as_int();
}

template <>
[[nodiscard]] inline double Config::get_as_impl<double>(const ConfigValue& v) {
    if (!v.is_double()) throw NovaError(ErrorCode::InvalidArgument, "Configuration key type mismatch");
    return v.as_double();
}

template <>
[[nodiscard]] inline std::string Config::get_as_impl<std::string>(const ConfigValue& v) {
    if (!v.is_string()) throw NovaError(ErrorCode::InvalidArgument, "Configuration key type mismatch");
    return v.as_string();
}

template <typename T>
[[nodiscard]] std::optional<T> Config::try_get_as(std::string_view key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = values_.find(std::string(key));
    if (it == values_.end()) {
        return std::nullopt;
    }
    const ConfigValue& v = it->second;
    if constexpr (std::is_same_v<T, bool>) {
        if (!v.is_bool()) return std::nullopt;
        return v.as_bool();
    } else if constexpr (std::is_same_v<T, int64_t>) {
        if (!v.is_int()) return std::nullopt;
        return v.as_int();
    } else if constexpr (std::is_same_v<T, double>) {
        if (!v.is_double()) return std::nullopt;
        return v.as_double();
    } else if constexpr (std::is_same_v<T, std::string>) {
        if (!v.is_string()) return std::nullopt;
        return v.as_string();
    } else {
        return std::nullopt;
    }
}

}  // namespace nova::core
