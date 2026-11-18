#include "nova/core/config.h"

#include <gtest/gtest.h>

#include <thread>
#include <vector>
#include <atomic>

TEST(ConfigTest, SetAndGetString) {
    nova::core::Config& config = nova::core::Config::instance();
    config.set("name", std::string("NOVA"));
    EXPECT_TRUE(config.has("name"));
    EXPECT_EQ(std::string("NOVA"), config.get_as<std::string>("name"));
}

TEST(ConfigTest, SetAndGetBool) {
    nova::core::Config& config = nova::core::Config::instance();
    config.set("enabled", true);
    EXPECT_TRUE(config.get_as<bool>("enabled"));
}

TEST(ConfigTest, SetAndGetInt) {
    nova::core::Config& config = nova::core::Config::instance();
    config.set("port", int64_t(8080));
    EXPECT_EQ(int64_t(8080), config.get_as<int64_t>("port"));
}

TEST(ConfigTest, SetAndGetDouble) {
    nova::core::Config& config = nova::core::Config::instance();
    config.set("scale", 1.5);
    EXPECT_DOUBLE_EQ(1.5, config.get_as<double>("scale"));
}

TEST(ConfigTest, GetWithDefault) {
    nova::core::Config& config = nova::core::Config::instance();
    EXPECT_EQ(std::string("fallback"),
        config.get_as<std::string>("missing", std::string("fallback")));
}

TEST(ConfigTest, HasMissingKey) {
    nova::core::Config& config = nova::core::Config::instance();
    EXPECT_FALSE(config.has("does.not.exist"));
}

TEST(ConfigTest, RemoveKey) {
    nova::core::Config& config = nova::core::Config::instance();
    config.set("temp", std::string("x"));
    EXPECT_TRUE(config.has("temp"));
    config.remove("temp");
    EXPECT_FALSE(config.has("temp"));
}

TEST(ConfigTest, Clear) {
    nova::core::Config& config = nova::core::Config::instance();
    config.set("a", std::string("1"));
    config.clear();
    EXPECT_FALSE(config.has("a"));
}

TEST(ConfigTest, LoadFromString) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    bool ok = config.load_from_string(R"({"name": "NOVA", "port": 8080, "debug": true})");
    EXPECT_TRUE(ok);
    EXPECT_EQ(std::string("NOVA"), config.get_as<std::string>("name"));
    EXPECT_EQ(int64_t(8080), config.get_as<int64_t>("port"));
    EXPECT_TRUE(config.get_as<bool>("debug"));
}

TEST(ConfigTest, LoadFromStringInvalid) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_FALSE(config.load_from_string("not json"));
}

TEST(ConfigTest, LoadFromStringTrailingGarbageRejected) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_FALSE(config.load_from_string(R"({"a": 1} extra)"));
    EXPECT_FALSE(config.load_from_string(R"({} {})"));
    EXPECT_FALSE(config.load_from_string(R"({"a": 1} 123)"));
}

TEST(ConfigTest, LoadFromStringUnquotedKeyRejected) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_FALSE(config.load_from_string(R"({a: 1})"));
    EXPECT_FALSE(config.load_from_string(R"({a: "b"})"));
}

TEST(ConfigTest, LoadFromStringMissingColonRejected) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_FALSE(config.load_from_string(R"({"a" 1})"));
    EXPECT_FALSE(config.load_from_string(R"({"a": 1 "b": 2})"));
}

TEST(ConfigTest, LoadFromStringMissingCommaRejected) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_FALSE(config.load_from_string(R"({"a": 1 "b": 2})"));
}

TEST(ConfigTest, LoadFromStringTrailingCommaRejected) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_FALSE(config.load_from_string(R"({"a": 1,})"));
    EXPECT_FALSE(config.load_from_string(R"({"a": [1,]})"));
}

TEST(ConfigTest, LoadFromStringUnterminatedRejected) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_FALSE(config.load_from_string(R"({"a": 1)"));
    EXPECT_FALSE(config.load_from_string(R"({"a": [1)"));
    EXPECT_FALSE(config.load_from_string(R"({"a": "abc)"));
}

