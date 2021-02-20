#!/usr/bin/env bash

set -euo pipefail

source "$(dirname "$BASH_SOURCE[0]")/wasm_env.sh"

DUCKDB_CC="${PROJECT_ROOT}/src/amalgamation/duckdb.cpp"
DUCKDB_H="${PROJECT_ROOT}/src/amalgamation/duckdb.hpp"

test -f "${DUCKDB_CC}" \
    && { echo "[ OK  ] Amalgamation Source: ${DUCKDB_CC}"; } \
    || { echo "[ ERR ] Amalgamation Source: ${DUCKDB_CC}"; exit 1; }

test -f "${DUCKDB_H}" \
    && { echo "[ OK  ] Amalgamation Header: ${DUCKDB_H}"; } \
    || { echo "[ ERR ] Amalgamation Header: ${DUCKDB_H}"; exit 1; }

set -x
BUILD_DIR="${PROJECT_ROOT}/.wasm/build"
if [ -d ${BUILD_DIR} ]; then
    rm -r "${BUILD_DIR}"
fi
mkdir -p ${BUILD_DIR}

source "${EMSDK_ENV}"

${EMCPP} \
    -std=gnu++17 \
    -O2 -g \
    -fexceptions \
    -sDISABLE_EXCEPTION_CATCHING=0 \
    -sUSE_PTHREADS=0 \
    -DNDEBUG \
    -DDUCKDB_NO_THREADS=1 \
    -I ${PROJECT_ROOT}/src/include \
    -I ${PROJECT_ROOT}/third_party/concurrentqueue/ \
    -o ${DUCKDB_WASM} \
    -c ${DUCKDB_CC}