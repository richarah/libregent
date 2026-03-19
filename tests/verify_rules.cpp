// Quick verification of rule count
#include "regent/rule_registry.h"
#include <iostream>

int main() {
    using namespace regent;

    auto coord = RuleRegistry::get_coordination_rules();
    auto subord = RuleRegistry::get_subordination_rules();
    auto relcl = RuleRegistry::get_relative_clause_rules();
    auto appos = RuleRegistry::get_apposition_rules();
    auto passive = RuleRegistry::get_passive_rules();
    auto participial = RuleRegistry::get_participial_rules();
    auto infinitival = RuleRegistry::get_infinitival_rules();
    auto ccomp = RuleRegistry::get_clausal_complement_rules();
    auto np_mod = RuleRegistry::get_np_modification_rules();
    auto comparative = RuleRegistry::get_comparative_rules();
    auto all_rules = RuleRegistry::get_all_rules();

    std::cout << "Rule counts by category:\n";
    std::cout << "  Coordination: " << coord.size() << "\n";
    std::cout << "  Subordination: " << subord.size() << "\n";
    std::cout << "  Relative clause: " << relcl.size() << "\n";
    std::cout << "  Apposition: " << appos.size() << "\n";
    std::cout << "  Passive: " << passive.size() << "\n";
    std::cout << "  Participial: " << participial.size() << "\n";
    std::cout << "  Infinitival: " << infinitival.size() << "\n";
    std::cout << "  Clausal complement: " << ccomp.size() << "\n";
    std::cout << "  NP modification: " << np_mod.size() << "\n";
    std::cout << "  Comparative: " << comparative.size() << "\n";
    std::cout << "\n";

    size_t sum = coord.size() + subord.size() + relcl.size() + appos.size() +
                 passive.size() + participial.size() + infinitival.size() +
                 ccomp.size() + np_mod.size() + comparative.size();

    std::cout << "Total rules (individual): " << sum << "\n";
    std::cout << "Total rules (get_all_rules): " << all_rules.size() << "\n";

    if (sum == all_rules.size()) {
        std::cout << "\n[OK] All rules accounted for!\n";
        return 0;
    } else {
        std::cout << "\n[FAIL] Mismatch in rule counts!\n";
        return 1;
    }
}
