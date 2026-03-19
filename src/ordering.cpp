// Copyright (c) 2025 libregent
// SPDX-License-Identifier: MIT

#include "regent/ordering.h"
#include "regent/dep_graph.h"

namespace regent {

SentenceOrderer::OrderResult SentenceOrderer::decide_order(
    const DepGraph& a,
    Relation R,
    const DepGraph& b,
    const std::vector<Constraint>& inherited_constraints
) const {
    OrderResult result;
    result.order = OrderResult::Order::AB;  // Default to (a, b)

    // Copy inherited constraints
    result.constraints_a = inherited_constraints;
    result.constraints_b = inherited_constraints;

    // Add relation-specific constraints
    std::vector<Constraint> C = inherited_constraints;
    add_relation_constraints(R, C, result.constraints_a, result.constraints_b);

    // CSP Algorithm: Check hard constraints
    bool has_ab_hard = has_hard_constraint(C, OrderConstraint::APrecedesB);
    bool has_ba_hard = has_hard_constraint(C, OrderConstraint::BPrecedesA);

    // Check for conflicts
    if (has_ab_hard && has_ba_hard) {
        // Conflict: prefer original order (AB) as fallback
        result.order = OrderResult::Order::AB;
        return result;
    }

    // Satisfy hard constraints
    if (has_ab_hard) {
        result.order = OrderResult::Order::AB;
        return result;
    }
    if (has_ba_hard) {
        result.order = OrderResult::Order::BA;
        return result;
    }

    // No hard constraints: use soft constraints and heuristics
    int ab_score = 0;
    int ba_score = 0;

    // Count soft constraints
    for (const auto& c : C) {
        if (c.type == ConstraintType::Soft) {
            if (c.order == OrderConstraint::APrecedesB) {
                ab_score++;
            } else if (c.order == OrderConstraint::BPrecedesA) {
                ba_score++;
            }
        }
    }

    // Heuristic: Check if further simplification possible
    Config dummy_config;
    if (has_further_simplifiable(a, dummy_config)) {
        ab_score++;  // Prefer simplifying 'a' first
    }
    if (has_further_simplifiable(b, dummy_config)) {
        ba_score++;  // Prefer simplifying 'b' first
    }

    // Heuristic: Centering/connectedness
    if (check_connectedness(a, b)) {
        ab_score++;  // Entities in 'a' connect to 'b'
    }

    // Decide based on scores
    if (ba_score > ab_score) {
        result.order = OrderResult::Order::BA;
    } else {
        result.order = OrderResult::Order::AB;  // Default to original order
    }

    return result;
}

bool SentenceOrderer::has_hard_constraint(const std::vector<Constraint>& C, OrderConstraint dir) const {
    for (const auto& c : C) {
        if (c.type == ConstraintType::Hard && c.order == dir) {
            return true;
        }
    }
    return false;
}

bool SentenceOrderer::has_conflicting_hard([[maybe_unused]] const std::vector<Constraint>& C, [[maybe_unused]] OrderConstraint dir) const {
    // TODO: Implement conflict detection
    return false;
}

bool SentenceOrderer::check_connectedness([[maybe_unused]] const DepGraph& a, [[maybe_unused]] const DepGraph& b) const {
    // TODO: Implement centering theory heuristic
    return false;
}

bool SentenceOrderer::has_further_simplifiable(const DepGraph& a, [[maybe_unused]] const Config& config) const {
    // TODO: Implement proper check
    (void)a;
    return false;
}

void SentenceOrderer::add_relation_constraints(
    Relation R,
    std::vector<Constraint>& C,
    std::vector<Constraint>& Ca,
    std::vector<Constraint>& Cb
) const {
    // Add constraints based on relation type
    switch (R) {
        case Relation::Result:
            // "because" reversed: hard b->a
            C.push_back({ConstraintType::Hard, OrderConstraint::BPrecedesA});
            Ca.push_back({ConstraintType::Soft, OrderConstraint::NucleusFirst});
            Cb.push_back({ConstraintType::Soft, OrderConstraint::NucleusLast});
            break;

        case Relation::Elaboration:
            // Non-restrictive: soft a->b
            C.push_back({ConstraintType::Soft, OrderConstraint::APrecedesB});
            break;

        case Relation::Identification:
            // Restrictive: soft b->a
            C.push_back({ConstraintType::Soft, OrderConstraint::BPrecedesA});
            break;

        default:
            // Most relations: hard a->b
            C.push_back({ConstraintType::Hard, OrderConstraint::APrecedesB});
            Ca.push_back({ConstraintType::Soft, OrderConstraint::NucleusLast});
            Cb.push_back({ConstraintType::Soft, OrderConstraint::NucleusFirst});
            break;
    }
}

}  // namespace regent
