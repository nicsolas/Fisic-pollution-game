#include "e2e_adapter.hpp"
#include "e2e_harness.hpp"
#include "e2e_paths.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::optional<eco::e2e::E2ETier> ParseTierArg(std::string_view value, bool& is_all) {
    if (value == "all") {
        is_all = true;
        return std::nullopt;
    }
    is_all = false;
    if (value == "1") return eco::e2e::E2ETier::Tier1_Feature;
    if (value == "2") return eco::e2e::E2ETier::Tier2_Boundary;
    if (value == "3") return eco::e2e::E2ETier::Tier3_Pairwise;
    if (value == "4") return eco::e2e::E2ETier::Tier4_Scenario;
    return std::nullopt;
}

bool IsValidFeatureArg(std::string_view value) {
    if (value == "all") {
        return true;
    }
    if (value.size() != 3 || value[0] != 'F') {
        return false;
    }
    if (value[1] < '0' || value[1] > '9' || value[2] < '0' || value[2] > '9') {
        return false;
    }
    const int num = (value[1] - '0') * 10 + (value[2] - '0');
    return num >= 1 && num <= 25;
}

bool TestMatchesFilters(
    const eco::e2e::TestCase& tc,
    std::optional<eco::e2e::E2ETier> tier_filter,
    std::string_view feature_filter) {
    if (tier_filter.has_value() && tc.tier != *tier_filter) {
        return false;
    }
    if (feature_filter != "all") {
        bool found = false;
        for (const auto& f : tc.features) {
            if (f == feature_filter) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

std::string JoinFeatures(const std::vector<std::string>& features) {
    std::string out;
    for (std::size_t i = 0; i < features.size(); ++i) {
        if (i > 0) out += ",";
        out += features[i];
    }
    return out;
}

bool RunContractAssertionSelfTest() {
    auto expect_fail = [](auto fn, const char* label) -> bool {
        try {
            fn();
        } catch (const eco::e2e::AssertionFailure&) {
            return true;
        }
        std::cerr << "[CONTRACT-SELF-TEST FAIL] Expected AssertionFailure for " << label << "\n";
        return false;
    };

    bool ok = true;
    ok = ok && expect_fail([]() { E2E_ASSERT_TRUE(false); }, "E2E_ASSERT_TRUE(false)");
    ok = ok && expect_fail([]() { E2E_ASSERT_EQ(1, 2); }, "E2E_ASSERT_EQ(1, 2)");
    ok = ok && expect_fail([]() { E2E_ASSERT_NEAR(1.0f, 1.5f, 0.1f); }, "E2E_ASSERT_NEAR");
    ok = ok && expect_fail(
                   []() {
                       E2E_ASSERT_NEAR(
                           std::numeric_limits<float>::quiet_NaN(), 0.0f, 1.0f);
                   },
                   "E2E_ASSERT_NEAR(NaN)");
    ok = ok && expect_fail([]() { E2E_ASSERT_GE(1, 2); }, "E2E_ASSERT_GE(1, 2)");
    ok = ok && expect_fail([]() { E2E_ASSERT_LE(2, 1); }, "E2E_ASSERT_LE(2, 1)");
    return ok;
}

void PrintUsage(const char* argv0) {
    std::cerr << "Usage: " << argv0
              << " [--tier <1|2|3|4|all>] [--feature <F01..F25|all>]"
              << " [--list] [--summary] [--contract-check]"
              << " [--repo-root <path>] [--build-dir <path>]\n";
}

} // namespace

int main(int argc, char** argv) {
    std::optional<eco::e2e::E2ETier> tier_filter;
    std::string feature_filter = "all";
    bool list_only = false;
    bool summary_only = false;
    bool contract_check_flag = false;
    std::string repo_root = eco::e2e::kDefaultProjectRoot;
    std::string build_dir = eco::e2e::kDefaultGameBuildDir;
    bool build_dir_overridden = false;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (arg == "--tier") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --tier requires an argument.\n";
                return 2;
            }
            bool is_all = false;
            const auto parsed = ParseTierArg(argv[++i], is_all);
            if (!is_all && !parsed.has_value()) {
                std::cerr << "Error: invalid --tier value '" << argv[i]
                          << "' (expected 1|2|3|4|all).\n";
                return 2;
            }
            tier_filter = parsed;
        } else if (arg == "--feature") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --feature requires an argument.\n";
                return 2;
            }
            const std::string_view val(argv[++i]);
            if (!IsValidFeatureArg(val)) {
                std::cerr << "Error: invalid --feature value '" << val
                          << "' (expected F01..F25 or all).\n";
                return 2;
            }
            feature_filter = std::string(val);
        } else if (arg == "--list") {
            list_only = true;
        } else if (arg == "--summary") {
            summary_only = true;
        } else if (arg == "--contract-check") {
            contract_check_flag = true;
        } else if (arg == "--repo-root") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --repo-root requires a path.\n";
                return 2;
            }
            repo_root = argv[++i];
            if (!build_dir_overridden) {
                build_dir = repo_root + "/build";
            }
        } else if (arg == "--build-dir") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --build-dir requires a path.\n";
                return 2;
            }
            build_dir = argv[++i];
            build_dir_overridden = true;
        } else if (arg == "--help" || arg == "-h") {
            PrintUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "Error: unknown CLI option '" << arg << "'.\n";
            PrintUsage(argv[0]);
            return 2;
        }
    }

    const auto& all_tests = eco::e2e::TestRegistry::Instance().Tests();
    std::vector<const eco::e2e::TestCase*> selected;
    for (const auto& tc : all_tests) {
        if (TestMatchesFilters(tc, tier_filter, feature_filter)) {
            selected.push_back(&tc);
        }
    }

    if (list_only) {
        for (const auto* tc : selected) {
            std::cout << tc->name << " | " << eco::e2e::TierToString(tc->tier)
                      << " | " << JoinFeatures(tc->features) << "\n";
        }
    }

    if (summary_only) {
        std::array<int, 5> tier_counts{};
        std::map<std::string, std::array<int, 5>> feature_tier_counts;
        for (const auto* tc : selected) {
            const int t_idx = static_cast<int>(tc->tier);
            if (t_idx >= 1 && t_idx <= 4) {
                ++tier_counts[static_cast<std::size_t>(t_idx)];
            }
            for (const auto& feat : tc->features) {
                auto& row = feature_tier_counts[feat];
                if (t_idx >= 1 && t_idx <= 4) {
                    ++row[static_cast<std::size_t>(t_idx)];
                }
            }
        }

        std::cout << "=== E2E Test Registry Summary ===\n";
        std::cout << "Total matching tests: " << selected.size() << "\n";
        std::cout << "Tier 1 (Feature):  " << tier_counts[1] << "\n";
        std::cout << "Tier 2 (Boundary): " << tier_counts[2] << "\n";
        std::cout << "Tier 3 (Pairwise): " << tier_counts[3] << "\n";
        std::cout << "Tier 4 (Scenario): " << tier_counts[4] << "\n";
        std::cout << "Per-Feature Breakdown (T1 / T2 / T3 / T4):\n";
        for (const auto& [feat, counts] : feature_tier_counts) {
            std::cout << "  " << std::setw(4) << std::left << feat << ": "
                      << counts[1] << " / " << counts[2] << " / "
                      << counts[3] << " / " << counts[4] << "\n";
        }
    }

    if (list_only || summary_only) {
        return selected.empty() ? 1 : 0;
    }

    if (selected.empty()) {
        std::cerr << "Error: CLI filter matched 0 tests.\n";
        return 1;
    }

    auto adapter = eco::e2e::CreateAdapter(repo_root, build_dir);
    if (!adapter) {
        std::cerr << "Error: CreateAdapter returned null.\n";
        return 2;
    }

    const bool is_live = adapter->IsLiveBackend();
    if (!is_live && !contract_check_flag) {
        std::cerr << "FATAL: Non-live contract-check backend cannot run without --contract-check.\n"
                  << "Build live game_core/eco_proto and use eco_e2e_runner for live verification.\n";
        return 2;
    }

    if (!is_live && contract_check_flag) {
        std::cout << "[CONTRACT-CHECK MODE] Live backend not linked. "
                  << "Verifying contracts/invariants and assertion self-tests ONLY.\n";
        if (!RunContractAssertionSelfTest()) {
            return 1;
        }
    }

    int pass_count = 0;
    int contract_only_count = 0;
    int fail_count = 0;

    for (const auto* tc : selected) {
        try {
            tc->fn(*adapter);
            if (is_live) {
                std::cout << "[PASS] " << tc->name << "\n";
                ++pass_count;
            } else {
                // Even if a test had no live-only probe, never claim live PASS when !is_live
                std::cout << "[CONTRACT_ONLY] " << tc->name
                          << " (contract invariants verified; live probe not linked)\n";
                ++contract_only_count;
            }
        } catch (const eco::e2e::ContractOnlyException& ex) {
            if (!is_live && contract_check_flag) {
                std::cout << "[CONTRACT_ONLY] " << tc->name << " (" << ex.what() << ")\n";
                ++contract_only_count;
            } else {
                std::cout << "[FAIL] " << tc->name
                          << " (CONTRACT_ONLY disallowed in live mode: " << ex.what() << ")\n";
                ++fail_count;
            }
        } catch (const eco::e2e::AssertionFailure& ex) {
            std::cout << "[FAIL] " << tc->name << " — " << ex.file << ":" << ex.line
                      << ": " << ex.message << "\n";
            ++fail_count;
        } catch (const std::exception& ex) {
            std::cout << "[FAIL] " << tc->name << " — uncaught exception: " << ex.what() << "\n";
            ++fail_count;
        } catch (...) {
            std::cout << "[FAIL] " << tc->name << " — unknown exception\n";
            ++fail_count;
        }
    }

    std::cout << "\n=== E2E Execution Summary ===\n";
    std::cout << "Backend:       " << (is_live ? "LIVE (game_core + eco_proto)" : "CONTRACT_CHECK_ONLY") << "\n";
    std::cout << "PASS:          " << pass_count << "\n";
    std::cout << "CONTRACT_ONLY: " << contract_only_count << "\n";
    std::cout << "FAIL:          " << fail_count << "\n";

    if (fail_count > 0) {
        return 1;
    }
    if (is_live && contract_only_count > 0) {
        return 1;
    }
    return 0;
}
