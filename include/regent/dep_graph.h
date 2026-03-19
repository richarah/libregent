// Copyright (c) 2025 libregent
// SPDX-License-Identifier: MIT

#pragma once

#include "regent/types.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace regent {

// Forward declarations
struct DepPattern;
struct NodeOp;
class Config;

/// Wraps ParsedSentence with graph operations needed by the rule engine
class DepGraph {
public:

    /// Construct a dependency graph from a parsed sentence
    explicit DepGraph(ParsedSentence sentence);

    /// Copy constructor
    DepGraph(const DepGraph&) = default;

    /// Move constructor
    DepGraph(DepGraph&&) noexcept = default;

    /// Copy assignment
    DepGraph& operator=(const DepGraph&) = default;

    /// Move assignment
    DepGraph& operator=(DepGraph&&) noexcept = default;

    /// Destructor
    ~DepGraph() = default;

    /// Pattern matching: try to unify a rule's CONTEXT against this graph
    /// Returns bindings (variable name -> token id) if successful
    [[nodiscard]] regent::Bindings match(const std::vector<DepPattern>& patterns) const;

    /// Mutation: apply DELETE, INSERT, NODE-OPS to produce a new graph
    [[nodiscard]] DepGraph apply_transform(
        const regent::Bindings& bindings,
        const std::vector<DepPattern>& deletions,
        const std::vector<DepPattern>& insertions,
        const std::vector<NodeOp>& node_ops,
        const std::unordered_map<uint32_t, std::vector<uint32_t>>& ordering_specs = {}
    ) const;

    /// Split: given a transform that creates two sentence trees,
    /// extract them as two separate DepGraphs
    [[nodiscard]] std::pair<DepGraph, DepGraph> split_trees() const;

    /// Query: does this graph have any simplifiable construct?
    [[nodiscard]] bool has_simplifiable_construct(const Config& config) const;

    /// Get all root tokens in this graph
    [[nodiscard]] std::vector<uint32_t> roots() const;

    /// Get the underlying sentence
    [[nodiscard]] const ParsedSentence& sentence() const noexcept { return sent_; }

    /// Get mutable sentence (for testing)
    [[nodiscard]] ParsedSentence& sentence() noexcept { return sent_; }

    /// Tree traversal for linearisation
    /// Returns tokens in traversal order, respecting ORDERING specs
    [[nodiscard]] std::vector<uint32_t> linearisation_order(
        const std::unordered_map<uint32_t, std::vector<uint32_t>>& ordering_specs
    ) const;

    /// Get all dependents of a given head
    [[nodiscard]] std::vector<const DepRel*> dependents_of(uint32_t head) const;

    /// Get the head of a given dependent (returns nullptr if no head found)
    [[nodiscard]] const DepRel* head_of(uint32_t dep) const;

    /// Find all tokens reachable from a given token (including the token itself)
    [[nodiscard]] std::unordered_set<uint32_t> reachable_from(uint32_t token_id) const;

    /// Check if two tokens are in different connected components
    [[nodiscard]] bool in_different_trees(uint32_t token1, uint32_t token2) const;

    /// Get all tokens in the subtree rooted at the given token
    [[nodiscard]] std::vector<uint32_t> subtree(uint32_t root) const;

    /// Find token ID by lemma
    [[nodiscard]] std::vector<uint32_t> find_by_lemma(std::string_view lemma) const;

    /// Check if there's a comma immediately before or after a token span
    [[nodiscard]] bool has_comma_before(uint32_t token_id) const;
    [[nodiscard]] bool has_comma_after(uint32_t token_id) const;
    [[nodiscard]] bool has_commas_around(uint32_t start, uint32_t end) const;

    /// Check if there are parentheses around a token span
    [[nodiscard]] bool has_parens_around(uint32_t start, uint32_t end) const;

private:
    ParsedSentence sent_;

    // Adjacency lists for efficient traversal
    std::unordered_map<uint32_t, std::vector<const DepRel*>> head_to_deps_;
    std::unordered_map<uint32_t, const DepRel*> dep_to_head_;

    /// Build the adjacency lists
    void build_indices();

    /// DFS helper for traversal
    void dfs_linearise(
        uint32_t node,
        const std::unordered_map<uint32_t, std::vector<uint32_t>>& ordering_specs,
        std::unordered_set<uint32_t>& visited,
        std::vector<uint32_t>& output
    ) const;

    /// DFS helper for reachability
    void dfs_reachable(
        uint32_t node,
        std::unordered_set<uint32_t>& visited
    ) const;
};

}  // namespace regent
