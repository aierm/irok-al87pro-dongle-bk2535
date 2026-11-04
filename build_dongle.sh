#!/usr/bin/env bash
# Build BK2535 2.4G USB Dongle Firmware with SDCC
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Toolchain discovery
SDCC_BIN=""
if [ -n "${SDCC_DIR:-}" ] && [ -x "${SDCC_DIR}/sdcc" ]; then
    SDCC_BIN="${SDCC_DIR}"
elif command -v sdcc >/dev/null 2>&1; then
    SDCC_BIN="$(dirname "$(command -v sdcc)")"
elif [ -x "/usr/bin/sdcc" ]; then
    SDCC_BIN="/usr/bin"
elif [ -x "/usr/local/bin/sdcc" ]; then
    SDCC_BIN="/usr/local/bin"
else
    # Fallback search for local or portable sdcc
    for candidate in \
        "/home/aier/.gemini/antigravity-cli/brain/26899515-c741-45b3-8707-d588d5e3ee33/scratch/sdcc/usr/bin" \
        "$HOME/.local/bin" \
        "/opt/sdcc/bin"; do
        if [ -x "${candidate}/sdcc" ]; then
            SDCC_BIN="$candidate"
            break
        fi
    done
fi

if [ -z "$SDCC_BIN" ] || [ ! -x "${SDCC_BIN}/sdcc" ]; then
    echo "Error: SDCC toolchain not found. Please install SDCC (e.g. 'sudo apt install sdcc') or set SDCC_DIR." >&2
    exit 1
fi

SDCC="${SDCC_BIN}/sdcc"
MAKEBIN="${SDCC_BIN}/makebin"
if [ ! -x "$MAKEBIN" ]; then
    MAKEBIN="$(command -v makebin || echo "")"
fi

# Locate SDCC include and library directories
if [ -d "$(dirname "${SDCC_BIN}")/share/sdcc/include" ]; then
    SDCC_SHARE="$(dirname "${SDCC_BIN}")/share/sdcc"
elif [ -d "/usr/share/sdcc/include" ]; then
    SDCC_SHARE="/usr/share/sdcc"
elif [ -d "/usr/local/share/sdcc/include" ]; then
    SDCC_SHARE="/usr/local/share/sdcc"
else
    SDCC_SHARE="$(dirname "${SDCC_BIN}")/share/sdcc"
fi

SDCC_INC="${SDCC_SHARE}/include"
SDCC_LIB="${SDCC_SHARE}/lib/large"

CFLAGS="-mmcs51 --model-large --std-sdcc99 -I. -I${SDCC_INC}"
LFLAGS="-mmcs51 --model-large --nostdlib -L${SDCC_LIB} -lmcs51 -llibsdcc -llibint -lliblong --code-loc 0x0000 --code-size 0x8000 --xram-loc 0x0000 --xram-size 0x0380 --iram-size 256"

mkdir -p build output

echo "[1/4] Compiling BK2535 sources..."
"$SDCC" $CFLAGS -c usb_dongle.c -o build/usb_dongle.rel
"$SDCC" $CFLAGS -c rf_dongle.c  -o build/rf_dongle.rel
"$SDCC" $CFLAGS -c main.c       -o build/main.rel

echo "[2/4] Linking firmware..."
"$SDCC" $LFLAGS build/main.rel build/usb_dongle.rel build/rf_dongle.rel -o build/dongle_bk2535.ihx

echo "[3/4] Converting HEX to 32KB binary image..."
"$MAKEBIN" -p -s 32768 build/dongle_bk2535.ihx output/dongle_bk2535.bin

echo "[4/4] Generating Beken CRC binary (dongle_bk2535_crc.bin)..."
if [ -f "./bin_to_crcbin.exe" ]; then
    (
        cd output
        cp -f dongle_bk2535.bin fw.bin
        if command -v wine >/dev/null 2>&1; then
            WINEDEBUG=-all wine "../bin_to_crcbin.exe" fw.bin 32768 >/dev/null 2>&1 || true
            if [ -f fw_crc.bin ]; then
                cp -f fw_crc.bin dongle_bk2535_crc.bin
                rm -f fw.bin fw_crc.bin
            fi
        fi
    )
fi

ls -lh output/dongle_bk2535.bin output/dongle_bk2535_crc.bin
echo "BK2535 Dongle build PASS!"