TEST(ConfigTest, LoadFromStringInvalidEscapeRejected) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_FALSE(config.load_from_string(R"({"a": "\x"})"));
    EXPECT_FALSE(config.load_from_string(R"({"a": "\q"})"));
}

TEST(ConfigTest, LoadFromStringUnescapedControlRejected) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_FALSE(config.load_from_string(R"({"a": "a
b"})"));
}

TEST(ConfigTest, LoadFromStringIncompleteUnicodeRejected) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_FALSE(config.load_from_string(R"({"a": "\u}")"));
    EXPECT_FALSE(config.load_from_string(R"({"a": "\u00}")"));
    EXPECT_FALSE(config.load_from_string(R"({"a": "\u00GG}")"));
}

TEST(ConfigTest, LoadFromStringValidUnicodeEscapeAccepted) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_TRUE(config.load_from_string(R"({"a": "\u0041"})"));
    EXPECT_EQ(config.get_as<std::string>("a"), "A");
}

TEST(ConfigTest, LoadFromStringValidSurrogatePairAccepted) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_TRUE(config.load_from_string(R"({"a": "\uD83D\uDE00"})"));
    EXPECT_FALSE(config.load_from_string(R"({"a": "\uD800"})"));
    EXPECT_FALSE(config.load_from_string(R"({"a": "\uDC00"})"));
}

TEST(ConfigTest, LoadFromStringLeadingZeroRejected) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_FALSE(config.load_from_string(R"({"a": 01})"));
    EXPECT_FALSE(config.load_from_string(R"({"a": 007})"));
}

TEST(ConfigTest, LoadFromStringDecimalRejected) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_FALSE(config.load_from_string(R"({"a": .5})"));
    EXPECT_FALSE(config.load_from_string(R"({"a": 1.})"));
    EXPECT_FALSE(config.load_from_string(R"({"a": 1.2.3})"));
}

TEST(ConfigTest, LoadFromStringExponentRejected) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_FALSE(config.load_from_string(R"({"a": 1e})"));
    EXPECT_FALSE(config.load_from_string(R"({"a": 1e+})"));
    EXPECT_FALSE(config.load_from_string(R"({"a": 1e-})"));
    EXPECT_FALSE(config.load_from_string(R"({"a": 1.5e}")"));
}

TEST(ConfigTest, LoadFromStringPlusAndMinusRejected) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_FALSE(config.load_from_string(R"({"a": +1})"));
    EXPECT_FALSE(config.load_from_string(R"({"a": -})"));
}

TEST(ConfigTest, LoadFromStringNumericOverflowRejected) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_FALSE(config.load_from_string(R"({"a": 1e999})"));
    EXPECT_FALSE(config.load_from_string(R"({"a": 99999999999999999999999})"));
}

TEST(ConfigTest, LoadFromStringDuplicateKeysRejected) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_FALSE(config.load_from_string(R"({"a": 1, "a": 2})"));
}

TEST(ConfigTest, LoadFromStringValidJsonAccepted) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_TRUE(config.load_from_string(R"({"name": "NOVA", "port": 8080, "debug": true})"));
    EXPECT_EQ(config.get_as<std::string>("name"), "NOVA");
    EXPECT_EQ(config.get_as<int64_t>("port"), 8080);
    EXPECT_TRUE(config.get_as<bool>("debug"));
}

TEST(ConfigTest, LoadFromStringWhitespaceAccepted) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_TRUE(config.load_from_string(R"( { "a" : 1 } )"));
    EXPECT_EQ(config.get_as<int64_t>("a"), 1);
}

TEST(ConfigTest, LoadFromStringEmptyContainersAccepted) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_TRUE(config.load_from_string(R"({})"));
    EXPECT_TRUE(config.load_from_string(R"({"a": []})"));
    EXPECT_TRUE(config.load_from_string(R"({"a": {}, "b": {}})"));
}

