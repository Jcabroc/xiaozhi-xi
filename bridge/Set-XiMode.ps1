param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Ollama', 'GPT', 'XiaoZhi', 'Status')]
    [string]$Mode,

    [string]$Port = 'COM4',
    [switch]$NoDeviceReset
)

$ErrorActionPreference = 'Stop'
$env:PYTHONIOENCODING = 'utf-8'
$env:PYTHONUTF8 = '1'
if ([string]::IsNullOrWhiteSpace($env:OPENAI_API_KEY)) {
    $env:OPENAI_API_KEY = [Environment]::GetEnvironmentVariable('OPENAI_API_KEY', 'User')
}

$bridgeRoot = 'C:\Users\Gir\Documents\ChatGPT\XiaoZhi\xiaozhi-esp32-server\main\xiaozhi-server'
$configPath = Join-Path $bridgeRoot 'data\.config.yaml'
$pythonExe = 'C:\ESP32_Projects\xi-bridge-venv\Scripts\python.exe'
$ollamaExe = 'C:\Users\Gir\AppData\Local\Programs\Ollama\ollama.exe'
$opusBin = 'C:\ESP32_Projects\libopus-runtime\Library\bin'
$stateDir = 'C:\ESP32_Projects\xi-bridge-data'
$statePath = Join-Path $stateDir 'mode-state.json'
$stdoutPath = Join-Path $stateDir 'bridge-stdout.log'
$stderrPath = Join-Path $stateDir 'bridge-stderr.log'
$idfPython = 'C:\Espressif\python_env\idf5.5_py3.11_env\Scripts\python.exe'
$ffmpegRoot = Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages\BtbN.FFmpeg.LGPL.Shared.8.1_Microsoft.Winget.Source_8wekyb3d8bbwe'

function Get-XiState {
    if (-not (Test-Path -LiteralPath $statePath)) {
        return $null
    }
    try {
        return Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
    } catch {
        return $null
    }
}

function Stop-XiBridge {
    $state = Get-XiState
    if ($null -eq $state -or $null -eq $state.pid) {
        return
    }

    $processInfo = Get-CimInstance Win32_Process -Filter "ProcessId=$($state.pid)" -ErrorAction SilentlyContinue
    if (
        $null -ne $processInfo -and
        $processInfo.ExecutablePath -ieq $pythonExe -and
        $processInfo.CommandLine -match '(^|[\\\s"'']+)app\.py([\s"'']|$)'
    ) {
        Stop-Process -Id $state.pid -Force
        Wait-Process -Id $state.pid -Timeout 5 -ErrorAction SilentlyContinue
    }
}

function Set-ConfiguredLlm([string]$provider) {
    $content = Get-Content -LiteralPath $configPath -Raw
    $pattern = '(?m)^(\s{2}LLM:)\s*\S+\s*$'
    if ($content -notmatch $pattern) {
        throw 'No encontré selected_module.LLM en data/.config.yaml.'
    }
    $content = [regex]::Replace($content, $pattern, "`$1 $provider", 1)
    Set-Content -LiteralPath $configPath -Value $content -Encoding utf8
}

