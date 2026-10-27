# Build all gripper + lift STLs.  Run:  .\build.ps1
#
# NOTE on quoting: OpenSCAD needs  -D PART="base"  with the inner double
# quotes surviving all the way to the exe. PowerShell 5.1 eats plain and
# backtick-escaped quotes on native calls; only BACKSLASH-escaped ones get
# through:  "PART=\`"$p\`""
# Get this wrong and OpenSCAD silently renders an empty object and still
# exits 0 -- it only warns "Ignoring unknown variable 'base'".

$oscad = 'C:\Program Files\OpenSCAD\openscad.exe'
if (-not (Test-Path $oscad)) { throw "OpenSCAD not found at $oscad" }

# lift.scad is SUPERSEDED - the real robot already has a rack-and-pinion
# lift with the motor on the carriage, which is a better arrangement than
# the belt drive that file describes. Kept only for the reach arithmetic.
# Do not print idler_top / motor_base / belt_clamp / carriage.
$jobs = [ordered]@{
    'gripper.scad' = @('base', 'pinion', 'rack', 'jaw', 'pad')
}

$outdir = Join-Path $PSScriptRoot 'stl'
if (-not (Test-Path $outdir)) { New-Item -ItemType Directory -Path $outdir | Out-Null }

foreach ($src in $jobs.Keys) {
    foreach ($p in $jobs[$src]) {
        $out = Join-Path $outdir "$p.stl"
        if (Test-Path $out) { Remove-Item $out }
        & $oscad -o $out -D "PART=\`"$p\`"" (Join-Path $PSScriptRoot $src) | Out-Null
        if (Test-Path $out) {
            $kb = [math]::Round((Get-Item $out).Length / 1KB)
            "{0,-12} {1,6} KB" -f $p, $kb
        } else {
            Write-Warning "FAILED: $p from $src"
        }
    }
}
