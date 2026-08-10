# Builds the local API-docs preview described in API_DOCUMENTATION.md:
#   1. Doxygen's own HTML (doxygen-html\) - quick sanity check, opened
#      in a browser.
#   2. If doxybook2 is on PATH, the actual Markdown Doxybook2 hands to
#      the docs site (docs-api\) - closer to the real thing.
# Never touches anything outside sdk_cpp\ - both outputs are gitignored,
# and the committed Doxyfile/.doxybook/config.json are read, not written.
#
# Native PowerShell equivalent of preview_api_docs.sh, for Windows users
# who don't have/want to use Git Bash.

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

$doxygen = Get-Command doxygen -ErrorAction SilentlyContinue
if (-not $doxygen) {
    $fallback = "C:\Program Files\doxygen\bin\doxygen.exe"
    if (Test-Path $fallback) {
        $doxygen = $fallback
    } else {
        Write-Error "doxygen not found on PATH or at '$fallback'. Install it: winget install --id DimitriVanHeesch.Doxygen -e"
        exit 1
    }
} else {
    $doxygen = $doxygen.Source
}

& $doxygen
Write-Output "Doxygen HTML: $PSScriptRoot\doxygen-html\index.html"
Start-Process -FilePath (Join-Path $PSScriptRoot "doxygen-html\index.html")

$doxybook2 = Get-Command doxybook2 -ErrorAction SilentlyContinue
if ($doxybook2) {
    $docsApi = Join-Path $PSScriptRoot "docs-api"
    Remove-Item -Recurse -Force $docsApi -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Path $docsApi | Out-Null
    & $doxybook2.Source --quiet --input (Join-Path $PSScriptRoot "doxygen-xml") --output $docsApi --config (Join-Path $PSScriptRoot ".doxybook\config.json")
    Write-Output "Doxybook2 markdown: $docsApi\ (browse directly, or VS Code's Markdown preview)"
} else {
    Write-Output ""
    Write-Output "doxybook2 not found on PATH - skipping the closer Markdown check."
    Write-Output "It has no package-manager install (no npm, winget, or brew package"
    Write-Output "exists - despite what some docs may say). Download the binary for your"
    Write-Output "OS from https://github.com/matusnovak/doxybook2/releases/latest, then"
    Write-Output "put its bin\ folder on PATH and re-run this script."
}
