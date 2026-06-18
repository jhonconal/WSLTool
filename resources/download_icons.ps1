# PowerShell script to download and generate SVG icons for WSLTool
$ErrorActionPreference = "Stop"

$destDir = Join-Path $PSScriptRoot "icons"
if (-not (Test-Path $destDir)) {
    New-Item -ItemType Directory -Path $destDir | Out-Null
}

$headers = @{"User-Agent" = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) WSLTool/1.0"}

# 1. Download official Linux distribution brand logos
$distros = @{
    "ubuntu.svg"    = "https://raw.githubusercontent.com/lukas-w/font-logos/master/vectors/ubuntu.svg"
    "debian.svg"    = "https://raw.githubusercontent.com/lukas-w/font-logos/master/vectors/debian.svg"
    "fedora.svg"    = "https://raw.githubusercontent.com/lukas-w/font-logos/master/vectors/fedora.svg"
    "archlinux.svg" = "https://raw.githubusercontent.com/lukas-w/font-logos/master/vectors/archlinux.svg"
    "opensuse.svg"  = "https://raw.githubusercontent.com/lukas-w/font-logos/master/vectors/opensuse.svg"
    "alpine.svg"    = "https://raw.githubusercontent.com/lukas-w/font-logos/master/vectors/alpine.svg"
    "kali.svg"      = "https://raw.githubusercontent.com/lukas-w/font-logos/master/vectors/kali-linux.svg"
    "oracle.svg"    = "https://raw.githubusercontent.com/gilbarbara/logos/master/logos/oracle.svg"
    "amazon.svg"    = "https://raw.githubusercontent.com/gilbarbara/logos/master/logos/aws.svg"
    "tux.svg"       = "https://raw.githubusercontent.com/lukas-w/font-logos/master/vectors/tux.svg"
}

Write-Host "=== Downloading Linux Distro Logos ==="
foreach ($key in $distros.Keys) {
    $url = $distros[$key]
    $outPath = Join-Path $destDir $key
    try {
        Invoke-WebRequest -Uri $url -OutFile $outPath -Headers $headers -UseBasicParsing
        Write-Host "Downloaded $key successfully ($((Get-Item $outPath).Length) bytes)"
    } catch {
        Write-Warning "Failed to download $key from $url : $_"
    }
}

# 2. Download Microsoft Fluent UI system icons
$fluent = @{
    "card_windows.svg"     = "Window/SVG/ic_fluent_window_24_regular.svg"
    "card_wsl_ok.svg"      = "Checkmark%20Circle/SVG/ic_fluent_checkmark_circle_24_regular.svg"
    "card_wsl_fail.svg"    = "Dismiss%20Circle/SVG/ic_fluent_dismiss_circle_24_regular.svg"
    "card_kernel.svg"      = "Settings/SVG/ic_fluent_settings_24_regular.svg"
    "card_version.svg"     = "Info/SVG/ic_fluent_info_24_regular.svg"
    "card_disk.svg"        = "Hard%20Drive/SVG/ic_fluent_hard_drive_24_regular.svg"
    "card_warning.svg"     = "Warning/SVG/ic_fluent_warning_24_regular.svg"
    "card_play.svg"        = "Play/SVG/ic_fluent_play_24_regular.svg"
    "theme_dark.svg"       = "Dark%20Theme/SVG/ic_fluent_dark_theme_24_regular.svg"
    "theme_light.svg"      = "Weather%20Sunny/SVG/ic_fluent_weather_sunny_24_regular.svg"
}

Write-Host "`n=== Downloading Microsoft Fluent UI System Icons ==="
foreach ($key in $fluent.Keys) {
    $path = $fluent[$key]
    $url = "https://raw.githubusercontent.com/microsoft/fluentui-system-icons/main/assets/$path"
    $outPath = Join-Path $destDir $key
    try {
        Invoke-WebRequest -Uri $url -OutFile $outPath -UseBasicParsing
        Write-Host "Downloaded $key successfully ($((Get-Item $outPath).Length) bytes)"
    } catch {
        Write-Warning "Failed to download $key from $url : $_"
    }
}

