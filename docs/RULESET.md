# Complete Siddharthan / RegenT Syntactic Simplification Ruleset

**A standalone specification for implementing a general-purpose, newspaper-text-grade syntactic simplification system.**

Reconstructed from: Siddharthan (2003 PhD thesis, Cambridge Tech Report 597), Siddharthan (2006 journal paper, Research on Language & Computation), Siddharthan (2010 INLG), Siddharthan (2011 ENLG — RegenT), Siddharthan & Mandya (2014 EACL — hybrid system), Siddharthan & Copestake (2004 ACL — referring expressions).

---

## Part 1: System Architecture

Three-stage pipeline: **Analysis -> Transformation -> Regeneration**

### 1.1 Analysis Stage

The analysis module produces:

1. **Sentence segmentation**
2. **POS tagging** of all tokens
3. **Noun phrase chunking** with grammatical function annotation (subject, object, etc.)
4. **Clause and appositive boundary identification** — marking which spans are simplifiable constructs
5. **Clause and appositive attachment** — determining which NP a relative clause or appositive attaches to
6. **Pronoun coreference resolution** — co-referring third-person pronouns with antecedents (used later by anaphoric post-processor)

In the original system (2003), analysis uses the LT Text Tokenization Toolkit with POS tagging and noun chunking, plus custom clause/appositive identification using local context and WordNet animacy information.

In RegenT (2011+), analysis is replaced by **Stanford Parser typed dependencies** — flat triples of (relation, governor, dependent) with POS tags and word positions.

### 1.2 Transformation Stage

Operates as a **recursive stack-based algorithm**:

```
1. Push all analysed sentences onto a stack (first sentence on top)
2. LOOP:
   a. Pop top sentence from stack
   b. BASE CASE: sentence contains no simplifiable construct
      -> append to output queue
   c. RECURSIVE CASE:
      i.   Apply highest-priority applicable transformation rule
      ii.  Rule produces two sentences (a, b) and a relation R
      iii. Send (a, R, b) to Conjunctive Cohesion module (regeneration)
      iv.  Module returns ordered pair and constraint sets
      v.   Push both regenerated sentences back onto stack (in returned order)
3. When stack is empty, output queue contains all simplified sentences
4. Run Anaphoric Post-Processor on the output queue
5. Output final simplified text
```

**Transform application order within a sentence is top-down**: outermost constructs (e.g., main clause conjunction) are simplified before inner embedded constructs (e.g., relative clauses within a conjunct). This ensures adjacency preservation — clauses that were adjacent in the original remain adjacent in the output.

### 1.3 Regeneration Stage

Two modules, invoked at different times:

**Module A — Conjunctive Cohesion**: Called as a **subroutine after each individual transform**. Handles:
- Sentence ordering (constraint satisfaction)
- Cue-word selection
- Determiner choice
- Referring expression generation

**Module B — Anaphoric Cohesion**: Called as a **post-process after ALL transforms are complete**. Handles:
- Detection of broken pronominal links
- Replacement of broken pronouns with referring expressions

---

## Part 2: The Seven Core Transformation Rules

The original system (2003/2006) uses **7 rules**: 3 for conjunction, 2 for relative clauses, 2 for apposition.

Every rule produces a triplet **(a, R, b)** where:
- **a** = nucleus sentence
- **R** = discourse relation
- **b** = satellite sentence

The relation R determines sentence ordering, cue-word selection, and determiner behaviour.

### Rule 1: Clausal Coordination

**Trigger**: Two full clauses joined by a coordinating or subordinating conjunction.

**Pattern**: `[S1 CONJ S2]`

**Action**: Split into two independent sentences. The conjunction is removed from the text and replaced by a regenerated cue word (see Part 6).

**Relation assignments**:

| Conjunction(s) | Relation | Notes |
|---|---|---|
| `although, though, whereas, but, however` | (a, Concession, b) | |
| `or, or else` | (a, Anti-Conditional, b) | |
| `because` | (a, Cause, b) -> converted to (b, Result, a) | Clause order reversed |
| `and` | (a, And, b) | Underspecified relation |
| `when` | (a, When, b) | |
| `before` | (a, Before, b) | |
| `after` | (a, After, b) | |
| `since` | (a, Since, b) | Ambiguous cause/temporal — left underspecified |
| `as` | (a, As, b) | Ambiguous cause/temporal — left underspecified |
| `while` | (a, While, b) | |
| `if` | (a, If, b) | |
| `unless` | (a, Unless, b) | |
| `so that` | (a, So-that, b) | Purpose |
| `in order to` | (a, In-order-to, b) | Purpose |

**Design principle**: Ambiguous conjunctions (`as`, `since`, `and`) are NOT disambiguated. The system transfers the ambiguity from input to output by defining its own underspecified relations, avoiding the need for rhetorical analysis.

**Examples**:
```
IN:  "Though all these politicians avow their respect for genuine cases, it's the tritest lip service."
OUT: "All these politicians avow their respect for genuine cases. But, it's the tritest lip service."

IN:  "The federal government suspended sales of U.S. savings bonds because Congress hasn't lifted the ceiling on government debt."
OUT: "Congress hasn't lifted the ceiling on government debt. So, the federal government suspended sales of U.S. savings bonds."
```

Note: `because` triggers clause-order reversal — the cause clause comes first, the result clause second with cue `So,`.

### Rule 2: VP Coordination

**Trigger**: A single subject shared across coordinated verb phrases.

**Pattern**: `[NP VP1 CONJ VP2]`

**Action**: Split into two sentences, duplicating the subject NP. Second mention uses a referring expression.

**Relation**: Same as for clausal coordination (determined by conjunction).

**Example**:
```
IN:  "Mr. Anthony decries program trading and runs an employment agency."
OUT: "Mr. Anthony decries program trading. Mr. Anthony runs an employment agency."
```

For multi-way coordination (`VP1, VP2, and VP3`), apply recursively.

### Rule 3: Subordination

**Trigger**: A main clause with an attached adverbial subordinate clause.

**Pattern**: `[S, SUBCONJ S]` or `[SUBCONJ S, S]`

**Action**: Split into two sentences. The subordinating conjunction is removed and replaced by a sentence-initial cue word (see Part 6).

**Relation**: Determined by the specific subordinating conjunction (see Rule 1 table).

