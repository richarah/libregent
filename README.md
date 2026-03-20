# libregent

A C++20 library for rule-based syntactic text simplification, based on the RegenT system.

## Overview

libregent takes dependency-parsed sentences and produces syntactically simplified text. It splits complex sentences into shorter ones, converts passive voice to active, simplifies relative clauses and coordination, and maintains proper discourse coherence with cue words and referring expressions.

This is a pure C++ implementation with no runtime ML dependencies, with optional Python bindings via nanobind.

## Features

- 63 transformation rules covering coordination, subordination, relative clauses, apposition, passive voice, participial clauses, infinitival clauses, clausal complements, and complex lexico-syntactic reformulations
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
- nanobind (automatically fetched by CMake if Python bindings enabled)
- Catch2 (automatically fetched by CMake if tests enabled)

## Build

```bash
# Clone the repository
git clone <repository-url>
cd libregent

# Create build directory
mkdir build && cd build

# Configure
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DREGENT_BUILD_TESTS=ON \
    -DREGENT_BUILD_PYTHON=ON \
    -DREGENT_BUILD_EXAMPLES=ON

# Build
cmake --build . --target regent -j

# Run tests
ctest --output-on-failure

# Install
sudo cmake --install .
```

## Quickstart (C++)

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

## Quickstart (Python)

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

## Usage (CLI)

The `regent-cli` tool reads CoNLL-U formatted input and writes simplified text to stdout.

### Basic usage

```bash
# From stdin
cat input.conllu | regent-cli

# From file
regent-cli -i input.conllu

# To file
regent-cli -i input.conllu -o output.txt

# With statistics (to stderr)
cat input.conllu | regent-cli --stats
```

### Examples

```bash
# Simple pipe
echo "1	The	the	DET	DT	_	2	det	_	_
2	cat	cat	NOUN	NN	_	3	nsubj	_	_
3	slept	sleep	VERB	VBD	_	0	root	_	_
4	because	because	SCONJ	IN	_	7	mark	_	_
5	it	it	PRON	PRP	_	7	nsubj	_	_
6	was	be	AUX	VBD	_	7	cop	_	_
7	tired	tired	ADJ	JJ	_	3	advcl	_	_
8	.	.	PUNCT	.	_	3	punct	_	_" | regent-cli

# Output: It was tired. So, the cat slept.

# Disable specific transformations
regent-cli -i input.conllu --no-passive --no-coord

# Show transformation statistics
regent-cli -i input.conllu --stats
# Statistics:
#   Input sentences:  1
#   Output sentences: 2
#   Transforms:       1
#   Avg length:       3.5 tokens

# Chain with other tools
cat corpus.conllu | regent-cli | wc -l
```

### Options

```
-i, --input FILE        Input file (CoNLL-U format)
-o, --output FILE       Output file (default: stdout)
--min-length N          Minimum sentence length (default: 5)
--no-passive            Disable passive voice conversion
--no-relcl              Disable relative clause simplification
--no-appos              Disable apposition simplification
--no-coord              Disable coordination simplification
--no-subord             Disable subordination simplification
--anaphora LEVEL        Anaphora level: cohesion, coherence, local (default: local)
--stats                 Print statistics to stderr
-h, --help              Show help message
```

## Config

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

## Structure

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
└── examples/               # Example programs
```

## Rule categories

The library implements 63 transformation rules across nine categories:

1. Coordination (12 rules): Clausal coordination (`and`, `but`, `or`, `yet`, `so`, `nor`, semicolons) and VP coordination with shared subjects
2. Subordination (16 rules): `because`, `although`, `though`, `when`, `while`, `if`, `unless`, `after`, `before`, `since`, `as`, `so that`, `in order to`, `whereas`, `until`, `however`
3. Relative clauses (8 rules): restrictive/non-restrictive, reduced, infinitival; handles `who`, `which`, `that`, `whom`, `whose`
4. Apposition (8 rules): restrictive/non-restrictive, titles, roles, parenthetical, name descriptions, locations
5. Passive to active (5 rules): simple, get-passive, modal, adjectival, agentless variants
6. Participial clauses (2 rules): present and past participial clauses
7. Infinitival clauses (2 rules): purpose and result infinitives
8. Clausal complements (3 rules): `that`-clauses, clausal subjects, parataxis
9. Complex lexico-syntactic (7 rules): nominalisation unpacking, causality reformulation, compound sentence splitting, negative copula rewriting, modifier chain splitting

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

## Testing

```bash
# Run all tests
cd build
ctest --output-on-failure

# Run specific test suite
./tests/test_dep_graph
./tests/test_rules
./tests/test_integration
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
