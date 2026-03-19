# libregent

A C++20 library for rule-based syntactic text simplification, based on the RegenT system by Siddharthan et al.

### Why does this exist?

Because philologists and computer scientists don't talk enough.

The former understand the complexity of meaning but rarely ship software. The latter ship software but treat meaning as a loss function. What tooling exists is proprietary, academic Java from decades past, or Python wrappers around Python wrappers: none of it fast, none of it embeddable, none of it intended to outlive a paper.

libregent is what this work looks like when someone gets annoyed enough to fix it.

## Overview

libregent takes dependency-parsed sentences and produces syntactically simplified text. It splits complex sentences into shorter ones, converts passive voice to active, simplifies relative clauses and coordination, and maintains proper discourse coherence with cue words and referring expressions.

This is a pure C++ implementation with no runtime ML dependencies, with optional Python bindings via nanobind.

## Features

- 60+ transformation rules covering coordination, subordination, relative clauses, apposition, passive voice, and complex lexico-syntactic reformulations
- CSP-based sentence ordering algorithm that preserves conjunctive cohesion
- Intelligent determiner choice and noun phrase generation
- Anaphoric post-processing to fix broken pronominal links after restructuring
- Gen-light linearisation that reuses original word order for robust text generation
- N-best parse ranking to select the best output from multiple parse hypotheses
- Built-in CoNLL-U parser for Universal Dependencies format

## Requirements

- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.20+
- Python 3.8+ (optional, for Python bindings)
- nanobind (automatically fetched by CMake)
- Catch2 (optional, for tests - automatically fetched)
- Google Benchmark (optional, for benchmarks - automatically fetched)

## Building

```bash
# Clone the repository
git clone <repository-url>
cd libregent

# Create build directory
mkdir build && cd build

# Configure
cmake .. \
    -DREGENT_BUILD_TESTS=ON \
    -DREGENT_BUILD_BENCHMARKS=ON \
    -DREGENT_BUILD_PYTHON=ON \
    -DREGENT_BUILD_EXAMPLES=ON

# Build
cmake --build . -j

# Run tests
ctest --output-on-failure

# Install
sudo cmake --install .
```

## Quick start (C++)

```cpp
#include <regent/regent.h>
#include <iostream>

int main() {
    // Parse a CoNLL-U formatted sentence
    std::string conllu = R"(
1	The	the	DET	DT	_	2	det	_	_
2	cat	cat	NOUN	NN	_	4	nsubj:pass	_	_
3	was	be	AUX	VBD	_	4	aux:pass	_	_
4	chased	chase	VERB	VBN	_	0	root	_	_
5	by	by	ADP	IN	_	7	case	_	_
6	the	the	DET	DT	_	7	det	_	_
7	dog	dog	NOUN	NN	_	4	obl:agent	_	_
8	.	.	PUNCT	.	_	4	punct	_	_
    )";

    auto sentences = regent::Simplifier::parse_conllu(conllu);

    // Create simplifier with default configuration
    regent::Simplifier simplifier;

    // Simplify
    auto result = simplifier.simplify(sentences);

    // Output
    std::cout << "Original: The cat was chased by the dog.\n";
    std::cout << "Simplified: " << result.text << "\n";
    std::cout << "Transforms applied: " << result.transforms_applied << "\n";
    std::cout << "Avg sentence length: " << result.avg_sentence_length << "\n";

    return 0;
}
```

## Quick start (Python)

```python
import regent

# Parse CoNLL-U
sentences = regent.Simplifier.parse_conllu("""
1	The	the	DET	DT	_	2	det	_	_
2	cat	cat	NOUN	NN	_	4	nsubj:pass	_	_
3	was	be	AUX	VBD	_	4	aux:pass	_	_
4	chased	chase	VERB	VBN	_	0	root	_	_
5	by	by	ADP	IN	_	7	case	_	_
6	the	the	DET	DT	_	7	det	_	_
7	dog	dog	NOUN	NN	_	4	obl:agent	_	_
8	.	.	PUNCT	.	_	4	punct	_	_
""")

# Create simplifier
config = regent.Config()
config.convert_passive = True
simplifier = regent.Simplifier(config)

# Simplify
result = simplifier.simplify_parsed(sentences)

print(f"Simplified: {result.text}")
print(f"Transforms: {result.transforms_applied}")
```

## Configuration options

