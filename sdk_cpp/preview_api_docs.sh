#!/usr/bin/env bash
# Builds the local API-docs preview described in API_DOCUMENTATION.md:
#   1. Doxygen's own HTML (doxygen-html/) — quick sanity check, opened
#      in a browser.
#   2. If doxybook2 is on PATH, the actual Markdown Doxybook2 hands to
#      the docs site (docs-api/) — closer to the real thing.
# Never touches anything outside sdk_cpp/ — both outputs are gitignored,
# and the committed Doxyfile/.doxybook/config.json are read, not written.
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

if command -v doxybook2 >/dev/null; then
  rm -rf docs-api && mkdir docs-api
  doxybook2 --quiet --input doxygen-xml --output docs-api --config .doxybook/config.json
  echo "Doxybook2 markdown: $(pwd)/docs-api/ (browse directly, or VS Code's Markdown preview)"
else
  cat <<'EOF'

doxybook2 not found on PATH — skipping the closer Markdown check.
It has no package-manager install (no npm, winget, or brew package
exists — despite what some docs may say). Download the binary for your
OS from https://github.com/matusnovak/doxybook2/releases/latest, then
put its bin/ folder on PATH and re-run this script.
EOF
fi
