// Copyright (c) 2025 libregent
// SPDX-License-Identifier: MIT

#pragma once

#include "regent/types.h"
#include <optional>
#include <string>
#include <unordered_map>

namespace regent {

// Forward declaration
class DepGraph;

/// Selects cue words based on discourse relations and sentence properties
class CueWordSelector {
public:
    /// Given a relation and the nucleus sentence (for tense), produce the cue word string
    [[nodiscard]] std::optional<std::string> select(Relation rel, const DepGraph& nucleus) const;

private:
    /// Determine auxiliary tense ("is" or "was") from the nucleus
    [[nodiscard]] std::string determine_aux(const DepGraph& nucleus) const;

    /// Static mapping from Relation -> cue-word template
    static const std::unordered_map<Relation, std::string> templates_;
};

}  // namespace regent