TEST(ConfigTest, LoadFromStringValidLiterals) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_TRUE(config.load_from_string(R"({"a": true, "b": false, "c": null})"));
    EXPECT_TRUE(config.get_as<bool>("a"));
    EXPECT_FALSE(config.get_as<bool>("b"));
    EXPECT_TRUE(config.get("c").is_null());
}

TEST(ConfigTest, LoadFromStringStatePreservedOnFailure) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    EXPECT_TRUE(config.load_from_string(R"({"name": "NOVA", "port": 8080})"));
    EXPECT_EQ(config.get_as<std::string>("name"), "NOVA");
    EXPECT_EQ(config.get_as<int64_t>("port"), 8080);

    EXPECT_FALSE(config.load_from_string(R"({"port": )"));
    EXPECT_FALSE(config.load_from_string(R"({"a": 1} extra)"));
    EXPECT_FALSE(config.load_from_string(R"({a: 1})"));

    EXPECT_EQ(config.get_as<std::string>("name"), "NOVA");
    EXPECT_EQ(config.get_as<int64_t>("port"), 8080);
}

TEST(ConfigTest, LoadFromStringNoExceptionOnMalformedNumber) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    bool r1 = config.load_from_string(R"({"a": 1.2.3})");
    bool r2 = config.load_from_string(R"({"a": -})");
    bool r3 = config.load_from_string(R"({"a": 1e999})");
    bool r4 = config.load_from_string(R"({"a": 01})");
    (void)r1; (void)r2; (void)r3; (void)r4;
    EXPECT_FALSE(config.load_from_string(R"({"a": 1.2.3})"));
    EXPECT_FALSE(config.load_from_string(R"({"a": -})"));
}

TEST(ConfigValueTest, TryAsMatchingScalarTypes) {
    nova::core::ConfigValue bool_v(true);
    nova::core::ConfigValue int_v(int64_t(42));
    nova::core::ConfigValue double_v(1.5);
    nova::core::ConfigValue string_v(std::string("hello"));

    EXPECT_TRUE(bool_v.try_as_bool().has_value());
    EXPECT_EQ(*bool_v.try_as_bool(), true);
    EXPECT_TRUE(int_v.try_as_int().has_value());
    EXPECT_EQ(*int_v.try_as_int(), int64_t(42));
    EXPECT_TRUE(double_v.try_as_double().has_value());
    EXPECT_DOUBLE_EQ(*double_v.try_as_double(), 1.5);
    EXPECT_TRUE(string_v.try_as_string() != nullptr);
    EXPECT_EQ(*string_v.try_as_string(), "hello");
}

TEST(ConfigValueTest, TryAsMismatchedScalarTypes) {
    nova::core::ConfigValue int_v(int64_t(42));
    EXPECT_FALSE(int_v.try_as_bool().has_value());
    EXPECT_FALSE(int_v.try_as_double().has_value());
    EXPECT_FALSE(int_v.try_as_string() != nullptr);
}

TEST(ConfigValueTest, TryAsMatchingContainerTypes) {
    nova::core::ConfigValue array_v(std::vector<nova::core::ConfigValue>{});
    nova::core::ConfigValue object_v(std::map<std::string, nova::core::ConfigValue>{});

    EXPECT_TRUE(array_v.try_as_array() != nullptr);
    EXPECT_TRUE(array_v.try_as_array()->empty());
    EXPECT_TRUE(object_v.try_as_object() != nullptr);
    EXPECT_TRUE(object_v.try_as_object()->empty());
}

TEST(ConfigValueTest, TryAsMismatchedContainerTypes) {
    nova::core::ConfigValue int_v(int64_t(42));
    EXPECT_EQ(int_v.try_as_array(), nullptr);
    EXPECT_EQ(int_v.try_as_object(), nullptr);
}

TEST(ConfigValueTest, TryAsDefaultConstructedIsNull) {
    nova::core::ConfigValue null_v;
    EXPECT_FALSE(null_v.try_as_bool().has_value());
    EXPECT_FALSE(null_v.try_as_int().has_value());
    EXPECT_FALSE(null_v.try_as_double().has_value());
    EXPECT_EQ(null_v.try_as_string(), nullptr);
    EXPECT_EQ(null_v.try_as_array(), nullptr);
    EXPECT_EQ(null_v.try_as_object(), nullptr);
}