# 3. Generate Custom Window and Navigation Control SVG Icons
Write-Host "`n=== Generating Custom SVG Icons ==="

$customIcons = @{
    "window_minimize.svg" = @'
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 10 10" width="10" height="10">
    <line x1="1" y1="5" x2="9" y2="5" stroke="#64748b" stroke-width="1"/>
</svg>
'@

    "window_minimize_hover.svg" = @'
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 10 10" width="10" height="10">
    <line x1="1" y1="5" x2="9" y2="5" stroke="#e2e8f0" stroke-width="1"/>
</svg>
'@

    "window_maximize.svg" = @'
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 10 10" width="10" height="10">
    <rect x="1.5" y="1.5" width="7" height="7" fill="none" stroke="#64748b" stroke-width="1"/>
</svg>
'@

    "window_maximize_hover.svg" = @'
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 10 10" width="10" height="10">
    <rect x="1.5" y="1.5" width="7" height="7" fill="none" stroke="#e2e8f0" stroke-width="1"/>
</svg>
'@

    "window_restore.svg" = @'
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 10 10" width="10" height="10">
    <path d="M 3.5 3 V 1.5 H 8.5 V 6.5 H 7" fill="none" stroke="#64748b" stroke-width="1"/>
    <rect x="1.5" y="3.5" width="5" height="5" fill="none" stroke="#64748b" stroke-width="1"/>
</svg>
'@

    "window_restore_hover.svg" = @'
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 10 10" width="10" height="10">
    <path d="M 3.5 3 V 1.5 H 8.5 V 6.5 H 7" fill="none" stroke="#e2e8f0" stroke-width="1"/>
    <rect x="1.5" y="3.5" width="5" height="5" fill="none" stroke="#e2e8f0" stroke-width="1"/>
</svg>
'@

    "window_close.svg" = @'
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 10 10" width="10" height="10">
    <path d="M 1.5 1.5 L 8.5 8.5 M 8.5 1.5 L 1.5 8.5" fill="none" stroke="#64748b" stroke-width="1"/>
</svg>
'@

    "window_close_hover.svg" = @'
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 10 10" width="10" height="10">
    <path d="M 1.5 1.5 L 8.5 8.5 M 8.5 1.5 L 1.5 8.5" fill="none" stroke="#ff4d6d" stroke-width="1"/>
</svg>
'@

    "refresh.svg" = @'
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16">
    <path d="M 13.65 4.35 A 6 6 0 1 0 13.65 11.65" fill="none" stroke="#00d4aa" stroke-width="1.8" stroke-linecap="round"/>
    <path d="M 14 1 V 5 H 10" fill="none" stroke="#00d4aa" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/>
</svg>
'@

    "home.svg" = @'
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16">
    <path d="M 1.5 6.5 L 8 1.5 L 14.5 6.5 V 14.5 H 9.5 V 10 H 6.5 V 14.5 H 1.5 Z" fill="none" stroke="#64748b" stroke-width="1.2" stroke-linejoin="round"/>
</svg>
'@

    "distro.svg" = @'
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16">
    <rect x="1.5" y="2.5" width="13" height="11" rx="1.5" fill="none" stroke="#64748b" stroke-width="1.2"/>
    <path d="M 4 5.5 L 6.5 8 L 4 10.5 M 8 10 H 11" fill="none" stroke="#64748b" stroke-width="1.2" stroke-linecap="round"/>
</svg>
'@
}

foreach ($key in $customIcons.Keys) {
    $outPath = Join-Path $destDir $key
    Set-Content -Path $outPath -Value $customIcons[$key] -Encoding utf8
    Write-Host "Generated $key successfully"
}

Write-Host "`n=== Icon preparation completed! ==="
