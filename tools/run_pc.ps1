<#
.SYNOPSIS
    Run the PC build for a fixed time and keep its output in build/run-*.log.
.EXAMPLE
    .\tools\run_pc.ps1 -Seconds 30
#>
param([int]$Seconds = 30)
$repo = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $repo "build\pc\games\dbz\Release\dbz_ttt.exe"
if (-not (Test-Path $exe)) { throw "build first: $exe not found" }
New-Item -ItemType Directory -Force (Join-Path $repo "build\fs\ms") | Out-Null
$out = Join-Path $repo "build\run-out.log"
$err = Join-Path $repo "build\run-err.log"
$p = Start-Process -FilePath $exe -RedirectStandardOutput $out -RedirectStandardError $err -PassThru -NoNewWindow
if (-not $p.WaitForExit($Seconds * 1000)) { Stop-Process -Id $p.Id -Force; "stopped after $Seconds s" }
else { "exited with code $($p.ExitCode)" }