TEST(ConfigValueTest, ExistingAsAccessorsUnchanged) {
    nova::core::ConfigValue int_v(int64_t(7));
    nova::core::ConfigValue str_v(std::string("x"));
    EXPECT_EQ(int_v.as_int(), int64_t(7));
    EXPECT_EQ(str_v.as_string(), "x");
}

TEST(ConfigTest, TryGetExistingKeyReturnsReference) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    config.set("name", std::string("NOVA"));

    auto ref = config.try_get("name");
    ASSERT_TRUE(ref.has_value());
    EXPECT_EQ(ref->get().as_string(), std::string("NOVA"));
}

TEST(ConfigTest, TryGetMissingKeyReturnsNullopt) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();

    EXPECT_FALSE(config.try_get("missing").has_value());
}

TEST(ConfigTest, TryGetReturnsReferenceNotCopy) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    config.set("counter", int64_t(42));

    auto ref = config.try_get("counter");
    ASSERT_TRUE(ref.has_value());
    EXPECT_EQ(ref->get().as_int(), int64_t(42));
}

TEST(ConfigTest, TryGetEmptyKeyReturnsNullopt) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    config.set("a", std::string("1"));

    EXPECT_FALSE(config.try_get("").has_value());
}

TEST(ConfigTest, TryGetAsMatchingTypes) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    config.set("flag", true);
    config.set("port", int64_t(8080));
    config.set("scale", 1.5);
    config.set("name", std::string("NOVA"));

    auto b = config.try_get_as<bool>("flag");
    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(*b, true);

    auto i = config.try_get_as<int64_t>("port");
    ASSERT_TRUE(i.has_value());
    EXPECT_EQ(*i, int64_t(8080));

    auto d = config.try_get_as<double>("scale");
    ASSERT_TRUE(d.has_value());
    EXPECT_DOUBLE_EQ(*d, 1.5);

    auto s = config.try_get_as<std::string>("name");
    ASSERT_TRUE(s.has_value());
    EXPECT_EQ(*s, std::string("NOVA"));
}

TEST(ConfigTest, TryGetAsMissingKeyReturnsNullopt) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();

    EXPECT_FALSE(config.try_get_as<bool>("missing").has_value());
    EXPECT_FALSE(config.try_get_as<int64_t>("missing").has_value());
    EXPECT_FALSE(config.try_get_as<double>("missing").has_value());
    EXPECT_FALSE(config.try_get_as<std::string>("missing").has_value());
}

TEST(ConfigTest, TryGetAsTypeMismatchReturnsNullopt) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    config.set("value", std::string("42"));

    EXPECT_FALSE(config.try_get_as<bool>("value").has_value());
    EXPECT_FALSE(config.try_get_as<int64_t>("value").has_value());
    EXPECT_FALSE(config.try_get_as<double>("value").has_value());
}

TEST(ConfigTest, TryGetAsDoesNotMutateConfig) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    config.set("name", std::string("NOVA"));

    (void)config.try_get_as<std::string>("name");
    (void)config.try_get_as<int64_t>("name");

    EXPECT_TRUE(config.has("name"));
    EXPECT_EQ(config.get_as<std::string>("name"), std::string("NOVA"));
}

TEST(ConfigTest, ExistingGetStillThrowsOnMissingKey) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    bool threw = false;
    try {
        auto v = config.get("missing");
        (void)v;
    } catch (const nova::core::NovaError&) {
        threw = true;
    }
    EXPECT_TRUE(threw);
}

TEST(ConfigTest, ExistingGetAsStillThrowsOnMissingKey) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    bool threw = false;
    try {
        auto v = config.get_as<int64_t>("missing");
        (void)v;
    } catch (const nova::core::NovaError&) {
        threw = true;
    }
    EXPECT_TRUE(threw);
}

TEST(ConfigTest, ExistingGetAsStillThrowsOnTypeMismatch) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    config.set("value", std::string("42"));
    bool threw = false;
    try {
        auto v = config.get_as<int64_t>("value");
        (void)v;
    } catch (const nova::core::NovaError&) {
        threw = true;
    }
    EXPECT_TRUE(threw);
}

