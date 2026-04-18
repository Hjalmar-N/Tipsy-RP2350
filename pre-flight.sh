#!/usr/bin/env bash
set -euo pipefail

find_cmake() {
  local candidates=(
    "/c/Program Files/CMake/bin/cmake.exe"
    "/mnt/c/Program Files/CMake/bin/cmake.exe"
    "C:/Program Files/CMake/bin/cmake.exe"
  )

  for candidate in "${candidates[@]}"; do
    if [ -x "$candidate" ]; then
      printf '%s\n' "$candidate"
      return
    fi
  done

  if command -v cmake >/dev/null 2>&1; then
    command -v cmake
    return
  fi

  echo "[pre-flight] Error: could not find cmake." >&2
  exit 1
}

find_ninja() {
  local candidates=()

  if [ -n "${LOCALAPPDATA:-}" ]; then
    candidates+=("$LOCALAPPDATA/Microsoft/WinGet/Links/ninja.exe")
  fi

  if [ -n "${USERPROFILE:-}" ]; then
    candidates+=("$USERPROFILE/AppData/Local/Microsoft/WinGet/Links/ninja.exe")
  fi

  candidates+=(
    "/c/Users/$USER/AppData/Local/Microsoft/WinGet/Links/ninja.exe"
    "/mnt/c/Users/$USER/AppData/Local/Microsoft/WinGet/Links/ninja.exe"
  )

  for candidate in "${candidates[@]}"; do
    if [ -x "$candidate" ]; then
      printf '%s\n' "$candidate"
      return
    fi
  done

  if command -v ninja >/dev/null 2>&1; then
    command -v ninja
    return
  fi
}

CMAKE_EXE="$(find_cmake)"
NINJA_EXE="$(find_ninja || true)"

echo "[pre-flight] Tipsy-RP2350"
echo "[pre-flight] Using cmake: $CMAKE_EXE"

if [ ! -f CMakeLists.txt ]; then
  echo "[pre-flight] Error: run this from the project root."
  exit 1
fi

echo "[pre-flight] Configuring..."
if [ -n "$NINJA_EXE" ]; then
  echo "[pre-flight] Using ninja: $NINJA_EXE"
  "$CMAKE_EXE" -G Ninja -DCMAKE_MAKE_PROGRAM="$NINJA_EXE" -S . -B build -DPICO_BOARD=pico2 -Wno-dev
else
  echo "[pre-flight] Ninja not found, using CMake default generator."
  "$CMAKE_EXE" -S . -B build -DPICO_BOARD=pico2 -Wno-dev
fi

echo "[pre-flight] Building..."
"$CMAKE_EXE" --build build

echo "[pre-flight] Build complete."
echo "[pre-flight] UF2 files found:"
find build -maxdepth 3 -name "*.uf2" 2>/dev/null || true

echo "[pre-flight] If needed, copy the UF2 to the board in BOOTSEL mode."
