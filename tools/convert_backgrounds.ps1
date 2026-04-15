param(
    [string]$SourceDir = "",
    [string]$OutDir = "src/display/ui/bg"
)

$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")

if ([string]::IsNullOrWhiteSpace($SourceDir)) {
    $OneDrivePictures = Join-Path $env:USERPROFILE "OneDrive\Pictures\Dongle Pictures"
    if (Test-Path -LiteralPath $OneDrivePictures) {
        $SourceDir = $OneDrivePictures
    } else {
        $SourceDir = Join-Path $RepoRoot "src/display/ui/bg/source"
    }
}

$SourcePath = Resolve-Path $SourceDir
$OutPath = Join-Path $RepoRoot $OutDir

# Edit these if the main subject needs to move on the round display.
# center-x: smaller = left, larger = right
# center-y: smaller = up, larger = down
# zoom: larger = closer crop
$CropPresets = @(
    @{ Bg = 4; CenterX = "0.54"; CenterY = "0.50"; Zoom = "1.00" },
    @{ Bg = 5; CenterX = "0.50"; CenterY = "0.42"; Zoom = "1.00" },
    @{ Bg = 6; CenterX = "0.50"; CenterY = "0.42"; Zoom = "1.00" }
)

$Images = Get-ChildItem -LiteralPath $SourcePath |
    Where-Object { $_.Extension -match '^\.(jpg|jpeg|png)$' } |
    Sort-Object Name |
    Select-Object -First $CropPresets.Count

if ($Images.Count -lt $CropPresets.Count) {
    throw "Need at least $($CropPresets.Count) JPG/PNG images in $SourcePath"
}

python -c "import PIL" 2>$null
if ($LASTEXITCODE -ne 0) {
    python -m pip install --user pillow
}

for ($i = 0; $i -lt $CropPresets.Count; $i++) {
    $Preset = $CropPresets[$i]
    $Image = $Images[$i]

    Write-Host "Converting $($Image.Name) -> bg$($Preset.Bg)"
    python (Join-Path $RepoRoot "tools/convert_xiaord_bg.py") `
        $Image.FullName `
        $Preset.Bg `
        --center-x $Preset.CenterX `
        --center-y $Preset.CenterY `
        --zoom $Preset.Zoom `
        --out-dir $OutPath
}

Write-Host "Done. Check the generated bg4.png, bg5.png, and bg6.png preview images."
