// Simple integration test
#include "regent/regent.h"
#include <iostream>

int main() {
    using namespace regent;

    // Create config
    Config config;
    config.anaphora_level = Config::AnaphoraLevel::LocalCoherence;
    config.simplify_coordination = true;
    config.simplify_subordination = true;

    // Create simplifier
    Simplifier simplifier(config);

    // Test 1: Simple coordination
    std::cout << "Test 1: Coordination\n";
    std::cout << "Input: \"The cat slept and the dog barked.\"\n";

    ParsedSentence sent1;
    sent1.tokens = {
        {1, "The", "the", "DET", "DT"},
        {2, "cat", "cat", "NOUN", "NN"},
        {3, "slept", "sleep", "VERB", "VBD"},
        {4, "and", "and", "CCONJ", "CC"},
        {5, "the", "the", "DET", "DT"},
        {6, "dog", "dog", "NOUN", "NN"},
        {7, "barked", "bark", "VERB", "VBD"},
        {8, ".", ".", "PUNCT", "."}
    };
    sent1.deps = {
        {"root", 0, 3},
        {"det", 2, 1},
        {"nsubj", 3, 2},
        {"cc", 3, 4},
        {"conj", 3, 7},
        {"det", 6, 5},
        {"nsubj", 7, 6},
        {"punct", 3, 8}
    };

    auto result1 = simplifier.simplify({sent1});
    std::cout << "Output sentences: " << result1.sentences.size() << "\n";
    std::cout << "Text: " << result1.text << "\n";
    for (const auto& s : result1.sentences) {
        std::cout << "  - " << s.tokens.size() << " tokens";
        if (s.cue_word) {
            std::cout << " (cue: \"" << *s.cue_word << "\")";
        }
        std::cout << "\n";
    }
    std::cout << "\n";

    // Test 2: Simple subordination
    std::cout << "Test 2: Subordination\n";
    std::cout << "Input: \"The cat slept because it was tired.\"\n";

    ParsedSentence sent2;
    sent2.tokens = {
        {1, "The", "the", "DET", "DT"},
        {2, "cat", "cat", "NOUN", "NN"},
        {3, "slept", "sleep", "VERB", "VBD"},
        {4, "because", "because", "SCONJ", "IN"},
        {5, "it", "it", "PRON", "PRP"},
        {6, "was", "be", "AUX", "VBD"},
        {7, "tired", "tired", "ADJ", "JJ"},
        {8, ".", ".", "PUNCT", "."}
    };
    sent2.deps = {
        {"root", 0, 3},
        {"det", 2, 1},
        {"nsubj", 3, 2},
        {"mark", 7, 4},
        {"advcl", 3, 7},
        {"nsubj", 7, 5},
        {"cop", 7, 6},
        {"punct", 3, 8}
    };

    auto result2 = simplifier.simplify({sent2});
    std::cout << "Output sentences: " << result2.sentences.size() << "\n";
    std::cout << "Text: " << result2.text << "\n";
    for (const auto& s : result2.sentences) {
        std::cout << "  - " << s.tokens.size() << " tokens";
        if (s.cue_word) {
            std::cout << " (cue: \"" << *s.cue_word << "\")";
        }
        std::cout << "\n";
    }
    std::cout << "\n";

    std::cout << "[OK] Integration tests completed successfully!\n";
    return 0;
}
