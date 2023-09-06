# Contributing to QuartzDB

Thank you for contributing to QuartzDB.

## Development Setup

```bash
git clone <repository-url>
cd QuartzDB
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQUARTZDB_BUILD_TESTS=ON -DQUARTZDB_WARNINGS_AS_ERRORS=ON
cmake --build build
cd build && ctest --output-on-failure
```

## Code Style

Follow [coding_guidelines.md](coding_guidelines.md):

- C++17, no exceptions in library internals
- `Status`-based error handling
- `camelCase` methods, `PascalCase` types, `snake_case_` members
- RAII resource management
- Serialize via `BinaryReader`/`BinaryWriter`

## Architecture Rules

- Respect the dependency graph (see [architecture.md](architecture.md))
- No cyclic dependencies between layers
- New subsystems get headers under `include/quartz/<layer>/` and sources under `src/<layer>/`
- Add unit tests for every public API
- Update documentation when adding features

## Pull Request Checklist

- [ ] Tests pass locally
- [ ] New tests cover added behavior
- [ ] Documentation updated (README, architecture, subsystem docs)
- [ ] No warnings with `QUARTZDB_WARNINGS_AS_ERRORS=ON`
- [ ] Public API documented in headers

## Running Benchmarks and Fuzz

```bash
cmake -B build -DQUARTZDB_BUILD_BENCHMARKS=ON -DQUARTZDB_BUILD_FUZZ=ON
cmake --build build
./build/benchmarks/bench_btree
```

Fuzz targets require Clang with libFuzzer support.

## Offline Builds

Catch2 is vendored at `third_party/Catch2`. No network access is required when `QUARTZDB_USE_VENDORED_CATCH2=ON`.
