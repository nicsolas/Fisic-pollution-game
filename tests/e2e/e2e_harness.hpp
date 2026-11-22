#pragma once

#include <cmath>
#include <cstddef>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace eco::e2e {

class IE2EAdapter;

enum class E2ETier {
    Tier1_Feature = 1,
    Tier2_Boundary = 2,
    Tier3_Pairwise = 3,
    Tier4_Scenario = 4
};

[[nodiscard]] inline const char* TierToString(E2ETier tier) noexcept {
    switch (tier) {
        case E2ETier::Tier1_Feature:
            return "Tier1_Feature";
        case E2ETier::Tier2_Boundary:
            return "Tier2_Boundary";
        case E2ETier::Tier3_Pairwise:
            return "Tier3_Pairwise";
        case E2ETier::Tier4_Scenario:
            return "Tier4_Scenario";
    }
    return "Unknown";
}

struct AssertionFailure : std::exception {
    std::string file;
    int line = 0;
    std::string message;

    AssertionFailure(std::string f, int l, std::string m)
        : file(std::move(f)), line(l), message(std::move(m)) {}

    [[nodiscard]] const char* what() const noexcept override {
        return message.c_str();
    }
};

struct ContractOnlyException : std::exception {
    std::string reason;

    explicit ContractOnlyException(std::string r) : reason(std::move(r)) {}

    [[nodiscard]] const char* what() const noexcept override {
        return reason.c_str();
    }
};

struct TestCase {
    using Fn = void (*)(IE2EAdapter&);
    std::string name;
    E2ETier tier = E2ETier::Tier1_Feature;
    std::vector<std::string> features;
    Fn fn = nullptr;
};

[[nodiscard]] inline std::vector<std::string> ParseFeaturesCsv(std::string_view csv) {
    std::vector<std::string> out;
    std::size_t start = 0;
    while (start <= csv.size()) {
        const std::size_t comma = csv.find(',', start);
        const std::size_t end = (comma == std::string_view::npos) ? csv.size() : comma;
        std::string_view token = csv.substr(start, end - start);
        while (!token.empty() && (token.front() == ' ' || token.front() == '\t')) {
            token.remove_prefix(1);
        }
        while (!token.empty() && (token.back() == ' ' || token.back() == '\t')) {
            token.remove_suffix(1);
        }
        if (!token.empty()) {
            out.emplace_back(token);
        }
        if (comma == std::string_view::npos) {
            break;
        }
        start = comma + 1;
    }
    return out;
}

class TestRegistry {
public:
    static TestRegistry& Instance() {
        static TestRegistry instance;
        return instance;
    }

    void Register(std::string name, E2ETier tier, std::string_view features_csv, TestCase::Fn fn) {
        TestCase tc;
        tc.name = std::move(name);
        tc.tier = tier;
        tc.features = ParseFeaturesCsv(features_csv);
        tc.fn = fn;
        tests_.push_back(std::move(tc));
    }

    [[nodiscard]] const std::vector<TestCase>& Tests() const noexcept {
        return tests_;
    }

private:
    TestRegistry() = default;
    std::vector<TestCase> tests_;
};

class TestRegistrar {
public:
    TestRegistrar(const char* name, E2ETier tier, const char* features_csv, TestCase::Fn fn) {
        TestRegistry::Instance().Register(name, tier, features_csv, fn);
    }
};

template <typename T>
[[nodiscard]] inline std::string FormatAssertionValue(const T& val) {
    std::ostringstream oss;
    if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
        oss << (val ? "true" : "false");
    } else if constexpr (std::is_enum_v<std::decay_t<T>>) {
        oss << static_cast<std::underlying_type_t<std::decay_t<T>>>(val);
    } else if constexpr (requires(std::ostream& os, const T& v) { os << v; }) {
        oss << val;
    } else {
        oss << "<value>";
    }
    return oss.str();
}

inline void RequireLiveBackend(const char* file, int line, bool is_live_backend) {
    if (!is_live_backend) {
        std::ostringstream oss;
        oss << file << ":" << line << " requires live game_core/eco_proto backend";
        throw ContractOnlyException(oss.str());
    }
}

} // namespace eco::e2e

