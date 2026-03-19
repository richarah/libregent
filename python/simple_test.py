#!/usr/bin/env python3
"""
Simple test to verify pyregent installation works
"""

def test_import():
    """Test that pyregent can be imported"""
    try:
        import pyregent
        print("[OK] pyregent imported successfully")
        print(f"  Version: {pyregent.__version__}")
        return True
    except ImportError as e:
        print(f"[FAIL] Failed to import pyregent: {e}")
        print("\nTo install, run:")
        print("  cd python")
        print("  pip install nanobind scikit-build-core")
        print("  pip install .")
        return False

def test_basic_simplification():
    """Test basic simplification"""
    import pyregent

    try:
        # Create simplifier
        simplifier = pyregent.Simplifier()
        print("[OK] Created Simplifier")

        # Simple CoNLL-U sentence
        conllu = """1	The	the	DET	DT	_	2	det	_	_
2	cat	cat	NOUN	NN	_	3	nsubj	_	_
3	slept	sleep	VERB	VBD	_	0	root	_	_
4	and	and	CCONJ	CC	_	3	cc	_	_
5	the	the	DET	DT	_	6	det	_	_
6	dog	dog	NOUN	NN	_	7	nsubj	_	_
7	barked	bark	VERB	VBD	_	3	conj	_	_
8	.	.	PUNCT	.	_	3	punct	_	_
"""

        # Parse
        sentences = pyregent.Simplifier.parse_conllu(conllu)
        print(f"[OK] Parsed {len(sentences)} sentence(s)")

        # Simplify
        result = simplifier.simplify(sentences[0])
        print(f"[OK] Simplified successfully")
        print(f"  Input:  The cat slept and the dog barked.")
        print(f"  Output: {result.text}")
        print(f"  Result: {len(result.sentences)} sentences, {result.transforms_applied} transforms")

        return True
    except Exception as e:
        print(f"[FAIL] Simplification failed: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_configuration():
    """Test configuration options"""
    import pyregent

    try:
        config = pyregent.Config()
        config.simplify_coordination = True
        config.simplify_subordination = True
        config.anaphora_level = pyregent.AnaphoraLevel.LocalCoherence

        print("[OK] Configuration test passed")
        print(f"  Anaphora level: {config.anaphora_level}")
        return True
    except Exception as e:
        print(f"[FAIL] Configuration test failed: {e}")
        return False

def test_rules():
    """Test rule inspection"""
    import pyregent

    try:
        all_rules = pyregent.get_all_rules()
        coord_rules = pyregent.get_coordination_rules()
        subord_rules = pyregent.get_subordination_rules()

        print("[OK] Rule inspection test passed")
        print(f"  Total rules: {len(all_rules)}")
        print(f"  Coordination: {len(coord_rules)}")
        print(f"  Subordination: {len(subord_rules)}")
        return True
    except Exception as e:
        print(f"[FAIL] Rule inspection failed: {e}")
        return False

def main():
    print("=" * 60)
    print("pyregent Installation Test")
    print("=" * 60)
    print()

    # Test import first
    if not test_import():
        return 1

    print()

    # Run other tests
    tests = [
        ("Basic Simplification", test_basic_simplification),
        ("Configuration", test_configuration),
        ("Rule Inspection", test_rules),
    ]

    passed = 0
    failed = 0

    for name, test_func in tests:
        print(f"\nTest: {name}")
        print("-" * 40)
        if test_func():
            passed += 1
        else:
            failed += 1

    print()
    print("=" * 60)
    print(f"Results: {passed} passed, {failed} failed")
    print("=" * 60)

    return 0 if failed == 0 else 1

if __name__ == "__main__":
    exit(main())
