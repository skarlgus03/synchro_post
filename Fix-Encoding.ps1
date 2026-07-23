Get-ChildItem -Path ".\Source" -Recurse -Include *.h,*.cpp | ForEach-Object {
    $bytes = [System.IO.File]::ReadAllBytes($_.FullName)

    # 이미 UTF-8 BOM 있으면 건너뜀
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        return
    }

    $utf8Strict = New-Object System.Text.UTF8Encoding($false, $true)
    try {
        $text = $utf8Strict.GetString($bytes)   # 이미 유효한 UTF-8이면 그대로 사용
    }
    catch {
        $cp949 = [System.Text.Encoding]::GetEncoding(949)
        $text = $cp949.GetString($bytes)        # 아니면 CP949로 해석
    }

    $utf8Bom = New-Object System.Text.UTF8Encoding($true)
    [System.IO.File]::WriteAllText($_.FullName, $text, $utf8Bom)
    Write-Host "Fixed: $($_.FullName)"
}