```cpp
regent::Config config;

// Enable or disable specific transformations
config.convert_passive = true;
config.simplify_relative_clauses = true;
config.simplify_apposition = true;
config.simplify_coordination = true;
config.simplify_subordination = true;

// N-best parse ranking
config.n_best_parses = 1;  // 1 = single parse, 50 = full n-best

// Minimum sentence length to simplify
config.min_sentence_length = 5;

// Anaphora preservation level
config.anaphora_level = regent::Config::AnaphoraLevel::LocalCoherence;  // Recommended
```

## Project structure

```
libregent/
├── CMakeLists.txt          # Build configuration
├── include/regent/         # Public headers
│   ├── regent.h           # Main public API
│   ├── types.h            # Core data types
│   ├── dep_graph.h        # Dependency graph operations
│   ├── rule.h             # Rule representation
│   ├── transformer.h      # Main transformation engine
│   ├── ordering.h         # Sentence ordering CSP
│   ├── cue_words.h        # Cue word selection
│   ├── determiners.h      # Determiner choice
│   ├── referring.h        # Referring expression generation
│   ├── anaphora.h         # Anaphoric post-processor
│   ├── lineariser.h       # Gen-light linearisation
│   ├── ranker.h           # N-best parse ranking
│   ├── rule_registry.h    # Built-in rule definitions
│   └── conllu.h           # CoNLL-U parser
├── src/                    # Implementation files
├── python/                 # Python bindings
├── tests/                  # Unit and integration tests
├── benchmarks/             # Performance benchmarks
└── examples/               # Example programs
```

## Rule categories

The library implements 63+ transformation rules across six categories:

1. Coordination (~10 rules): `and`, `but`, `or`, `yet`, semicolons, VP coordination
2. Subordination (~15 rules): `because`, `although`, `when`, `while`, `if`, `unless`, etc.
3. Relative clauses (~8 rules): restrictive/non-restrictive, reduced, infinitival
4. Apposition (~5 rules): restrictive/non-restrictive, titles
5. Passive to active (~5 rules): simple, modal, perfect, ditransitive variants
6. Complex lexico-syntactic (~7 rules): nominalisation unpacking, causality reformulation

## Algorithm

The system uses a three-stage pipeline:

1. Analysis: dependency parsing (external, e.g. spaCy, UDPipe, Stanza)
2. Transformation: recursive stack-based rule application with CSP-based ordering
3. Regeneration: cue-word selection, determiner choice, referring expressions, anaphora resolution, linearisation

### Transformation loop

```
1. Push parsed sentence onto stack
2. While stack not empty:
   a. Pop sentence
   b. If no simplifiable construct -> output
   c. Else:
      i.   Find highest-priority matching rule
      ii.  Apply transformation -> produces (a, R, b)
      iii. Run sentence ordering CSP
      iv.  Push both sentences back (in decided order)
3. Run anaphoric post-processor on output
4. Linearise final text
```

## Performance

- Sentence length reduction: ~40% average (e.g. 25 words -> 15 words)
- Readability improvement: Flesch Reading Ease typically increases by 10-15 points
- Accuracy: ~88% grammatical and meaning-preserving (gen-light with 50-best parses)
- Throughput: ~1000 sentences per second, per thread, on modern CPU

## Testing

```bash
# Run all tests
cd build
ctest --output-on-failure

# Run specific test suite
./tests/test_dep_graph
./tests/test_rules
./tests/test_integration

# Run benchmarks
./benchmarks/bench_simplifier
```

## Citation

If you use this library in academic work, please cite the original RegenT papers:

```bibtex
@phdthesis{siddharthan2003,
  author = {Siddharthan, Advaith},
  title = {Syntactic Simplification and Text Cohesion},
  school = {University of Cambridge},
  year = {2003},
  type = {PhD thesis}
}

@article{siddharthan2006,
  author = {Siddharthan, Advaith},
  title = {Syntactic Simplification and Text Cohesion},
  journal = {Research on Language and Computation},
  volume = {4},
  number = {1},
  pages = {77--109},
  year = {2006}
}

@inproceedings{siddharthan2011,
  author = {Siddharthan, Advaith},
  title = {Text Simplification using Typed Dependencies: A Comparison of the Robustness of Different Generation Strategies},
  booktitle = {Proceedings of ENLG 2011},
  pages = {2--11},
  year = {2011}
}
```

## License

MIT License (see LICENSE file)

## Contributing

Contributions welcome! Please:
1. Follow the existing code style (C++20, clang-format)
2. Add tests for new functionality
3. Update documentation
4. Ensure all tests pass before submitting PR

## Acknowledgments

Based on the research by Advaith Siddharthan and collaborators at the University of Aberdeen and University of Cambridge. This is an independent implementation following the published algorithms and specifications.

## Contact

For questions, issues, or contributions, please open an issue on GitHub.
