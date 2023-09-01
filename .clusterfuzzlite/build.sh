#!/bin/bash -eu
# ClusterFuzzLite build script for QuartzDB.
#
# Infra provides: $SRC $OUT $WORK $CC $CXX $CFLAGS $CXXFLAGS $LDFLAGS
#   $LIB_FUZZING_ENGINE $SANITIZER

cd "${SRC:?}"

BUILD_DIR="${WORK:-/tmp}/quartzdb_fuzz_build"
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

FUZZ_CXXFLAGS="${CXXFLAGS} -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION"

# libFuzzer provides main(); link it only on fuzz targets (see fuzz/CMakeLists.txt).
CMAKE_LDFLAGS="${LDFLAGS:-}"
CMAKE_LDFLAGS="${CMAKE_LDFLAGS//-fsanitize=fuzzer,address/-fsanitize=address}"
CMAKE_LDFLAGS="${CMAKE_LDFLAGS//-fsanitize=fuzzer-no-link,address/-fsanitize=address}"
CMAKE_LDFLAGS="${CMAKE_LDFLAGS//-fsanitize=fuzzer/}"
CMAKE_LDFLAGS="${CMAKE_LDFLAGS//-fsanitize=fuzzer-no-link/}"

export LIB_FUZZING_ENGINE="${LIB_FUZZING_ENGINE:-}"

cmake -S "${SRC}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER="${CC:?}" \
  -DCMAKE_CXX_COMPILER="${CXX:?}" \
  -DCMAKE_C_FLAGS="${CFLAGS}" \
  -DCMAKE_CXX_FLAGS="${FUZZ_CXXFLAGS}" \
  -DCMAKE_EXE_LINKER_FLAGS="${CMAKE_LDFLAGS}" \
  -DQUARTZDB_BUILD_FUZZ=ON \
  -DQUARTZDB_BUILD_TESTS=OFF \
  -DQUARTZDB_BUILD_EXAMPLES=OFF \
  -DQUARTZDB_BUILD_BENCHMARKS=OFF \
  -DQUARTZDB_BUILD_TOOLS=OFF \
  -DQUARTZDB_WARNINGS_AS_ERRORS=OFF \
  -DQUARTZDB_USE_VENDORED_CATCH2=ON

FUZZERS="fuzz_serialization fuzz_page fuzz_recovery"
cmake --build "${BUILD_DIR}" --target ${FUZZERS} -j"$(nproc)"

for fuzzer in ${FUZZERS}; do
  cp "${BUILD_DIR}/fuzz/${fuzzer}" "${OUT:?}/"
done
