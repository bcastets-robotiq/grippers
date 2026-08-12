# Builds the local API-docs preview described in API_DOCUMENTATION.md:
# Doxygen's own HTML (doxygen-html\) - quick sanity check, opened in a
# browser. Never touches anything outside sdk_cpp\ - the output is
# gitignored, and the committed Doxyfile is read, not written.
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
