# Modified version of vcpkg's applocal.ps1 script.
# Source: https://github.com/microsoft/vcpkg/blob/619b29470031781e0c787eff32dbc7d8568bdbe6/scripts/buildsystems/msbuild/applocal.ps1#L60
#
# Modifications involve renamed variables, externally passed destination dir, and removed code for irrelevant cases.

[cmdletbinding()]

param([string]$targetBinary, [string]$originDir, [string]$destinationDir, [switch]$listOnly)

Write-Verbose "Inputs: targetBinary=$targetBinary, destinationDir=$destinationDir, originDir=$originDir, listOnly=$listOnly"

$g_searched = @{}
$g_sourcelist = @()
$g_install_root = Split-Path $originDir -parent
$cwd = Get-Location
Write-Verbose "cwd: $cwd"

Write-Verbose "Resolving base path $targetBinary..."
try
{
    $baseBinaryPath = Resolve-Path $targetBinary -erroraction stop
    $baseTargetBinaryDir = Resolve-Path $destinationDir -erroraction stop
}
catch [System.Management.Automation.ItemNotFoundException]
{
    return
}

function computeHash([System.Security.Cryptography.HashAlgorithm]$alg, [string]$str) {
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($str)
    $hash = $alg.ComputeHash($bytes)
    return [Convert]::ToBase64String($hash)
}

function getMutex([string]$targetDir) {
    try {
        $sha512Hash = [System.Security.Cryptography.SHA512]::Create()
        if ($sha512Hash) {
            $hash = (computeHash $sha512Hash $targetDir) -replace ('/' ,'-')
            $mtxName = "VcmiDeployBinary-" + $hash
            return New-Object System.Threading.Mutex($false, $mtxName)
        }

        return New-Object System.Threading.Mutex($false, "VcmiDeployBinary")
    }
    catch {
        Write-Error -Message $_ -ErrorAction Stop
    }
}

function deployBinary([string]$targetBinaryDir, [string]$SourceDir, [string]$targetBinaryName) {
    try {
        $mtx = getMutex($targetBinaryDir)
        if ($mtx) {
            $mtx.WaitOne() | Out-Null
        }

        $sourceBinaryFilePath = Join-Path $SourceDir $targetBinaryName
        $targetBinaryFilePath = Join-Path $targetBinaryDir $targetBinaryName
        if ($listOnly) {
            $g_sourcelist += $targetBinaryName
        } else {
            if (Test-Path $targetBinaryFilePath) {
                $sourceModTime = (Get-Item $sourceBinaryFilePath).LastWriteTime
                $destModTime = (Get-Item $targetBinaryFilePath).LastWriteTime
                if ($destModTime -lt $sourceModTime) {
                    Write-Verbose "  ${targetBinaryName}: Updating from $sourceBinaryFilePath to $targetBinaryDir"
                    Copy-Item $sourceBinaryFilePath $targetBinaryDir
                } else {
                    Write-Verbose "  ${targetBinaryName}: already present"
                }
            }
            else {
                Write-Verbose "  ${targetBinaryName}: Copying $sourceBinaryFilePath to $targetBinaryDir"
                Copy-Item $sourceBinaryFilePath $targetBinaryDir
            }
        }
    } finally {
        if ($mtx) {
            $mtx.ReleaseMutex() | Out-Null
            $mtx.Dispose() | Out-Null
        }
    }
}

function resolve([string]$targetBinary) {
    Write-Verbose "Resolving $targetBinary..."
    try
    {
        $targetBinaryPath = Resolve-Path $targetBinary -erroraction stop
    }
    catch [System.Management.Automation.ItemNotFoundException]
    {
        return
    }

    # dumpbin /DEPENDENTS .\out\build\windows-msvc-release\bin\RelWithDebInfo\AI\MMAI.dll |
    if (Get-Command "dumpbin" -ErrorAction SilentlyContinue) {
        $a = $(dumpbin /DEPENDENTS $targetBinaryPath| ? { $_ -match "^    [^ ].*\.dll" } | % { $_ -replace "^    ","" })
    } elseif (Get-Command "llvm-objdump" -ErrorAction SilentlyContinue) {
        $a = $(llvm-objdump -p $targetBinary| ? { $_ -match "^ {4}DLL Name: .*\.dll" } | % { $_ -replace "^ {4}DLL Name: ","" })
    } elseif (Get-Command "objdump" -ErrorAction SilentlyContinue) {
        $a = $(objdump -p $targetBinary| ? { $_ -match "^\tDLL Name: .*\.dll" } | % { $_ -replace "^\tDLL Name: ","" })
    } else {
        Write-Error "Neither dumpbin, llvm-objdump nor objdump could be found. Can not take care of dll dependencies."
    }

    $a | % {
        if ([string]::IsNullOrEmpty($_)) {
            return
        }
        if ($g_searched.ContainsKey($_)) {
            Write-Verbose "  ${_}: previously searched - Skip"
            return
        }
        $g_searched.Set_Item($_, $true)
        $installedItemFilePath = Join-Path $originDir $_
        $targetItemFilePath = Join-Path $destinationDir $_
        if (Test-Path $installedItemFilePath) {
            deployBinary $baseTargetBinaryDir $originDir "$_"
            resolve (Join-Path $baseTargetBinaryDir "$_")
        } elseif (Test-Path $targetItemFilePath) {
            Write-Verbose "  ${_}: $_ not found in $g_install_root; locally deployed at $targetItemFilePath"
            resolve "$targetItemFilePath"
        } else {
            Write-Verbose "  ${_}: $installedItemFilePath not found"
        }
    }
    Write-Verbose "Done Resolving $targetBinary."
}

resolve($targetBinary)
Write-Verbose $($g_searched | out-string)

if ($listOnly) {
    Write-Output $($g_sourcelist -join ",")
}