#define E2E_TEST(name, tier, features_csv)                                            \
    static void E2E_TestFn_##name([[maybe_unused]] ::eco::e2e::IE2EAdapter& adapter); \
    static const ::eco::e2e::TestRegistrar E2E_Reg_##name(                            \
        #name, (tier), (features_csv), &E2E_TestFn_##name);                           \
    static void E2E_TestFn_##name([[maybe_unused]] ::eco::e2e::IE2EAdapter& adapter)

#define E2E_REQUIRE_LIVE(adapter_ref) \
    ::eco::e2e::RequireLiveBackend(__FILE__, __LINE__, (adapter_ref).IsLiveBackend())

#define E2E_ASSERT_TRUE(cond)                                                          \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            throw ::eco::e2e::AssertionFailure(                                        \
                __FILE__, __LINE__, std::string("E2E_ASSERT_TRUE failed: ") + #cond);  \
        }                                                                              \
    } while (false)

#define E2E_ASSERT_EQ(a, b)                                                               \
    do {                                                                                  \
        const auto& e2e_lhs_ = (a);                                                       \
        const auto& e2e_rhs_ = (b);                                                       \
        if (!(e2e_lhs_ == e2e_rhs_)) {                                                    \
            std::ostringstream e2e_oss_;                                                  \
            e2e_oss_ << "E2E_ASSERT_EQ failed: (" << #a << ") == (" << #b << ") | got "   \
                     << ::eco::e2e::FormatAssertionValue(e2e_lhs_) << " vs "              \
                     << ::eco::e2e::FormatAssertionValue(e2e_rhs_);                       \
            throw ::eco::e2e::AssertionFailure(__FILE__, __LINE__, e2e_oss_.str());       \
        }                                                                                 \
    } while (false)

#define E2E_ASSERT_NEAR(a, b, eps)                                                            \
    do {                                                                                      \
        const double e2e_lhs_ = static_cast<double>(a);                                       \
        const double e2e_rhs_ = static_cast<double>(b);                                       \
        const double e2e_eps_ = static_cast<double>(eps);                                     \
        if (!std::isfinite(e2e_lhs_) || !std::isfinite(e2e_rhs_) ||                           \
            !std::isfinite(e2e_eps_) || std::fabs(e2e_lhs_ - e2e_rhs_) > e2e_eps_) {          \
            std::ostringstream e2e_oss_;                                                      \
            e2e_oss_ << "E2E_ASSERT_NEAR failed: |" << #a << " - " << #b << "| <= " << #eps   \
                     << " | got " << e2e_lhs_ << " vs " << e2e_rhs_ << " (eps=" << e2e_eps_   \
                     << ")";                                                                  \
            throw ::eco::e2e::AssertionFailure(__FILE__, __LINE__, e2e_oss_.str());           \
        }                                                                                     \
    } while (false)

#define E2E_ASSERT_GE(a, b)                                                               \
    do {                                                                                  \
        const auto& e2e_lhs_ = (a);                                                       \
        const auto& e2e_rhs_ = (b);                                                       \
        if (!(e2e_lhs_ >= e2e_rhs_)) {                                                    \
            std::ostringstream e2e_oss_;                                                  \
            e2e_oss_ << "E2E_ASSERT_GE failed: (" << #a << ") >= (" << #b << ") | got "   \
                     << ::eco::e2e::FormatAssertionValue(e2e_lhs_) << " vs "              \
                     << ::eco::e2e::FormatAssertionValue(e2e_rhs_);                       \
            throw ::eco::e2e::AssertionFailure(__FILE__, __LINE__, e2e_oss_.str());       \
        }                                                                                 \
    } while (false)

#define E2E_ASSERT_LE(a, b)                                                               \
    do {                                                                                  \
        const auto& e2e_lhs_ = (a);                                                       \
        const auto& e2e_rhs_ = (b);                                                       \
        if (!(e2e_lhs_ <= e2e_rhs_)) {                                                    \
            std::ostringstream e2e_oss_;                                                  \
            e2e_oss_ << "E2E_ASSERT_LE failed: (" << #a << ") <= (" << #b << ") | got "   \
                     << ::eco::e2e::FormatAssertionValue(e2e_lhs_) << " vs "              \
                     << ::eco::e2e::FormatAssertionValue(e2e_rhs_);                       \
            throw ::eco::e2e::AssertionFailure(__FILE__, __LINE__, e2e_oss_.str());       \
        }                                                                                 \
    } while (false)
