#!/bin/sh
# ===========================================================================
# RAFAELIA ARM32 — Build Script Unificado
# Alvo: Motorola E7 Power · Helio G25 · Cortex-A53 ARM32
# Requer: binutils-arm-linux-gnueabi (Termux: pkg install binutils)
# ===========================================================================

set -e

ARCH="-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=softfp"
AS_FLAGS="$ARCH"
LD_FLAGS=""
OK="✓"
FAIL="✗"

log()  { printf "  $1\n"; }
pass() { printf "  $OK %s\n" "$1"; }
fail() { printf "  $FAIL %s\n" "$1"; exit 1; }

echo "==================================================="
echo "  RAFAELIA ARM32 Build"
echo "  $(date)"
echo "==================================================="

# Detecta toolchain
if command -v arm-linux-gnueabihf-as >/dev/null 2>&1; then
    AS=arm-linux-gnueabihf-as
    LD=arm-linux-gnueabihf-ld
elif command -v as >/dev/null 2>&1; then
    # Termux roda diretamente em ARM32
    AS=as
    LD=ld
else
    fail "Toolchain não encontrado. Em Termux: pkg install binutils"
fi

log "Toolchain: $AS"
echo ""

# ---------------------------------------------------------------------------
# B1: TORUS 7D + NEON + CRC32SW + ARENA
# ---------------------------------------------------------------------------
echo "--- B1: TORUS + NEON + CRC32SW ---"
$AS $AS_FLAGS rafaelia_b1.S -o rafaelia_b1.o 2>&1 \
    && pass "Assembled b1.o" \
    || fail "Assembler error B1"

$LD $LD_FLAGS rafaelia_b1.o -o rafaelia_b1 2>&1 \
    && pass "Linked rafaelia_b1" \
    || fail "Linker error B1"

echo "  → Running:"
./rafaelia_b1 && pass "B1 OK (exit 0)" || fail "B1 runtime error"
echo ""

# ---------------------------------------------------------------------------
# B2: 7 DIREÇÕES + PIPELINE NEON
# ---------------------------------------------------------------------------
echo "--- B2: 7 DIREÇÕES + PIPELINE ---"
$AS $AS_FLAGS rafaelia_b2.S -o rafaelia_b2.o 2>&1 \
    && pass "Assembled b2.o" \
    || fail "Assembler error B2"

$LD $LD_FLAGS rafaelia_b2.o -o rafaelia_b2 2>&1 \
    && pass "Linked rafaelia_b2" \
    || fail "Linker error B2"

echo "  → Running:"
./rafaelia_b2 && pass "B2 OK (exit 0)" || fail "B2 runtime error"
echo ""

# ---------------------------------------------------------------------------
# B3: MULTICORE clone() + CRC32SW + THROUGHPUT
# ---------------------------------------------------------------------------
echo "--- B3: MULTICORE + THROUGHPUT ---"
$AS $AS_FLAGS rafaelia_b3.S -o rafaelia_b3.o 2>&1 \
    && pass "Assembled b3.o" \
    || fail "Assembler error B3"

$LD $LD_FLAGS rafaelia_b3.o -o rafaelia_b3 2>&1 \
    && pass "Linked rafaelia_b3" \
    || fail "Linker error B3"

echo "  → Running:"
./rafaelia_b3 && pass "B3 OK (exit 0)" || fail "B3 runtime error"
echo ""

# ---------------------------------------------------------------------------
# B4: SENOIDES + CAMADAS + SOBREPOSIÇÃO
# ---------------------------------------------------------------------------
echo "--- B4: SENOIDE + CAMADAS + OVERLAP ---"
$AS $AS_FLAGS rafaelia_b4.S -o rafaelia_b4.o 2>&1 \
    && pass "Assembled b4.o" \
    || fail "Assembler error B4"

$LD $LD_FLAGS rafaelia_b4.o -o rafaelia_b4 2>&1 \
    && pass "Linked rafaelia_b4" \
    || fail "Linker error B4"

echo "  → Running:"
./rafaelia_b4 && pass "B4 OK (exit 0)" || fail "B4 runtime error"
echo ""

# ---------------------------------------------------------------------------
# Sumário
# ---------------------------------------------------------------------------
echo "==================================================="
echo "  $OK TODOS OS BLOCOS COMPILADOS E EXECUTADOS"
echo ""
echo "  Binários gerados:"
ls -lh rafaelia_b1 rafaelia_b2 rafaelia_b3 rafaelia_b4 2>/dev/null
echo "==================================================="

# ---------------------------------------------------------------------------
# B5: BITSTACK 1008 + COMMIT GATE + HYPERFORMS + PARIDADE + ROLLBACK
# ---------------------------------------------------------------------------
echo "--- B5: BITSTACK-1008 + COMMIT-GATE + HYPERFORMS ---"
$AS $AS_FLAGS rafaelia_b5.S -o rafaelia_b5.o 2>&1 \
    && pass "Assembled b5.o" \
    || fail "Assembler error B5"

$LD $LD_FLAGS rafaelia_b5.o -o rafaelia_b5 2>&1 \
    && pass "Linked rafaelia_b5" \
    || fail "Linker error B5"

echo "  → Running:"
./rafaelia_b5 && pass "B5 OK (exit 0)" || fail "B5 runtime error"
echo ""

echo "==================================================="
echo "  $OK TODOS OS 5 BLOCOS COMPILADOS E EXECUTADOS"
echo ""
echo "  Binários gerados:"
ls -lh rafaelia_b1 rafaelia_b2 rafaelia_b3 rafaelia_b4 rafaelia_b5 2>/dev/null
echo "==================================================="