This is essentially the same mechanism as Rule 1, but distinguished because subordinate clauses may appear sentence-initially (`Although X, Y`) or sentence-finally (`Y, although X`), requiring position-aware splitting.

### Rule 4: Non-Restrictive Relative Clause

**Trigger**: Non-restrictive (parenthetical) relative clause, identified by:
- Surrounding commas
- Relative pronoun (`who`, `which`, `whom`, `whose`)
- Attachment analysis resolves which NP the clause modifies

**Pattern**: `[... NP, REL-PRONOUN VP, ...]`

**Relation**: **(a, Elaboration, b)** — the extracted clause (b) is the satellite, the main clause (a) is the nucleus.

**Action**:
1. Extract the relative clause content into a new sentence
2. Replace the relative pronoun with a referring expression for the antecedent NP
3. The antecedent NP is duplicated: once in the main clause (unchanged), once as subject of the new sentence (with definite determiner `this/these`)

**Example**:
```
IN:  "A former ceremonial officer, who was at the heart of Whitehall's patronage machinery, said there should be a review of the honours list."
OUT: "A former ceremonial officer said there should be a review of the honours list. This officer was at the heart of Whitehall's patronage machinery."
```

**Sentence order**: Determined by the ordering algorithm (Part 5). The soft constraint for Elaboration prefers main clause first, but this can be overridden by connectedness considerations.

### Rule 5: Restrictive Relative Clause

**Trigger**: Restrictive relative clause (no surrounding commas), identified by:
- No commas around the clause
- Relative pronoun or complementizer (`who`, `which`, `that`, or zero relative)
- Attachment analysis

**Pattern**: `[... NP REL-PRONOUN/that VP ...]`

**Relation**: **(a, Identification, b)** — a referential relation (not conjunctive). The clause's purpose is to identify a member from a larger set.

**Action**:
1. Extract relative clause content into new sentence
2. Replace relative pronoun with referring expression
3. **Critical determiner rule**: The original NP gets an **indefinite** determiner (`a/some`) to introduce the referent. The second mention gets a **definite** determiner (`this/these`).

**Example**:
```
IN:  "The man who had brought it in for an estimate returned to collect it."
OUT: "A man had brought it in for an estimate. This man returned to collect it."
```

**Exceptions to indefinite determiner**:
- **Numerical attributes present**: "He was involved in two conversions which turned out to be crucial." -> "He was involved in two conversions. These conversions turned out to be crucial." (no `a/some`)
- **Proper nouns**: No determiner change. "Mr. Anthony who runs..." -> "Mr. Anthony runs..."
- **Adjectival pronouns present**: No determiner change. "his latest book which..." -> "his latest book..."

**Sentence order**: Soft constraint for Identification prefers the identifying clause first (b before a), so the referent is established before being used.

### Rule 6: Non-Restrictive Appositive

**Trigger**: Non-restrictive appositive phrase, identified by:
- Surrounding commas
- NP followed by another NP in apposition

**Pattern**: `[... NP, NP-APPOSITIVE, VP ...]`

**Relation**: **(a, Elaboration, b)**

**Action**:
1. Extract appositive into new sentence
2. Insert copular verb (`is/was`, matching tense of main clause)
3. Antecedent NP becomes subject, appositive becomes predicate

**Example**:
```
IN:  "Garret Boone, who teaches art at Earlham College, calls the new structure 'just an ugly bridge' and one that blocks the view of a new park below."
OUT: "Garret Boone teaches art at Earlham College. Garret Boone calls the new structure 'just an ugly bridge' and one that blocks the view of a new park below."
```

### Rule 7: Restrictive Appositive

**Trigger**: Restrictive appositive (no commas, typically titles or roles).

**Pattern**: `[TITLE NP VP ...]` or `[NP NP-APPOSITIVE VP ...]`

**Relation**: **(a, Identification, b)**

**Action**: Same as Rule 6 but with Identification semantics (affecting determiner choice and sentence order preference).

---

## Part 3: Extended Rules in RegenT (2011)

RegenT extended the system to **63 transformation rules** specified in XML files, operating on Stanford typed dependencies. The rules cover five categories: coordination, subordination, relative clauses, apposition, and passive-to-active voice conversion.

### 3.1 RegenT Transfer Rule Format

Each rule specifies five components:

**CONTEXT**: List of typed dependency relations that must be unifiable with the input for the rule to fire. May include relations beyond what gets deleted (i.e., conditions that must hold but aren't modified).

**DELETE**: List of typed dependency relations to remove from the dependency graph.

**INSERT**: List of new typed dependency relations to add.

**ORDERING**: For nodes where the transform has reordered subtrees, the new traversal order. Format: `Node ??X0: [??X3, ??X0, ??X1]`. Only needed for reordered nodes. All other nodes use original word order from the input.

**NODE-OPERATIONS**: Two types:

- **Lexical substitution**: Change morphology of a node.
  - `Node ??X0: Get tense from ??X2 and number agreement from ??X3`
  - `Node ??Xn: Suffix="ing"`
- **Node deletion**: Remove a node from the tree, reassigning its dependents to a specified parent.
  - `Node ??X2: Target=??X0` (move ??X2's dependents to ??X0, then delete ??X2)

### 3.2 Passive -> Active Voice Conversion

**Typed dependency pattern**:
```
CONTEXT & DELETE:
  nsubjpass(??X0, ??X1)    — passive subject
  auxpass(??X0, ??X2)      — passive auxiliary (was/were/is/are/been)
  agent(??X0, ??X3)        — agent (the "by" phrase)

INSERT:
  nsubj(??X0, ??X3)        — agent becomes active subject
  dobj(??X0, ??X1)         — passive subject becomes direct object

ORDERING:
  Node ??X0: [??X3, ??X0, ??X1]   — subject, verb, object

NODE-OPERATIONS:
  Node ??X0: Get tense from ??X2, number agreement from ??X3
```

**Example**:
```
IN:  "The cat was chased by the dog."
     det(cat-2, The-1)
     nsubjpass(chased-4, cat-2)
     auxpass(chased-4, was-3)
     det(dog-7, the-6)
     agent(chased-4, dog-7)

OUT: "The dog chased the cat."
     det(dog-7, the-6)
     nsubj(chased-4, dog-7)
     det(cat-2, The-1)
     dobj(chased-4, cat-2)
```

**Variants** (each a separate rule):

| Variant | Input pattern | Notes |
|---|---|---|
| Simple passive | `NP was VPed by NP` | Standard case |
| Modal passive | `NP can/could/should/will be VPed by NP` | Tense from modal |
| Perfect passive | `NP has/had been VPed by NP` | Tense from perfective auxiliary |
| Passive with retained object | `NP was given X by NP` | Ditransitive: indirect object promoted |
| Agentless passive | `NP was VPed` (no `by` phrase) | **No conversion** — no agent to promote to subject |

### 3.3 Relative Clause (Dependency Version)

**Typed dependency pattern**:
```
CONTEXT & DELETE:
  rcmod(??X0, ??X1)        — relative clause modifier
  nsubj(??X1, ??X0)        — subject of relative clause is the antecedent

INSERT:
  nsubj(??X1, ??X0)        — antecedent becomes subject of independent clause
```

Removes the `rcmod` embedding relation when the relative clause has the antecedent as its subject. The result is two independent dependency trees (two sentences).

**Referring expression post-processing**: When the antecedent NP is duplicated, a post-processor generates a referring expression for the second mention (head noun + definite article or title).

### 3.4 Complex Lexico-Syntactic Reformulations (Siddharthan 2010)

These go beyond simple splitting and involve structural rewriting:

**Causality reformulation**:
```
IN:  "The cause of the explosion was an incendiary device."
OUT: "The explosion occurred because of an incendiary device."
```

**Nominalisation unpacking**:
```
IN:  "The destruction of the city by the army..."
OUT: "The army destroyed the city."
```

These require:
- Node deletion (removing copular verbs, prepositions)
- Lexical substitution (suffix changes: `-tion` -> verb form, `-ed` -> `-ing`)
- Subtree reordering
- Insertion of new function words

### 3.5 Full Category Breakdown of 63 Rules

| Category | Constructs handled | Approx. count |
|---|---|---|
| Clausal coordination | `and`, `but`, `or`, `yet`, semicolons, multi-way | ~10 |
| VP coordination | Shared subject with coordinated VPs, multi-way | ~5 |
| Subordination | `because`, `although/though`, `when`, `while`, `after`, `before`, `since`, `as`, `if`, `unless`, `so that`, `in order to` | ~15 |
| Non-restrictive relative clauses | `who`, `which`, `whom`, `whose` with commas | ~5 |
| Restrictive relative clauses | `who`, `which`, `that`, zero relative without commas | ~5 |
| Reduced relative clauses | Present participle (`NP VP-ing`), past participle (`NP VP-ed`), infinitival (`NP to VP`) | ~3 |
| Non-restrictive apposition | `NP, NP-appositive, VP`; parenthetical `NP (NP) VP` | ~5 |
| Restrictive apposition | Title/role appositives | ~3 |
| Passive -> active | Simple, modal, perfect, ditransitive variants | ~5 |
| Complex lexico-syntactic | Causality reformulation, nominalisation unpacking, other structural rewrites | ~7 |

---

## Part 4: The 278 + 5,172 Rule Hybrid System (2014)

Siddharthan & Mandya (2014) augmented the hand-crafted rules with automatically acquired lexicalised rules.

### 4.1 Hand-Crafted Rules (278)

These are the generalised syntactic rules — the 63 from RegenT expanded with additional variants for edge cases, punctuation patterns, multi-clause constructions, and interaction effects between constructs.

### 4.2 Automatically Acquired Rules (5,172)

**Source**: Parallel Wikipedia / Simple Wikipedia corpus (Woodsend & Lapata 2011 dataset).

**Method**: Synchronous Dependency Insertion Grammars (Ding & Palmer 2005). Complex and simple sentence pairs are parsed, aligned at the dependency level, and transformation rules extracted.

**Format**: Same RegenT format (CONTEXT/DELETE/INSERT/ORDERING/NODE-OPERATIONS), but **lexicalised** — they include specific words, not just dependency relation types.

**Example**:
```
Rule: Replace "described as" with "called"

IN:  "Coulter was described as a polemicist"
OUT: "Coulter was called a polemicist"

Also handles non-adjacent: 
IN:  "Coulter has described herself as a polemicist"
OUT: "Coulter has called herself a polemicist"
```

### 4.3 Rule Generalisation

Automatically acquired rules are generalised:
- **WordNet expansion**: Expand lexical context to include synonyms and related words, so a rule learned for "described" also applies to "characterised", "portrayed", etc.
- **Context removal**: For substitutions that are context-independent, remove the lexical context requirement so the rule applies universally.

### 4.4 The Reluctant Trimmer (Sentence Compression)

The 2014 system also includes a sentence compression module that removes peripheral information when sentences remain too long after syntactic simplification. It uses an LP solver (lp_solve 5.5.2.0) to optimise compression subject to grammaticality constraints. This is a **separate** module from the syntactic simplification rules and operates as a post-process.

---

## Part 5: Sentence Ordering Algorithm

Called after each transform application. The core algorithm for preserving conjunctive cohesion.

### 5.1 Inputs

The algorithm receives:
1. A triplet **(a, R, b)**: simplified sentences a (nucleus) and b (satellite), and relation R
2. A set **C** of inherited constraints from previously applied transforms

### 5.2 Outputs

The algorithm returns:
1. Ordered pair (either (a,b) or (b,a))
2. Constraint sets **Ca** and **Cb** to be inherited by future transforms on sentences a and b respectively

### 5.3 Constraint Generation by Relation

Each relation R adds constraints to the sets C (current ordering), Ca (constraints inherited by a), and Cb (constraints inherited by b).

**Most relations** — Concession, Anti-Conditional, And, When, Before, After, Since, As, While, If, Unless, So-that, In-order-to:
```
HARD in C:   a precedes b
SOFT in Ca:  nucleus is last    (ensures continued adjacency with b)
SOFT in Cb:  nucleus is first   (ensures continued adjacency with a)
```

**Cause** (converted to Result):
```
HARD in C:   b precedes a       (clause order REVERSED)
SOFT in Ca:  nucleus is first
SOFT in Cb:  nucleus is last
```

**Elaboration** (non-restrictive relative clauses and appositives):
```
SOFT in C:   a precedes b       (satellite after nucleus, but overridable)
```
This is soft because parenthetical information can be positioned flexibly without making the text misleading — only the style changes (elaborative vs. narrative).

**Identification** (restrictive relative clauses):
```
SOFT in C:   b precedes a       (identify referent before using it)
```
This is soft because it represents a preference, not a logical requirement.

### 5.4 The Full Algorithm

```
ALGORITHM Order-Sentences((a, R, b), C):

1. INITIALISE: Create Ca and Cb, copying any inherited constraints from C
   that are relevant to a and b respectively.

2. PROCESS R: Add new constraints to C, Ca, Cb as specified in §5.3.

3. CHECK HARD CONSTRAINTS (a before b):
   IF C contains hard constraints: (a->b) or ("a is first") or ("b is last")
   THEN:
     IF no conflicting hard constraints exist:
       RETURN order=(a, b), Ca, Cb
     ELSE:
       RETURN fail (abort this transform)

4. CHECK HARD CONSTRAINTS (b before a):
   IF C contains hard constraints: (b->a) or ("b is first") or ("a is last")
   THEN:
     IF no conflicting hard constraints exist:
       RETURN order=(b, a), Cb, Ca
     ELSE:
       RETURN fail

5. CONNECTEDNESS CHECK (centering theory):
   IF a = "...W..." and b = "W [extracted clause content]"
   (i.e., a contains NP W in non-subject position, b has W as subject)
   THEN:
     — Order (a, b) preserves center-continuation
     Add SOFT to Ca: nucleus is last
     Add SOFT to Cb: nucleus is first
     RETURN order=(a, b), Ca, Cb

6. ADJACENCY PRESERVATION:
   IF a can be simplified further
   OR C contains soft constraints suggesting (b, a) with no conflicts
   THEN:
     — Placing a second keeps it adjacent to future splits
     Add SOFT to Ca: nucleus is first
     Add SOFT to Cb: nucleus is last
     RETURN order=(b, a), Cb, Ca

7. DEFAULT:
     Add SOFT to Ca: nucleus is last
     Add SOFT to Cb: nucleus is first
     RETURN order=(a, b), Ca, Cb
```

### 5.5 Connectedness Heuristic (Step 5) — Detailed

Justified by centering theory (Grosz, Joshi & Weinstein 1995): the backward-looking center Cb is unlikely to be inside a relative clause. So the main clause should come first to preserve center-continuation.

```
PREFERRED:
  "(1) They will remain on a lower-priority list. (2) This list includes 17 other countries."
  — center-continuation (to sentence 1), then center-retaining (to sentence 2)

DISPREFERRED:
  "(1) A lower-priority list includes 17 other countries. (2) They will remain on this list."
  — center-shift to sentence 1 (disruptive)
```

**Implementation note**: Actually computing backward-looking centers would require full coreference resolution (not just pronouns but also definite descriptions). The heuristic avoids this by using a simpler structural test: does the shared NP appear as subject of b? If so, order (a, b).

### 5.6 Adjacency Preservation (Step 6) — Detailed

When sentence a contains further simplifiable constructs, placing it second risks interleaving:

```
GOOD (a second, will be split further):
  "The agency is funded through insurance premiums. The agency insures pension benefits for 30 million workers. These workers take part in single-employer pension plans."

BAD (a first, then split, with b stranded):
  "The agency insures pension benefits for 30 million workers. These workers take part in single-employer pension plans. The agency is funded through insurance premiums."
```

Local ordering with adjacency preservation is equivalent to global ordering with adjacency encoded as a hard constraint, but vastly more efficient (exponentially smaller search space).

---

## Part 6: Cue-Word Selection

### 6.1 Complete Mapping Table

| Original conjunction(s) | Assigned relation | Regenerated cue word(s) | Position |
|---|---|---|---|
| `although, though, whereas, but, however` | Concession | `But,` | Start of sentence 2 |
| `or, or else` | Anti-Conditional | `Otherwise,` | Start of sentence 2 |
| `because` | Cause -> Result | `So,` | Start of sentence 2 (= original main clause) |
| `and` | And | `And` (or nothing) | Start of sentence 2 |
| `when` | When | `This was/is when` | Start of sentence 2 |
| `before` | Before | `This was/is before` | Start of sentence 2 |
| `after` | After | `This was/is after` | Start of sentence 2 |
| `since` | Since | `This is/was since` | Start of sentence 2 |
| `as` | As | `This is/was as` | Start of sentence 2 |
| `while` | While | `This is/was while` | Start of sentence 2 |
| `if` | If | `This is/was if` | Start of sentence 2 |
| `unless` | Unless | `This is/was unless` | Start of sentence 2 |

### 6.2 Tense Selection for `This is/was CONJ`

The auxiliary `is/was` is determined from the tense of the **nucleus** clause:

```
IN:  "Kenya was the scene of a major terrorist attack on August 7 1998, when a car bomb blast outside the US embassy in Nairobi killed 219 people."
OUT: "Kenya was the scene of a major terrorist attack on August 7 1998. This was when a car bomb blast outside the US embassy in Nairobi killed 219 people."
     — "was" because nucleus is past tense

IN:  "A more recent novel, 'Norwegian Wood', has sold more than four million copies since Kodansha published it in 1987."
OUT: "A more recent novel, 'Norwegian Wood', has sold more than four million copies. This is since Kodansha published it in 1987."
     — "is" because nucleus is present perfect (non-past)
```

### 6.3 Design Principles

1. **Use simple cue words**: `so` and `but` rather than `therefore`, `hence`, `however`. Williams, Reiter & Osman (2003) showed faster reading times with simpler cue words.
2. **Include punctuation**: Comma after cue word (`But,` / `So,`). Punctuation with cue words yielded faster reading times.
3. **Sentence-initial position**: All introduced cue words go at the start of the second sentence.
4. **Ambiguity transfer**: Ambiguous conjunctions (`as`, `since`, `and`) are replaced with cue words that preserve the ambiguity. No rhetorical disambiguation is attempted.

---

## Part 7: Determiner Choice Rules

### 7.1 Non-Restrictive Case (Elaboration Relation)

The antecedent NP in the main clause is **unchanged**. The referring expression in the extracted sentence gets a **definite** determiner:

| Antecedent number | Determiner in referring expression |
|---|---|
| Singular | `this` |
| Plural | `these` |

```
IN:  "A former ceremonial officer, who was at the heart of Whitehall's patronage machinery, said..."
OUT: "A former ceremonial officer said... This officer was at the heart of Whitehall's patronage machinery."
```

### 7.2 Restrictive Case (Identification Relation)

The antecedent NP gets an **indefinite** determiner to introduce the referent:

| Context | Determiner on antecedent |
|---|---|
| Singular countable | `a` |
| Plural countable | `some` |

The referring expression gets a **definite** determiner (`this/these`):

```
IN:  "The man who had brought it in for an estimate returned to collect it."
OUT: "A man had brought it in for an estimate. This man returned to collect it."
```

### 7.3 Exceptions (No Determiner Change)

| Condition | Behaviour | Example |
|---|---|---|
| **Numerical attribute present** | Keep original determiner | "two conversions which..." -> "two conversions. These conversions..." |
| **Proper noun as head** | No determiner added/changed | "Mr. Anthony who..." -> "Mr. Anthony..." |
| **Adjectival pronoun present** | No determiner added/changed | "his latest book which..." -> "his latest book..." |
| **Demonstrative already present** | No change | "that policy which..." -> "that policy..." |

---

## Part 8: Referring Expression Generation

When a transformation duplicates an NP (splitting creates one occurrence in each output sentence), a referring expression must be generated for the second mention.

### 8.1 Algorithm

Based on Siddharthan & Copestake (2004):

- **Lexicalised incremental approach** using WordNet
- Uses synonym and antonym sets for open-domain disambiguation
- Allows incremental incorporation of relational attributes
- Does NOT require a pre-defined attribute classification scheme
- Works in open domains (not restricted to a closed set of attributes)

### 8.2 Content Selection Principles

**Include enough information to disambiguate, no more**:

```
GOOD:    "The Chicago report precedes the full report and gives an indication..."
         (Minimal distinguishing modifier retained)

TOO MUCH: "The report by Chicago purchasing agents precedes the full purchasing agents report and gives an indication..."
          (Stilted, conveys unwanted conversational implicatures)

TOO LITTLE: "The report precedes the report and gives an indication..."
            (Ambiguous — which report?)
```

### 8.3 Default Referring Expression Strategy

For most cases, the referring expression consists of:
- **Head noun** of the antecedent NP
- **Definite article** (`the`) or **demonstrative** (`this/these`)
- **Title** if applicable (`Mr`, `President`, `Dr`)

More complex disambiguation (adding modifiers) is invoked only when the default would be ambiguous.

### 8.4 Animacy Information

WordNet is used to determine animacy of head nouns:
- Animate -> pronoun candidates: `he/she/they`
- Inanimate -> pronoun candidates: `it/they`

Animacy information is used for both referring expression generation and pronoun resolution.

---

## Part 9: Anaphoric Post-Processor

### 9.1 Purpose

Syntactic restructuring changes the order of textual units, which can alter the attentional state (reader's focus of attention) at points where pronouns occur. If the attentional state has changed such that a pronoun would be resolved differently, the pronoun must be replaced with an explicit referring expression.

**Key constraint**: The post-processor only **replaces** pronouns. It never **introduces** pronouns. This is conservative — people who benefit from simplification often also have difficulty resolving pronouns.

### 9.2 The Algorithm

```
ALGORITHM Fix-Pronominal-Links:

FOR every pronoun P in the simplified text (excluding "it"):
  1. Compute salience list S_orig at P's position in the original text
  2. Compute salience list S_simp at P's position in the simplified text
  3. Find immediate antecedent of P in original text -> A_orig_imm
  4. Find absolute antecedent of P in original text -> A_orig_abs
  5. Find immediate antecedent of P in simplified text -> A_simp_imm
  6. Find absolute antecedent of P in simplified text -> A_simp_abs
  
  7. IF (A_simp_imm != A_orig_imm) AND (A_simp_abs != A_orig_abs):
       REPLACE P with referring expression for A_orig_abs
```

### 9.3 Three Levels of Preservation

The algorithm can be configured to three strictness levels by changing the condition in step 7:

| Level | Condition for replacement | Effect |
|---|---|---|
| **Cohesion-preserving** | Most salient entity at pronoun differs between texts | Most replacements (68 in test), most errors (19) |
| **Coherence-preserving** | Absolute antecedent differs | Moderate (17 replacements, 5 errors) |
| **Local-coherence-preserving** | Both immediate AND absolute antecedent differ | Fewest (11 replacements, 3 errors) |

**Recommended setting**: Local-coherence-preserving. Minimal intervention, fewest introduced errors, and sufficient to prevent misresolution.

### 9.4 Why Exclude `it`

In newspaper text, approximately **85% of occurrences of `it` are non-anaphoric** (pleonastic `it`, cleft constructions, etc.). Attempting to resolve and replace these would introduce far more errors than it would fix.

### 9.5 Salience Model

The salience list is computed by the pronoun resolution component (Siddharthan 2003b). It ranks discourse entities by recency, grammatical function, and other salience factors. The key comparison is whether the **same entity** is most salient at the pronoun position in both texts.

**Example**:
```
Original salience at "he": [Mr Blunkett, security breach, comedian, Prince William, ...]
Simplified salience at "he": [this breach, comedian, Prince William, ..., Mr Blunkett, ...]
-> "he" would resolve to "breach" (animate mismatch would catch this, but still — salience has shifted)
-> REPLACE "he" with "Mr Blunkett"
```

### 9.6 Attentional State Restoration

Replacing pronouns with full NPs has a side effect: it partially **restores** the attentional state in the simplified text. After replacing "he" with "Mr Blunkett", Mr Blunkett's salience increases in subsequent text, bringing the attentional state closer to the original.

---

## Part 10: N-Best Parse Ranking (RegenT 2011)

RegenT can operate on the top-1 parse or the n-best parses. In the n-best setting, all n parses are simplified, producing n candidate outputs, and the best is selected.

### 10.1 Problem

Most simplification errors trace back to incorrect parses. Example: the parser identifies "which housed" as a relative clause and treats "villas" and "blocks" as verbs, producing garbled output.

### 10.2 Ranking: Penalty Patterns

Deduct one point for each:

1. Sentence ends in a subject pronoun, preposition, or conjunction
2. Word repetition within a sentence ("is is", "to to")
3. Preposition followed by subject pronoun ("of he", "for she")
4. Bad conjunction/preposition sequences ("because but", "until for")
5. Very short sentence (<= 4 words)
6. Output has far fewer words than input (excessive deletion suggests misparse)

### 10.3 Ranking: Positive Signals

1. **Bigram and trigram overlap** with original sentence (as fraction) — higher overlap means less disruption
2. **Number of output sentences** — more sentences means more simplification was applied (encourages rule application)
3. **Top-parse bonus** — if the simplification came from the highest-ranked parse, add a bonus (most likely to be correct)

### 10.4 Results

Using 50-best parses improved:
- Transforms per sentence: 0.65 -> 0.74 (+14%)
- Sentences modified: 50.2% -> 55.4% (+5%)
- Accuracy (gen-light): 83.9% -> 87.9% (+4%)

---

## Part 11: Generation Strategies

RegenT offers two generation approaches from transformed dependency graphs.

### 11.1 Gen-Light (Recommended)

**Principle**: Reuse word order and morphology from the original sentence wherever possible. Encode any required changes (reordering, inflection) directly in the transformation rules.

**Process**:
1. After transform, traverse the dependency tree
2. For nodes without an ORDERING specification, use original word positions
3. For nodes with an ORDERING specification, use the specified subtree order
4. Apply any NODE-OPERATIONS (lexical substitution, node deletion)

**Advantages**: Robust to parser errors. Even when the parse is wrong, reusing the original word order produces readable output.

**Disadvantage**: Transformation rules are more complex to write (must encode ordering and morphology changes).

### 11.2 Gen-Heavy

**Principle**: Convert transformed dependencies to DSyntS (Deep Syntactic Structure) and use RealPro surface realiser for all generation decisions.

**Process**:
1. After transform, convert Stanford dependencies to DSyntS notation
2. Map 52 Stanford dependency types to 7 DSyntS types (I, II, III, IV, ATTR, DESC-ATTR, APPEND)
3. Extract morphological features (tense, voice, aspect, mood, number, person, gender) from POS tags, auxiliaries, pronouns
4. Feed DSyntS to RealPro for linearisation

**Advantages**: Simpler transformation rules (no ordering/morphology specifications needed). Potentially better for automatically learned rules.

**Disadvantage**: Brittle. Misparses cause RealPro to produce unacceptable word orders. Users are intolerant of suboptimal word ordering.

### 11.3 Recommended Hybrid

Based on evaluation results: use **gen-heavy for verb features only** (tense, mood, voice, agreement) and **gen-light for everything else** (word/phrase ordering). This gets the best of both approaches.

### 11.4 Performance Comparison

| Setting | Accuracy (developer) | Acceptability (majority vote) |
|---|---|---|
| gen-light, 1 parse | 83.9% | 69% |
| gen-light, 50 parses | 87.9% | 78% |
| gen-heavy, 1 parse | 70.8% | 40% |
| gen-heavy, 50 parses | 77.7% | 43% |

---

## Part 12: Evaluation Results

### 12.1 Correctness (2006 paper, 95 examples from 15 Guardian news reports)

Three judges (native English speakers with computational linguistics background).

| Metric | Unanimous agreement | Majority vote (>=2 of 3) |
|---|---|---|
| Grammatical | 80.0% | 94.7% |
| Meaning-preserving | 85.3% | 94.7% |
| Both grammatical AND meaning-preserving | 67.0% | 88.7% |

**Cohesion scores** (0–3 scale, 3 = no loss):
- Average across all examples: **2.43**
- 41% scored 3 unanimously (no loss)
- 75% averaged above 2 (little or no loss)
- 8% averaged <= 1 (significant incoherence)

**Error sources**: Most meaning-altering errors came from incorrect relative clause attachment in the analysis module, not from the transformation or regeneration rules.

### 12.2 Sentence Ordering Utility

18% of examples (17/95) received a different order from the sentence ordering algorithm than a baseline preserving original clause order. On these 17 examples, the average cohesion score was **2.47** (higher than the overall average), confirming the algorithm's decisions were beneficial.

### 12.3 Anaphoric Post-Processor

Of 95 simplified sentences:
- 68 pronouns had altered attentional state
- Only 17 had different absolute antecedents (agreement/binding constraints resolved most)
- Only 11 needed replacement (local-coherence-preserving level)
- 3 errors introduced by replacement
- **Net effect**: ~1 in 10 simplifications needs pronoun replacement

### 12.4 RegenT Performance (2011, 175 sentences from BBC/Guardian/Sun)

| System | Avg sent length | Transforms/sent | % sents modified | Accuracy |
|---|---|---|---|---|
| Original text | 20.9 | — | — | — |
| gen-light, 1 parse | 15.3 | 0.65 | 50.2% | 83.9% |
| gen-light, 50 parses | 14.3 | 0.74 | 55.4% | 87.9% |
| gen-heavy, 1 parse | 14.8 | 0.65 | 50.2% | 70.8% |
| gen-heavy, 50 parses | 14.0 | 0.74 | 55.4% | 77.7% |

### 12.5 Readability Improvement (2006, 15 reports per source)

| News source | Flesch Reading Ease (orig -> simp) | Avg sentence length (orig -> simp) |
|---|---|---|
| Wall Street Journal | ~40 -> ~55 | ~25 -> ~15 |
| Guardian | ~45 -> ~60 | ~23 -> ~15 |
| New York Times | ~42 -> ~57 | ~24 -> ~15 |
| Cambridge Evening News | ~52 -> ~65 | ~20 -> ~15 |
| Daily Mirror | ~55 -> ~68 | ~18 -> ~15 |
| BBC News Online | ~54 -> ~67 | ~20 -> ~15 |

The system converges average sentence length to approximately **15 words** regardless of source complexity.

---

## Part 13: Known Limitations and Edge Cases

### 13.1 Conflict Between Conjunctive and Anaphoric Cohesion

Sometimes the ordering that preserves conjunctive cohesion (rhetorical relations) disrupts anaphoric cohesion (pronoun resolution), and vice versa. The system prioritises conjunctive cohesion (correct ordering) and relies on the anaphoric post-processor to fix any resulting pronoun issues.

```
IN:  "Dr. Knudson found that some children with the eye cancer had inherited a damaged copy of chromosome No. 13 from a parent, who had necessarily had the disease."

Ordering for conjunctive cohesion (elaboration -> satellite second):
OUT: "Dr. Knudson found that some children with the eye cancer had inherited a damaged copy of chromosome No. 13 from a parent. This parent had necessarily had the disease."
-> This disrupts attentional state: "parent" becomes most salient, so "he" in the next sentence is ambiguous.
-> Post-processor detects this and replaces "he" with "Dr. Knudson".
```

### 13.2 Semantic-Level Coherence Loss

Some coherence loss cannot be repaired by syntactic means. Relative clauses, appositives, and conjunctions are **cohesive devices** — removing them inherently affects discourse structure. Judges sometimes could not propose any rewrite that preserved the subtleties of the original.

### 13.3 Analysis Module Errors

The most serious simplification errors come from incorrect clause boundary identification or attachment in the analysis stage:

```
IN:  "They paid cash for the vehicle, which was in 'showroom' condition."
WRONG: "They paid cash for the vehicle. This cash was in 'showroom' condition."
       (Relative clause attached to "cash" instead of "vehicle")
```

These are analysis errors, not transformation or regeneration errors. Improving the parser/analyser directly reduces error rate.

### 13.4 Inherited Ungrammaticality

Some output ungrammaticality is inherited from the input (bad punctuation in the source text). The system does not attempt to fix pre-existing grammatical errors.

### 13.5 Pleonastic `it`

The pronoun `it` is excluded from the anaphoric post-processor because 85% of occurrences in news text are non-anaphoric. A more sophisticated system could distinguish anaphoric from pleonastic `it`, but the current approach is conservative and effective.

---

## Part 14: Dependency Specifications for All Major Rules

This section provides the typed dependency patterns for all major rule categories in RegenT format.

### 14.1 Clausal Coordination with `and`

```
CONTEXT & DELETE:
  conj(??X0, ??X1)          — X1 is coordinated with X0
  cc(??X0, and-??)          — conjunction "and"

INSERT:
  root(ROOT, ??X1)          — X1 becomes root of new sentence

NODE-OPERATIONS:
  — If X0 and X1 share a subject via nsubj, duplicate the subject for X1
```

### 14.2 Clausal Coordination with `but`

```
CONTEXT & DELETE:
  conj(??X0, ??X1)
  cc(??X0, but-??)

INSERT:
  root(ROOT, ??X1)

CUE-WORD:
  Prepend "But, " to sentence containing X1

RELATION: (a, Concession, b)
```

### 14.3 Subordination with `because`

```
CONTEXT & DELETE:
  advcl(??X0, ??X1)         — adverbial clause modifier
  mark(??X1, because-??)    — subordinating conjunction

INSERT:
  root(ROOT, ??X1)          — subordinate clause becomes independent

CUE-WORD:
  Prepend "So, " to sentence containing X0

RELATION: (a, Cause, b) -> rewritten as (b, Result, a)
ORDER: b precedes a (reversed)
```

### 14.4 Subordination with `although/though`

```
CONTEXT & DELETE:
  advcl(??X0, ??X1)
  mark(??X1, although-??/though-??)

INSERT:
  root(ROOT, ??X1)

CUE-WORD:
  Prepend "But, " to sentence containing X1

RELATION: (a, Concession, b)
ORDER: a precedes b
```

### 14.5 Temporal Subordination (`when`)

```
CONTEXT & DELETE:
  advcl(??X0, ??X1)
  mark(??X1, when-??)

INSERT:
  root(ROOT, ??X1)

CUE-WORD:
  Prepend "This was/is when " to sentence containing X1
  (tense from X0)

RELATION: (a, When, b)
ORDER: a precedes b
```

Pattern is identical for `while`, `before`, `after`, `since`, `as`, `if`, `unless` — only the mark word and cue-word differ.

### 14.6 Non-Restrictive Relative Clause

```
CONTEXT & DELETE:
  rcmod(??X0, ??X1)         — relative clause modifier
  nsubj(??X1, ??REL)        — subject of relative clause (the relative pronoun)
  
  WHERE: ??REL matches who/which/whom/that AND commas surround the clause

INSERT:
  nsubj(??X1, ??X0)         — antecedent becomes subject of independent clause
  root(ROOT, ??X1)

REFERRING EXPRESSION:
  Generate referring expression for ??X0 in the new sentence
  Determiner: this/these (definite)

RELATION: (a, Elaboration, b)
```

### 14.7 Restrictive Relative Clause

```
CONTEXT & DELETE:
  rcmod(??X0, ??X1)
  nsubj(??X1, ??REL)
  
  WHERE: no commas surround the clause

INSERT:
  nsubj(??X1, ??X0)
  root(ROOT, ??X1)

DETERMINER CHANGES:
  ??X0 in new sentence: indefinite (a/some)
  ??X0 in main sentence: this/these (definite)

RELATION: (a, Identification, b)
```

### 14.8 Non-Restrictive Appositive

```
CONTEXT & DELETE:
  appos(??X0, ??X1)         — appositive modifier
  
  WHERE: commas surround ??X1

INSERT:
  nsubj(??COP, ??X0)        — new copular sentence
  cop(??COP, is/was)         — copular verb (tense from main clause)
  root(ROOT, ??X1)

RELATION: (a, Elaboration, b)
```

### 14.9 Passive -> Active

```
CONTEXT & DELETE:
  nsubjpass(??X0, ??X1)
  auxpass(??X0, ??X2)
  agent(??X0, ??X3)

INSERT:
  nsubj(??X0, ??X3)
  dobj(??X0, ??X1)

ORDERING:
  Node ??X0: [??X3, ??X0, ??X1]

NODE-OPERATIONS:
  Node ??X0: tense from ??X2, agreement from ??X3
  Delete node ??X2
```

### 14.10 Reduced Relative Clause (Present Participle)

```
CONTEXT:
  rcmod(??X0, ??X1)         — relative clause
  WHERE: ??X1 is tagged VBG (present participle)
  AND: no explicit relative pronoun

DELETE:
  rcmod(??X0, ??X1)

INSERT:
  nsubj(??X1, ??X0)
  root(ROOT, ??X1)

NODE-OPERATIONS:
  Node ??X1: Convert VBG to finite form (match tense of main clause)
```

### 14.11 Reduced Relative Clause (Past Participle)

```
CONTEXT:
  rcmod(??X0, ??X1)
  WHERE: ??X1 is tagged VBN (past participle)
  AND: no explicit relative pronoun

DELETE:
  rcmod(??X0, ??X1)

INSERT:
  nsubjpass(??X1, ??X0)     — passive subject (the participle stays passive)
  root(ROOT, ??X1)

NODE-OPERATIONS:
  Node ??X1: Insert appropriate auxiliary (was/were)
```

### 14.12 Iterative Rule Application Example

Multiple rules can be applied to the same dependency set:

```
IN:  "The cat was chased by a dog that was barking."

Dependencies:
  det(cat-2, The-1)
  nsubjpass(chased-4, cat-2)
  auxpass(chased-4, was-3)
  det(dog-7, a-6)
  agent(chased-4, dog-7)
  nsubj(barking-10, dog-7)
  aux(barking-10, was-9)
  rcmod(dog-7, barking-10)

Step 1 — Apply relative clause rule:
  DELETE: rcmod(dog-7, barking-10), nsubj(barking-10, dog-7)
  INSERT: nsubj(barking-10, dog-7)
  -> Now two trees: "chased" tree and "barking" tree

Step 2 — Apply passive->active rule to "chased" tree:
  DELETE: nsubjpass, auxpass, agent
  INSERT: nsubj(chased-4, dog-7), dobj(chased-4, cat-2)

Final output:
  "A dog chased the cat. The dog was barking."
```

---

## Appendix A: Summary of All Relations

| Relation | Source construct | Constraint type | Preferred order | Cue word |
|---|---|---|---|---|
| Concession | `although/though/but/however/whereas` | Hard: a->b | a, b | `But,` |
| Anti-Conditional | `or/or else` | Hard: a->b | a, b | `Otherwise,` |
| Result | `because` (reversed) | Hard: b->a | b, a | `So,` |
| And | `and` | Hard: a->b | a, b | `And` / empty |
| When | `when` | Hard: a->b | a, b | `This was/is when` |
| Before | `before` | Hard: a->b | a, b | `This was/is before` |
| After | `after` | Hard: a->b | a, b | `This was/is after` |
| Since | `since` | Hard: a->b | a, b | `This is/was since` |
| As | `as` | Hard: a->b | a, b | `This is/was as` |
| While | `while` | Hard: a->b | a, b | `This is/was while` |
| If | `if` | Hard: a->b | a, b | `This is/was if` |
| Unless | `unless` | Hard: a->b | a, b | `This is/was unless` |
| Elaboration | Non-restrictive RC / appositive | Soft: a->b | a, b (flexible) | empty |
| Identification | Restrictive RC / appositive | Soft: b->a | b, a (flexible) | empty |

## Appendix B: Summary of Determiner Rules

| Relation | Antecedent NP (1st mention) | Referring expr (2nd mention) |
|---|---|---|
| Elaboration | Unchanged | `this/these` + head noun |
| Identification | Changed to `a/some` (indefinite) | `this/these` + head noun |
| — Exception: numerical attribute | Unchanged | `these` + head noun |
| — Exception: proper noun | Unchanged | Proper noun repeated |
| — Exception: adjectival pronoun | Unchanged | Unchanged |

## Appendix C: Full System Parameters

| Parameter | Recommended value | Source |
|---|---|---|
| Generation strategy | gen-light | 2011 evaluation |
| N-best parses | 50 | 2011 evaluation |
| Anaphoric preservation level | Local-coherence-preserving | 2006 evaluation |
| Pronoun `it` handling | Excluded from post-processor | 85% non-anaphoric in news |
| Cue-word style | Simple (`but`, `so`) | Williams et al. 2003 |
| Target sentence length | ~15 words average | 2006 readability study |
| Transform application order | Top-down within sentence | Adjacency preservation |
| Sentence ordering | Local (per-transform) not global | Efficiency + adjacency |

## Appendix D: References

1. Siddharthan, A. (2003). Syntactic Simplification and Text Cohesion. PhD thesis, University of Cambridge. Technical Report UCAM-CL-TR-597.
2. Siddharthan, A. (2006). Syntactic Simplification and Text Cohesion. Research on Language and Computation, 4(1), 77–109.
3. Siddharthan, A. (2010). Complex Lexico-Syntactic Reformulation of Sentences Using Typed Dependency Representations. Proceedings of INLG 2010, 125–133.
4. Siddharthan, A. (2011). Text Simplification using Typed Dependencies: A Comparison of the Robustness of Different Generation Strategies. Proceedings of ENLG 2011, 2–11.
5. Siddharthan, A. (2014). A Survey of Research on Text Simplification. International Journal of Applied Linguistics, 165(2), 259–298.
6. Siddharthan, A. & Copestake, A. (2004). Generating Referring Expressions in Open Domains. Proceedings of ACL 2004.
7. Siddharthan, A. & Mandya, A. (2014). Hybrid Text Simplification Using Synchronous Dependency Grammars with Hand-Written and Automatically Harvested Rules. Proceedings of EACL 2014, 722–731.
8. Mandya, A. & Siddharthan, A. (2014). Text Simplification Using Synchronous Dependency Grammars: Generalising Automatically Harvested Rules. Proceedings of INLG 2014, 16–25.
9. Williams, S., Reiter, E. & Osman, L. (2003). Experiments with Discourse-Level Choices and Readability. Proceedings of ENLG 2003, 127–134.
10. Grosz, B., Joshi, A. & Weinstein, S. (1995). Centering: A Framework for Modelling the Local Coherence of Discourse. Computational Linguistics, 21(2), 203–226.
11. De Marneffe, M.C., MacCartney, B. & Manning, C.D. (2006). Generating Typed Dependency Parses from Phrase Structure Parses. Proceedings of LREC 2006.
12. Lavoie, B. & Rambow, O. (1997). A Fast and Portable Realizer for Text Generation Systems. Proceedings of ANLP 1997, 265–268.
13. Mann, W.C. & Thompson, S.A. (1988). Rhetorical Structure Theory: Towards a Functional Theory of Text Organization. Text, 8(3), 243–281.
14. Power, R. (2000). Planning Texts by Constraint Satisfaction. Proceedings of COLING 2000, 642–648.
