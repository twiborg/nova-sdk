#include "nova/core/config.h"

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <sstream>

namespace nova::core {

namespace {

[[nodiscard]] bool parse_value_impl(std::string_view& input, ConfigValue& out) noexcept;
[[nodiscard]] bool parse_object_impl(std::string_view& input, ConfigValue& out) noexcept;
[[nodiscard]] bool parse_array_impl(std::string_view& input, ConfigValue& out) noexcept;
bool consume(std::string_view& input, char expected) noexcept;
void skip_ws(std::string_view& input) noexcept;
[[nodiscard]] bool read_string(std::string_view& input, std::string& out) noexcept;
[[nodiscard]] bool read_literal(std::string_view& input, std::string_view literal) noexcept;
[[nodiscard]] bool read_number(std::string_view& input, double& out, bool& is_integer) noexcept;
void append_utf8(std::string& out, uint32_t cp) noexcept;

}  // namespace

Config& Config::instance() noexcept {
    static Config config;
    return config;
}

bool Config::load_from_file(std::string_view path) {
    std::ifstream file{std::string(path)};
    if (!file.is_open()) {
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return load_from_string(buffer.str());
}

bool Config::load_from_string(std::string_view json) {
    std::string_view input = json;
    skip_ws(input);
    if (input.empty() || input.front() != '{') {
        return false;
    }
    ConfigValue value;
    if (!parse_value_impl(input, value)) {
        return false;
    }
    if (!value.is_object()) {
        return false;
    }
    skip_ws(input);
    if (!input.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    values_ = value.as_object();
    return true;
}

bool Config::has(std::string_view key) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return values_.find(std::string(key)) != values_.end();
}

const ConfigValue& Config::get(std::string_view key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = values_.find(std::string(key));
    if (it == values_.end()) {
        throw NovaError(ErrorCode::InvalidArgument,
            "Configuration key not found: " + std::string(key));
    }
    return it->second;
}

const ConfigValue& Config::get(std::string_view key, const ConfigValue& default_value) const noexcept {
    try {
        return get(key);
    } catch (const NovaError&) {
        return default_value;
    }
}

std::optional<std::reference_wrapper<const ConfigValue>> Config::try_get(std::string_view key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = values_.find(std::string(key));
    if (it == values_.end()) {
        return std::nullopt;
    }
    return std::cref(it->second);
}

void Config::set(std::string_view key, ConfigValue value) {
    std::lock_guard<std::mutex> lock(mutex_);
    values_[std::string(key)] = std::move(value);
}

void Config::remove(std::string_view key) {
    std::lock_guard<std::mutex> lock(mutex_);
    values_.erase(std::string(key));
}

void Config::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    values_.clear();
}

namespace {

bool is_json_ws(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

void skip_ws(std::string_view& input) noexcept {
    while (!input.empty() && is_json_ws(input.front())) {
        input.remove_prefix(1);
    }
}

void append_utf8(std::string& out, uint32_t cp) noexcept {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

[[nodiscard]] bool consume(std::string_view& input, char expected) noexcept {
    if (input.empty() || input.front() != expected) {
        return false;
    }
    input.remove_prefix(1);
    return true;
}

[[nodiscard]] bool read_string(std::string_view& input, std::string& out) noexcept {
    if (!consume(input, '"')) {
        return false;
    }
    std::string result;
    while (!input.empty()) {
        char c = input.front();
        if (c == '"') {
            input.remove_prefix(1);
            out = std::move(result);
            return true;
        }
        if (static_cast<unsigned char>(c) < 0x20) {
            return false;
        }
        if (c == '\\') {
            input.remove_prefix(1);
            if (input.empty()) {
                return false;
            }
            char esc = input.front();
            switch (esc) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'u': {
                    input.remove_prefix(1);
                    if (input.size() < 4) {
                        return false;
                    }
                    uint32_t cp = 0;
                    for (int i = 0; i < 4; ++i) {
                        char h = input.front();
                        int digit = 0;
                        if (h >= '0' && h <= '9') {
                            digit = h - '0';
                        } else if (h >= 'a' && h <= 'f') {
                            digit = h - 'a' + 10;
                        } else if (h >= 'A' && h <= 'F') {
                            digit = h - 'A' + 10;
                        } else {
                            return false;
                        }
                        cp = (cp << 4) | static_cast<uint32_t>(digit);
                        input.remove_prefix(1);
                    }
                    if (cp >= 0xD800 && cp <= 0xDBFF) {
                        if (input.size() < 6 || input.front() != '\\' ||
                            input[1] != 'u') {
                            return false;
                        }
                        input.remove_prefix(2);
                        uint32_t low = 0;
                        for (int i = 0; i < 4; ++i) {
                            char h = input.front();
                            int digit = 0;
                            if (h >= '0' && h <= '9') {
                                digit = h - '0';
                            } else if (h >= 'a' && h <= 'f') {
                                digit = h - 'a' + 10;
                            } else if (h >= 'A' && h <= 'F') {
                                digit = h - 'A' + 10;
                            } else {
                                return false;
                            }
                            low = (low << 4) | static_cast<uint32_t>(digit);
                            input.remove_prefix(1);
                        }
                        if (low < 0xDC00 || low > 0xDFFF) {
                            return false;
                        }
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                    } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                        return false;
                    }
                    append_utf8(result, cp);
                    continue;
                }
                default:
                    return false;
            }
            input.remove_prefix(1);
        } else {
            result += c;
            input.remove_prefix(1);
        }
    }
    return false;
}

[[nodiscard]] bool read_literal(std::string_view& input, std::string_view literal) noexcept {
    if (input.size() >= literal.size() && input.compare(0, literal.size(), literal) == 0) {
        input.remove_prefix(literal.size());
        return true;
    }
    return false;
}

[[nodiscard]] bool read_number(std::string_view& input, double& out, bool& is_integer) noexcept {
    std::string_view token = input;
    size_t i = 0;
    bool is_float = false;
    if (i < token.size() && (token[i] == '-' || token[i] == '+')) {
        if (token[i] == '+') {
            return false;
        }
        ++i;
    }
    if (i >= token.size()) {
        return false;
    }
    if (token[i] == '0') {
        ++i;
    } else if (token[i] >= '1' && token[i] <= '9') {
        ++i;
        while (i < token.size() && token[i] >= '0' && token[i] <= '9') {
            ++i;
        }
    } else {
        return false;
    }
    if (i < token.size() && token[i] == '.') {
        is_float = true;
        ++i;
        if (i >= token.size() || token[i] < '0' || token[i] > '9') {
            return false;
        }
        while (i < token.size() && token[i] >= '0' && token[i] <= '9') {
            ++i;
        }
    }
    if (i < token.size() && (token[i] == 'e' || token[i] == 'E')) {
        is_float = true;
        ++i;
        if (i < token.size() && (token[i] == '+' || token[i] == '-')) {
            ++i;
        }
        if (i >= token.size() || token[i] < '0' || token[i] > '9') {
            return false;
        }
        while (i < token.size() && token[i] >= '0' && token[i] <= '9') {
            ++i;
        }
    }
    std::string token_str(token.data(), i);
    errno = 0;
    char* end = nullptr;
    double val = std::strtod(token_str.c_str(), &end);
    if (errno == ERANGE || end != token_str.c_str() + token_str.size()) {
        return false;
    }
    input.remove_prefix(i);
    out = val;
    is_integer = !is_float;
    return true;
}

std::string escape_json(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

std::string serialize_value(const ConfigValue& v, int indent) {
    std::string pad(indent * 2, ' ');
    std::string inner_pad((indent + 1) * 2, ' ');
    std::string result;
    switch (v.type()) {
        case ConfigValue::Type::Null: result = "null"; break;
        case ConfigValue::Type::Bool: result = v.as_bool() ? "true" : "false"; break;
        case ConfigValue::Type::Int: result = std::to_string(v.as_int()); break;
        case ConfigValue::Type::Double: result = std::to_string(v.as_double()); break;
        case ConfigValue::Type::String: result = "\"" + escape_json(v.as_string()) + "\""; break;
        case ConfigValue::Type::Array: {
            const auto& arr = v.as_array();
            if (arr.empty()) { result = "[]"; break; }
            result = "[\n";
            for (size_t i = 0; i < arr.size(); ++i) {
                result += inner_pad + serialize_value(arr[i], indent + 1);
                if (i + 1 < arr.size()) result += ",";
                result += "\n";
            }
            result += pad + "]";
            break;
        }
        case ConfigValue::Type::Object: {
            const auto& obj = v.as_object();
            if (obj.empty()) { result = "{}"; break; }
            result = "{\n";
            size_t i = 0;
            for (const auto& [k, val] : obj) {
                result += inner_pad + "\"" + escape_json(k) + "\": " + serialize_value(val, indent + 1);
                if (++i < obj.size()) result += ",";
                result += "\n";
            }
            result += pad + "}";
            break;
        }
    }
    return result;
}

}  // namespace

namespace {

[[nodiscard]] bool parse_value_impl(std::string_view& input, ConfigValue& out) noexcept {
    skip_ws(input);
    if (input.empty()) {
        return false;
    }
    char c = input.front();
    if (c == '{') {
        return parse_object_impl(input, out);
    }
    if (c == '[') {
        return parse_array_impl(input, out);
    }
    if (c == '"') {
        std::string s;
        if (!read_string(input, s)) {
            return false;
        }
        out = ConfigValue(std::move(s));
        return true;
    }
    if (c == 't') {
        if (!read_literal(input, "true")) {
            return false;
        }
        out = ConfigValue(true);
        return true;
    }
    if (c == 'f') {
        if (!read_literal(input, "false")) {
            return false;
        }
        out = ConfigValue(false);
        return true;
    }
    if (c == 'n') {
        if (!read_literal(input, "null")) {
            return false;
        }
        out = ConfigValue{};
        return true;
    }
    if (c == '-' || (c >= '0' && c <= '9')) {
        double d = 0;
        bool is_integer = false;
        if (!read_number(input, d, is_integer)) {
            return false;
        }
        if (is_integer &&
            (d < static_cast<double>(std::numeric_limits<int64_t>::min()) ||
             d > static_cast<double>(std::numeric_limits<int64_t>::max()))) {
            return false;
        }
        if (d >= static_cast<double>(std::numeric_limits<int64_t>::min()) &&
            d <= static_cast<double>(std::numeric_limits<int64_t>::max())) {
            int64_t iv = static_cast<int64_t>(d);
            if (static_cast<double>(iv) == d) {
                out = ConfigValue(iv);
                return true;
            }
        }
        out = ConfigValue(d);
        return true;
    }
    return false;
}

[[nodiscard]] bool parse_object_impl(std::string_view& input, ConfigValue& out) noexcept {
    if (!consume(input, '{')) {
        return false;
    }
    std::map<std::string, ConfigValue> obj;
    skip_ws(input);
    if (consume(input, '}')) {
        out = ConfigValue(std::move(obj));
        return true;
    }
    while (true) {
        skip_ws(input);
        if (input.empty()) {
            return false;
        }
        std::string key;
        if (!read_string(input, key)) {
            return false;
        }
        if (obj.find(key) != obj.end()) {
            return false;
        }
        skip_ws(input);
        if (!consume(input, ':')) {
            return false;
        }
        skip_ws(input);
        ConfigValue value;
        if (!parse_value_impl(input, value)) {
            return false;
        }
        obj.emplace(std::move(key), std::move(value));
        skip_ws(input);
        if (consume(input, ',')) {
            continue;
        }
        if (consume(input, '}')) {
            out = ConfigValue(std::move(obj));
            return true;
        }
        return false;
    }
}

[[nodiscard]] bool parse_array_impl(std::string_view& input, ConfigValue& out) noexcept {
    if (!consume(input, '[')) {
        return false;
    }
    std::vector<ConfigValue> arr;
    skip_ws(input);
    if (consume(input, ']')) {
        out = ConfigValue(std::move(arr));
        return true;
    }
    while (true) {
        skip_ws(input);
        if (input.empty()) {
            return false;
        }
        ConfigValue value;
        if (!parse_value_impl(input, value)) {
            return false;
        }
        arr.push_back(std::move(value));
        skip_ws(input);
        if (consume(input, ',')) {
            continue;
        }
        if (consume(input, ']')) {
            out = ConfigValue(std::move(arr));
            return true;
        }
        return false;
    }
}

}  // namespace

ConfigValue Config::parse_value(std::string_view& input) {
    ConfigValue result;
    if (!parse_value_impl(input, result)) {
        return ConfigValue{};
    }
    return result;
}

std::string Config::to_json() const {
    std::lock_guard<std::mutex> lock(mutex_);
    ConfigValue obj(values_);
    return serialize_value(obj, 0);
}

}  // namespace nova::core
