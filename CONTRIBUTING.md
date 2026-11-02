# Contributing

Use small pull requests with a concrete before/after behaviour, a regression test and evidence that existing behaviour stays intact.

1. Run `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j && ctest --test-dir build --output-on-failure`.
2. For score changes, give a table of stimuli/personality inputs and expected intent before modifying constants.
3. Add a deterministic test for bug reports; include exact tick step, entity IDs and seed.
4. Avoid external engine dependencies in `src/` and `include/rootcastle/npc.hpp`. Put integrations in isolated adapters or bindings.
5. Never add proprietary game code, game assets or hardcoded executable patching to this repository.
6. Changes to the C ABI require explicit ABI version review, compatibility tests and updated integration docs.

By contributing, you agree that your original contributions may be distributed under the MIT license.
