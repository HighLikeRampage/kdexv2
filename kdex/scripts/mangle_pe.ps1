param([Parameter(Mandatory=$true)][string]$Path)

if (-not (Test-Path $Path)) {
    Write-Host "mangle_pe: target not found: $Path"
    exit 0
}

$bytes = [System.IO.File]::ReadAllBytes($Path)
if ($bytes.Length -lt 0x400) { Write-Host "mangle_pe: file too small"; exit 0 }
if ($bytes[0] -ne 0x4D -or $bytes[1] -ne 0x5A) { Write-Host "mangle_pe: not a PE"; exit 0 }

$rand = [System.Random]::new()

# --- 1. Randomize the DOS stub area (keep MZ + e_lfanew intact) ---
$e_lfanew = [BitConverter]::ToInt32($bytes, 0x3C)
if ($e_lfanew -lt 0x40 -or $e_lfanew -gt ($bytes.Length - 0x100)) {
    Write-Host "mangle_pe: bad e_lfanew"; exit 0
}
for ($i = 0x40; $i -lt $e_lfanew; $i++) { $bytes[$i] = [byte]$rand.Next(0, 256) }
# preserve MZ magic and e_lfanew field
$bytes[0] = 0x4D; $bytes[1] = 0x5A
[Array]::Copy([BitConverter]::GetBytes([int32]$e_lfanew), 0, $bytes, 0x3C, 4)

# --- 2. Wipe the Rich header if present (fully overwritten by step 1, but be explicit) ---
# Rich header signature = "Rich" (0x68636952). Nothing else to do — already scrambled.

# --- 3. Verify PE signature and zero the TimeDateStamp ---
$peOff = $e_lfanew
if ($bytes[$peOff]     -ne 0x50 -or `
    $bytes[$peOff + 1] -ne 0x45 -or `
    $bytes[$peOff + 2] -ne 0x00 -or `
    $bytes[$peOff + 3] -ne 0x00) {
    Write-Host "mangle_pe: PE signature not found"; exit 0
}
$fileHdrOff = $peOff + 4
[Array]::Copy([BitConverter]::GetBytes([uint32]0), 0, $bytes, $fileHdrOff + 4, 4)

# --- 4. Randomize section names ---
$numSections   = [BitConverter]::ToUInt16($bytes, $fileHdrOff + 2)
$sizeOfOptHdr  = [BitConverter]::ToUInt16($bytes, $fileHdrOff + 16)
$sectionOff    = $fileHdrOff + 20 + $sizeOfOptHdr

for ($s = 0; $s -lt $numSections; $s++) {
    $off = $sectionOff + $s * 40
    for ($k = 0; $k -lt 8; $k++) { $bytes[$off + $k] = [byte]$rand.Next(97, 123) }
}

# --- 5. Zero the Debug Data Directory ---
$optOff     = $fileHdrOff + 20
$magic      = [BitConverter]::ToUInt16($bytes, $optOff)
$dataDirOff = if ($magic -eq 0x20b) { $optOff + 112 } else { $optOff + 96 }
$debugDirEntry = $dataDirOff + 6 * 8
[Array]::Copy([byte[]](@(0) * 8), 0, $bytes, $debugDirEntry, 8)

# --- 6. Zero the MajorLinkerVersion / MinorLinkerVersion ---
$bytes[$optOff + 2] = 0
$bytes[$optOff + 3] = 0

# --- 7. Zero CheckSum field (offset 64 in OptionalHeader) ---
[Array]::Copy([BitConverter]::GetBytes([uint32]0), 0, $bytes, $optOff + 64, 4)

[System.IO.File]::WriteAllBytes($Path, $bytes)
Write-Host "mangle_pe: header scrambled -> $Path"
