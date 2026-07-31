[CmdletBinding()]
param(
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release')]
    [string] $Configuration = 'RelWithDebInfo'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$releaseRoot = Join-Path $projectRoot "release\$Configuration"
$buildspec = Get-Content -LiteralPath (Join-Path $projectRoot 'buildspec.json') -Raw | ConvertFrom-Json
$packagePath = Join-Path $projectRoot ("release\AI-Caption-Plugin-{0}-windows-x64.zip" -f $buildspec.version)

Push-Location $projectRoot
try {
    cmake --preset windows-x64
    if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }

    cmake --build --preset windows-x64 --config $Configuration --parallel
    if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }

    ctest --test-dir build_x64 -C $Configuration --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }

    cmake --install build_x64 --prefix $releaseRoot --config $Configuration
    if ($LASTEXITCODE -ne 0) { throw 'Install staging failed.' }

    Compress-Archive -Path (Join-Path $releaseRoot 'ai-caption-plugin') -DestinationPath $packagePath -Force
    Write-Host "Package: $packagePath"
}
finally {
    Pop-Location
}
