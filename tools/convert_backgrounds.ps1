param(
    [string]$SourceDir = "",
    [string]$OutDir = "src/display/ui/bg",
    [switch]$ChooseFolder
)

$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")

function Get-ImageFiles {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        return @()
    }

    return @(Get-ChildItem -LiteralPath $Path |
        Where-Object { $_.Extension -match '^\.(jpg|jpeg|png)$' } |
        Sort-Object Name)
}

function Select-ImageFolder {
    Add-Type -AssemblyName System.Windows.Forms

    $Dialog = New-Object System.Windows.Forms.FolderBrowserDialog
    $Dialog.Description = "Choose the folder that contains your background pictures"
    $Dialog.ShowNewFolderButton = $true

    if ($Dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
        return $Dialog.SelectedPath
    }

    throw "No picture folder selected."
}

if ($ChooseFolder) {
    $SourceDir = Select-ImageFolder
} elseif ([string]::IsNullOrWhiteSpace($SourceDir)) {
    $OneDrivePictures = Join-Path $env:USERPROFILE "OneDrive\Pictures\Dongle Pictures"
    $RepoSourcePictures = Join-Path $RepoRoot "src/display/ui/bg/source"

    if ((Get-ImageFiles $OneDrivePictures).Count -gt 0) {
        $SourceDir = $OneDrivePictures
    } elseif ((Get-ImageFiles $RepoSourcePictures).Count -gt 0) {
        $SourceDir = $RepoSourcePictures
    } else {
        Write-Host "No pictures were found in the default folders."
        Write-Host "A folder picker will open. Choose the folder that contains your JPG or PNG pictures."
        $SourceDir = Select-ImageFolder
    }
}

$SourcePath = Resolve-Path $SourceDir
$OutPath = Join-Path $RepoRoot $OutDir

$Images = @(Get-ImageFiles $SourcePath | Select-Object -First 1)

if ($Images.Count -eq 0) {
    throw "Need at least one JPG/PNG image in $SourcePath"
}

python -c "import PIL" 2>$null
if ($LASTEXITCODE -ne 0) {
    python -m pip install --user pillow
}

$Image = $Images[0]

# Edit these if the main subject needs to move on the round display.
# center-x: smaller = left, larger = right
# center-y: smaller = up, larger = down
# zoom: larger = closer crop
$CenterX = "0.54"
$CenterY = "0.50"
$Zoom = "1.00"

Write-Host "Converting $($Image.Name) -> bg4"
python (Join-Path $RepoRoot "tools/convert_xiaord_bg.py") `
    $Image.FullName `
    4 `
    --center-x $CenterX `
    --center-y $CenterY `
    --zoom $Zoom `
    --out-dir $OutPath

Write-Host "Done. Converted bg4. Check src/display/ui/bg/bg4.png."