function Ensure-Ollama {
    try {
        Invoke-RestMethod -Uri 'http://127.0.0.1:11434/api/version' -TimeoutSec 2 | Out-Null
    } catch {
        Start-Process -FilePath $ollamaExe -ArgumentList 'serve' -WindowStyle Hidden
        foreach ($attempt in 1..20) {
            Start-Sleep -Milliseconds 500
            try {
                Invoke-RestMethod -Uri 'http://127.0.0.1:11434/api/version' -TimeoutSec 2 | Out-Null
                break
            } catch {
                if ($attempt -eq 20) { throw 'Ollama no respondió después de 10 segundos.' }
            }
        }
    }

    $warmupBody = @{
        model = 'qwen3.5:4b'; stream = $false; think = $false; keep_alive = '30m'
        messages = @(@{ role = 'user'; content = 'Responde solamente: lista.' })
        options = @{ num_ctx = 2048; num_predict = 8 }
    } | ConvertTo-Json -Depth 5
    Invoke-RestMethod -Uri 'http://127.0.0.1:11434/api/chat' -Method Post `
        -ContentType 'application/json' -Body $warmupBody -TimeoutSec 120 | Out-Null
}

function Start-XiServer([string]$activeMode) {
    $ffmpegExe = Get-ChildItem -LiteralPath $ffmpegRoot -Recurse -Filter ffmpeg.exe -File |
        Select-Object -First 1 -ExpandProperty FullName
    foreach ($requiredPath in @($bridgeRoot, $configPath, $pythonExe, $opusBin, $ffmpegExe)) {
        if (-not (Test-Path -LiteralPath $requiredPath)) {
            throw "Falta un componente del puente: $requiredPath"
        }
    }

    $env:PATH = "$opusBin;$(Split-Path -Parent $ffmpegExe);$env:PATH"
    $process = Start-Process -FilePath $pythonExe -ArgumentList 'app.py' `
        -WorkingDirectory $bridgeRoot -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath

    @{ mode = $activeMode; pid = $process.Id; started = (Get-Date).ToString('o') } |
        ConvertTo-Json | Set-Content -LiteralPath $statePath -Encoding utf8

    foreach ($attempt in 1..30) {
        Start-Sleep -Milliseconds 500
        $process.Refresh()
        if ($process.HasExited) {
            throw "El puente terminó al iniciar. Revisa $stderrPath"
        }
        try {
            Invoke-WebRequest -UseBasicParsing -Uri 'http://127.0.0.1:8003/xiaozhi/ota/' `
                -TimeoutSec 2 | Out-Null
            return
        } catch {
            if ($attempt -eq 30) { throw 'El puente no respondió después de 15 segundos.' }
        }
    }
}

function Reset-XiDevice {
    if ($NoDeviceReset) {
        Write-Host 'Reinicia Shi manualmente para aplicar el modo.'
        return
    }
    if ((Test-Path -LiteralPath $idfPython) -and ([System.IO.Ports.SerialPort]::GetPortNames() -contains $Port)) {
        & $idfPython -m esptool --chip esp32s3 --port $Port chip_id | Out-Null
        Write-Host "Shi reiniciada mediante $Port."
    } else {
        Write-Host "No pude reiniciar Shi automáticamente. Reiníciala manualmente; puerto esperado: $Port."
    }
}

New-Item -ItemType Directory -Path $stateDir -Force | Out-Null

if ($Mode -eq 'Status') {
    $state = Get-XiState
    if ($null -eq $state) {
        Write-Host 'Modo registrado: XiaoZhi oficial o puente sin iniciar.'
    } else {
        $running = $null -ne (Get-Process -Id $state.pid -ErrorAction SilentlyContinue)
        Write-Host "Modo registrado: $($state.mode) | PID: $($state.pid) | activo: $running"
    }
    exit 0
}

if ($Mode -eq 'GPT' -and [string]::IsNullOrWhiteSpace($env:OPENAI_API_KEY)) {
    throw 'Falta OPENAI_API_KEY. No se cambió el modo ni se realizó ningún consumo.'
}

Stop-XiBridge

switch ($Mode) {
    'Ollama' {
        Set-ConfiguredLlm 'OllamaLLM'
        Ensure-Ollama
        Start-XiServer 'Ollama'
        Write-Host 'Modo activo: Ollama local. No consume tokens de OpenAI.'
    }
    'GPT' {
        Set-ConfiguredLlm 'OpenAILLM'
        Start-XiServer 'GPT'
        Write-Host 'Modo activo: GPT mediante OpenAI API. Este modo sí consume tokens.'
    }
    'XiaoZhi' {
        @{ mode = 'XiaoZhi'; pid = $null; started = (Get-Date).ToString('o') } |
            ConvertTo-Json | Set-Content -LiteralPath $statePath -Encoding utf8
        Write-Host 'Modo activo: XiaoZhi oficial. El puente local está detenido.'
    }
}

Reset-XiDevice
