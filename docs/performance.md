# Performance Guide

## B-tree Operations

- Sequential inserts benefit from sorted bulk loading via `maintenance::BulkLoader`.
- Range scans use `BTree::begin()`/`end()` cursor iteration.
- Tree height grows logarithmically; monitor `tree.statistics().height`.

## WAL Tuning

- Increase `DatabaseOptions::walBufferCapacity` for fewer flush syscalls.
- Call `checkpoint()` periodically to bound recovery time.
- Enable `truncateWalOnCheckpoint` to limit WAL file growth.

## Benchmarks

Microbenchmarks are in `benchmarks/`:

| Target | Measures |
|--------|----------|
| `bench_btree` | 10,000 sequential inserts |
| `bench_wal` | 5,000 WAL appends + flush |
| `bench_recovery` | Checkpoint + incremental replay |

Build with `-DQUARTZDB_BUILD_BENCHMARKS=ON`.

## Instrumentation

Use `instrumentation::Timer`, `Counter`, and `Profiler` for ad-hoc profiling:

```cpp
quartz::instrumentation::Timer timer;
timer.start();
// ... operation ...
timer.stop();
```

## Space Management

Run `maintenance::Vacuum` to refresh fragmentation statistics. Monitor `FragmentationAnalyzer` output via `ConsistencyChecker::analyze`.