TEST(ConfigTest, ExistingDefaultValueOverloadsUnchanged) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();

    EXPECT_EQ(config.get_as<std::string>("missing", std::string("fb")),
              std::string("fb"));
    EXPECT_EQ(config.get_as<int64_t>("missing", int64_t(7)), int64_t(7));
}

TEST(ConfigTest, ConcurrentGetAsAndTryGetAsWhileMutating) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();
    config.set("name", std::string("NOVA"));
    config.set("port", int64_t(8080));
    config.set("scale", 1.5);
    config.set("flag", true);

    std::atomic<bool> stop{false};
    std::atomic<int> errors{0};

    auto reader = [&]() {
        while (!stop.load(std::memory_order_acquire)) {
            try {
                if (config.get_as<std::string>("name") != std::string("NOVA")) {
                    errors.fetch_add(1, std::memory_order_relaxed);
                }
                if (config.get_as<int64_t>("port") != int64_t(8080)) {
                    errors.fetch_add(1, std::memory_order_relaxed);
                }
                if (config.get_as<double>("scale") != 1.5) {
                    errors.fetch_add(1, std::memory_order_relaxed);
                }
                if (config.get_as<bool>("flag") != true) {
                    errors.fetch_add(1, std::memory_order_relaxed);
                }
            } catch (const nova::core::NovaError&) {
                // A concurrent mutation may remove the key; treat as benign.
            }
            auto b = config.try_get_as<bool>("flag");
            auto i = config.try_get_as<int64_t>("port");
            auto d = config.try_get_as<double>("scale");
            auto s = config.try_get_as<std::string>("name");
            if (b.has_value() && *b != true) errors.fetch_add(1, std::memory_order_relaxed);
            if (i.has_value() && *i != int64_t(8080)) errors.fetch_add(1, std::memory_order_relaxed);
            if (d.has_value() && *d != 1.5) errors.fetch_add(1, std::memory_order_relaxed);
            if (s.has_value() && *s != std::string("NOVA")) errors.fetch_add(1, std::memory_order_relaxed);
            (void)config.try_get_as<int64_t>("name");
            (void)config.try_get_as<std::string>("port");
        }
    };

    auto writer = [&]() {
        for (int iteration = 0; iteration < 200 && !stop.load(std::memory_order_acquire); ++iteration) {
            config.set("name", std::string("NOVA"));
            config.set("port", int64_t(8080));
            config.set("scale", 1.5);
            config.set("flag", true);
            config.remove("scale");
            config.set("scale", 1.5);
        }
    };

    std::vector<std::thread> readers;
    for (int i = 0; i < 4; ++i) readers.emplace_back(reader);
    std::vector<std::thread> writers;
    for (int i = 0; i < 2; ++i) writers.emplace_back(writer);

    for (auto& t : writers) t.join();
    stop.store(true, std::memory_order_release);
    for (auto& t : readers) t.join();

    EXPECT_EQ(errors.load(std::memory_order_acquire), 0);
}

TEST(ConfigTest, ConcurrentReadsDoNotCrash) {
    nova::core::Config& config = nova::core::Config::instance();
    config.clear();

    std::atomic<bool> stop{false};

    auto mutator = [&]() {
        for (int i = 0; i < 500 && !stop.load(std::memory_order_acquire); ++i) {
            config.set("k", std::string("v"));
            config.remove("k");
            config.clear();
            config.set("k", std::string("v"));
        }
    };

    auto reader = [&]() {
        while (!stop.load(std::memory_order_acquire)) {
            (void)config.try_get("k");
            (void)config.try_get_as<std::string>("k");
            (void)config.has("k");
        }
    };

    std::thread mutator_thread(mutator);
    std::vector<std::thread> readers;
    for (int i = 0; i < 4; ++i) readers.emplace_back(reader);

    mutator_thread.join();
    stop.store(true, std::memory_order_release);
    for (auto& t : readers) t.join();
}

