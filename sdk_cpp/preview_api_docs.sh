#!/usr/bin/env bash
# Builds the local API-docs preview described in API_DOCUMENTATION.md:
# Doxygen's own HTML (doxygen-html/) — quick sanity check, opened in a
# browser. Never touches anything outside sdk_cpp/ — the output is
# gitignored, and the committed Doxyfile is read, not written.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

doxygen
echo "Doxygen HTML: $(pwd)/doxygen-html/index.html"

open_file() {
  case "$(uname -s)" in
  Darwin) open "$1" ;;
  MINGW* | MSYS* | CYGWIN*)
    powershell.exe -NoProfile -Command "Start-Process -FilePath '$(cygpath -w "$1")'" >/dev/null
    ;;
  *)
    if command -v xdg-open >/dev/null; then
      xdg-open "$1"
    else
      echo "Open manually: $1"
    fi
    ;;
  esac
}
open_file "$(pwd)/doxygen-html/index.html"
