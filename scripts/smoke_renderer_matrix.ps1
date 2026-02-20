param(
	[ValidateSet("Debug", "Release", "Dist")]
	[string]$Configuration = "Debug",
	[string]$BuildDir = "build",
	[int]$SandboxStartupSeconds = 8,
	[int]$EditorStartupSeconds = 10,
	[int]$CloseWaitSeconds = 6,
	[switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildPath = Join-Path $repoRoot $BuildDir
$configLower = $Configuration.ToLowerInvariant()
$logPath = Join-Path $repoRoot "Syndra-Debug.log"

function Invoke-SmokeCase
{
	param(
		[string]$Name,
		[string]$ExePath,
		[string[]]$Arguments,
		[string]$ExpectedRenderer,
		[int]$StartupSeconds,
		[string]$RendererEnv
	)

	if (-not (Test-Path $ExePath))
	{
		return [PSCustomObject]@{
			Case = $Name
			ExitCode = -999
			Expected = $ExpectedRenderer
			Selection = "<missing executable>"
			Valid = $false
			Errors = "Executable not found: $ExePath"
		}
	}

	if (Test-Path $logPath)
	{
		Remove-Item $logPath -Force
	}

	$previousEnv = $env:SYNDRA_RENDERER
	if ([string]::IsNullOrEmpty($RendererEnv))
	{
		Remove-Item Env:SYNDRA_RENDERER -ErrorAction SilentlyContinue
	}
	else
	{
		$env:SYNDRA_RENDERER = $RendererEnv
	}

	try
	{
		$startArgs = @{
			FilePath = $ExePath
			WorkingDirectory = $repoRoot
			PassThru = $true
		}
		if ($Arguments -and $Arguments.Count -gt 0)
		{
			$startArgs.ArgumentList = $Arguments
		}

		$process = Start-Process @startArgs
		Start-Sleep -Seconds $StartupSeconds

		if (-not $process.HasExited)
		{
			$null = $process.CloseMainWindow()
			Start-Sleep -Seconds $CloseWaitSeconds
		}

		$forcedKill = $false
		if (-not $process.HasExited)
		{
			Stop-Process -Id $process.Id -Force
			$forcedKill = $true
		}

		$exitCode = if ($process.HasExited) { $process.ExitCode } else { -1 }
		$lines = @()
		if (Test-Path $logPath)
		{
			$lines = Get-Content $logPath
		}

		$selectionLine = ($lines | Select-String -SimpleMatch "Renderer backend requested=" | Select-Object -Last 1).Line
		$errorLines = @($lines | Select-String -Pattern "\[error\]|Vulkan validation|VUID|assert|exception" -CaseSensitive:$false)

		$valid = $true
		$errors = @()

		if (-not $selectionLine)
		{
			$valid = $false
			$errors += "Missing renderer selection line in log."
		}
		elseif ($selectionLine -notmatch "active='$ExpectedRenderer'")
		{
			$valid = $false
			$errors += "Expected renderer '$ExpectedRenderer' but got selection line: $selectionLine"
		}

		if ($exitCode -ne 0 -and -not $forcedKill)
		{
			$valid = $false
			$errors += "Process exited with non-zero code $exitCode."
		}

		if ($errorLines.Count -gt 0)
		{
			$valid = $false
			$errors += "Error markers found in runtime log."
		}

		return [PSCustomObject]@{
			Case = $Name
			ExitCode = $exitCode
			Expected = $ExpectedRenderer
			Selection = if ($selectionLine) { $selectionLine } else { "<none>" }
			Valid = $valid
			Errors = if ($errors.Count -gt 0) { ($errors -join " | ") } else { "" }
		}
	}
	finally
	{
		if ($null -ne $previousEnv)
		{
			$env:SYNDRA_RENDERER = $previousEnv
		}
		else
		{
			Remove-Item Env:SYNDRA_RENDERER -ErrorAction SilentlyContinue
		}
	}
}

if (-not $SkipBuild)
{
	Write-Host "Building Syndra-Editor and Sandbox ($Configuration)..."
	cmake --build $buildPath --config $Configuration --target Syndra-Editor Sandbox
	if ($LASTEXITCODE -ne 0)
	{
		throw "Build failed with exit code $LASTEXITCODE."
	}
}

$sandboxExe = Join-Path $repoRoot "bin/$configLower-windows-x86_64/Sandbox/Sandbox.exe"
$editorExe = Join-Path $repoRoot "bin/$configLower-windows-x86_64/Syndra-Editor/Syndra-Editor.exe"

$results = @()
$results += Invoke-SmokeCase -Name "sandbox-default" -ExePath $sandboxExe -Arguments @() -ExpectedRenderer "vulkan" -StartupSeconds $SandboxStartupSeconds -RendererEnv ""
$results += Invoke-SmokeCase -Name "sandbox-vulkan" -ExePath $sandboxExe -Arguments @("--renderer=vulkan") -ExpectedRenderer "vulkan" -StartupSeconds $SandboxStartupSeconds -RendererEnv ""
$results += Invoke-SmokeCase -Name "sandbox-opengl" -ExePath $sandboxExe -Arguments @("--renderer=opengl") -ExpectedRenderer "opengl" -StartupSeconds $SandboxStartupSeconds -RendererEnv ""
$results += Invoke-SmokeCase -Name "editor-default" -ExePath $editorExe -Arguments @() -ExpectedRenderer "vulkan" -StartupSeconds $EditorStartupSeconds -RendererEnv ""
$results += Invoke-SmokeCase -Name "editor-vulkan" -ExePath $editorExe -Arguments @("--renderer=vulkan") -ExpectedRenderer "vulkan" -StartupSeconds $EditorStartupSeconds -RendererEnv ""
$results += Invoke-SmokeCase -Name "editor-opengl" -ExePath $editorExe -Arguments @("--renderer=opengl") -ExpectedRenderer "opengl" -StartupSeconds $EditorStartupSeconds -RendererEnv ""

Write-Host ""
Write-Host "Renderer smoke matrix results:"
$results | Format-Table -AutoSize

$failed = @($results | Where-Object { -not $_.Valid })
if ($failed.Count -gt 0)
{
	Write-Host ""
	Write-Host "Smoke matrix failed:"
	$failed | Format-Table -Property Case, ExitCode, Errors -AutoSize
	exit 1
}

Write-Host ""
Write-Host "All renderer smoke cases passed."
exit 0
