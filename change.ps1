Get-ChildItem -Recurse -Filter *.nosdef | Rename-Item -NewName { $_.Name -replace '\.nosdef$', '.nosnode' }

Get-ChildItem -Recurse -Filter *.noscfg | Rename-Item -NewName { $_.Name -replace '\.noscfg$', '.nosplugin' }
Get-ChildItem -Recurse -Filter *.nossys | Rename-Item -NewName { $_.Name -replace '\.nossys$', '.nosplugin' }

Get-ChildItem -Recurse -Include *.fbs, *.json | Where-Object { $_.Directory.Name -eq "Config" } | ForEach-Object {
    $typesDir = Join-Path $_.Directory.Parent.FullName "Types"
    if (-not (Test-Path $typesDir)) {
        New-Item -ItemType Directory -Path $typesDir | Out-Null
    }
    Move-Item $_.FullName -Destination $typesDir
}

Get-ChildItem -Recurse -Filter *.nosnode | Where-Object { $_.Directory.Name -eq "Config" } | ForEach-Object {
    $nodesDir = Join-Path $_.Directory.Parent.FullName "Nodes"
    if (-not (Test-Path $nodesDir)) {
        New-Item -ItemType Directory -Path $nodesDir | Out-Null
    }
    Move-Item $_.FullName -Destination $nodesDir
}