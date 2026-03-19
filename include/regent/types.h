// Copyright (c) 2025 libregent
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace regent {

// Forward declarations
class DepGraph;

/// Bindings from variable names to token IDs (used by rules and dep_graph)
struct Bindings {
    std::unordered_map<std::string, uint32_t> vars;  // "??X0" -> token_id
    bool success = false;
};

/// A token in the sentence with position, surface form, lemma, and POS tags
struct Token {
    uint32_t id;                                              // 1-indexed position in sentence
    std::string form;                                         // Surface form ("was")
    std::string lemma;                                        // Lemma ("be")
    std::string upos;                                         // Universal POS tag ("AUX")
    std::string xpos;                                         // Language-specific POS ("VBD")
    std::vector<std::pair<std::string, std::string>> feats;  // Morphological features

    /// Equality comparison
    bool operator==(const Token& other) const noexcept {
        return id == other.id && form == other.form && lemma == other.lemma &&
               upos == other.upos && xpos == other.xpos;
    }

    /// Check if token is a verb
    [[nodiscard]] bool is_verb() const noexcept {
        return upos == "VERB" || upos == "AUX" || xpos.starts_with("VB");
    }

    /// Check if token is a noun
    [[nodiscard]] bool is_noun() const noexcept {
        return upos == "NOUN" || upos == "PROPN" || xpos.starts_with("NN");
    }

    /// Check if token is plural
    [[nodiscard]] bool is_plural() const noexcept {
        // Check morphological features
        for (const auto& [key, val] : feats) {
            if (key == "Number" && val == "Plur") return true;
        }
        // Check xpos tag
        return xpos == "NNS" || xpos == "NNPS";
    }

    /// Check if token is a proper noun
    [[nodiscard]] bool is_proper_noun() const noexcept {
        return upos == "PROPN" || xpos == "NNP" || xpos == "NNPS";
    }

    /// Check if token is a pronoun
    [[nodiscard]] bool is_pronoun() const noexcept {
        return upos == "PRON" || xpos.starts_with("PRP") || xpos == "WP";
    }

    /// Check if token is past tense
    [[nodiscard]] bool is_past_tense() const noexcept {
        for (const auto& [key, val] : feats) {
            if (key == "Tense" && val == "Past") return true;
        }
        return xpos == "VBD" || xpos == "VBN";
    }
};

/// A typed dependency relation between two tokens
struct DepRel {
    std::string rel;  // Relation type ("nsubj", "advcl", "acl:relcl", etc.)
    uint32_t head;    // Governor token id
    uint32_t dep;     // Dependent token id

    /// Equality comparison
    bool operator==(const DepRel& other) const noexcept {
        return rel == other.rel && head == other.head && dep == other.dep;
    }

    /// Check if this is a root relation
    [[nodiscard]] bool is_root() const noexcept {
        return rel == "root" && head == 0;
    }
};

/// A parsed sentence: tokens + dependency relations
struct ParsedSentence {
    std::vector<Token> tokens;
    std::vector<DepRel> deps;

    /// Find token by id (1-indexed)
    [[nodiscard]] const Token* token(uint32_t id) const noexcept {
        if (id == 0 || id > tokens.size()) return nullptr;
        return &tokens[id - 1];  // Convert to 0-indexed
    }

    /// Find token by id (mutable)
    [[nodiscard]] Token* token(uint32_t id) noexcept {
        if (id == 0 || id > tokens.size()) return nullptr;
        return &tokens[id - 1];
    }

    /// Find all deps matching a relation type
    [[nodiscard]] std::vector<const DepRel*> find_deps(std::string_view rel) const;

    /// Find all deps where the given token is head or dependent
    [[nodiscard]] std::vector<const DepRel*> deps_of(uint32_t token_id) const;

    /// Find root token id
    [[nodiscard]] std::vector<uint32_t> roots() const;
};

/// Discourse relation between two simplified sentences
enum class Relation {
    Concession,        // although, though, but, however, whereas
    AntiConditional,   // or, or else
    Cause,             // because (before reversal)
    Result,            // because (after reversal)
    And,               // and
    When,
    Before,
    After,
    Since,
    As,
    While,
    If,
    Unless,
    SoThat,        // so that
    InOrderTo,     // in order to
    Elaboration,   // non-restrictive RC / appositive
    Identification // restrictive RC / appositive
};

/// Convert relation to string for debugging
[[nodiscard]] std::string_view to_string(Relation rel) noexcept;

/// Constraint type for sentence ordering
enum class ConstraintType { Hard, Soft };

/// Ordering constraint specification
enum class OrderConstraint {
    APrecedesB,
    BPrecedesA,
    NucleusFirst,
    NucleusLast,
};

/// A constraint for sentence ordering
struct Constraint {
    ConstraintType type;
    OrderConstraint order;

    bool operator==(const Constraint& other) const noexcept {
        return type == other.type && order == other.order;
    }
};

/// A simplified output sentence with metadata
struct SimplifiedSentence {
    std::vector<Token> tokens;  // Tokens in output order
    std::vector<DepRel> deps;   // Transformed dependency relations

    // Metadata for regeneration
    std::optional<std::string> cue_word;                   // Prepended cue word, if any
    std::optional<Relation> relation_to_prev;              // Relation to the sentence this was split from
    std::unordered_map<uint32_t, std::vector<uint32_t>> ordering_specs;  // Node ordering specifications

    /// Find root tokens
    [[nodiscard]] std::vector<uint32_t> roots() const;
};

/// Configuration for the simplifier
struct Config {
    /// Anaphora preservation level
    enum class AnaphoraLevel {
        Cohesion,         // Replace pronouns when most salient entity changes
        Coherence,        // Replace when absolute antecedent changes
        LocalCoherence    // Replace when both immediate AND absolute antecedents change (recommended)
    };

    AnaphoraLevel anaphora_level = AnaphoraLevel::LocalCoherence;
    bool convert_passive = true;
    bool simplify_relative_clauses = true;
    bool simplify_apposition = true;
    bool simplify_coordination = true;
    bool simplify_subordination = true;
    uint32_t n_best_parses = 1;        // 1 = single parse, >1 = n-best ranking
    uint32_t min_sentence_length = 5;  // Don't simplify sentences shorter than this
};

/// Final output of the simplifier
struct SimplificationResult {
    std::vector<SimplifiedSentence> sentences;
    std::string text;  // Linearised output text

    // Metadata
    uint32_t transforms_applied = 0;
    double avg_sentence_length = 0.0;

    /// Calculate statistics
    void compute_stats() noexcept {
        if (sentences.empty()) {
            avg_sentence_length = 0.0;
            return;
        }

        size_t total_tokens = 0;
        for (const auto& sent : sentences) {
            total_tokens += sent.tokens.size();
        }
        avg_sentence_length = static_cast<double>(total_tokens) / sentences.size();
    }
};

}  // namespace regent
