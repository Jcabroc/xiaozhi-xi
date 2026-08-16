param(
    [switch]$SkipWarmup
)

$ErrorActionPreference = 'Stop'
$env:PYTHONIOENCODING = 'utf-8'
$env:PYTHONUTF8 = '1'

$bridgeRoot = 'C:\Users\Gir\Documents\ChatGPT\XiaoZhi\xiaozhi-esp32-server\main\xiaozhi-server'
$pythonExe = 'C:\ESP32_Projects\xi-bridge-venv\Scripts\python.exe'
$ollamaExe = 'C:\Users\Gir\AppData\Local\Programs\Ollama\ollama.exe'
$opusBin = 'C:\ESP32_Projects\libopus-runtime\Library\bin'
$ffmpegRoot = Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages\BtbN.FFmpeg.LGPL.Shared.8.1_Microsoft.Winget.Source_8wekyb3d8bbwe'
$ffmpegExe = Get-ChildItem -LiteralPath $ffmpegRoot -Recurse -Filter ffmpeg.exe -File |
    Select-Object -First 1 -ExpandProperty FullName

foreach ($requiredPath in @($bridgeRoot, $pythonExe, $ollamaExe, $opusBin, $ffmpegExe)) {
    if (-not (Test-Path -LiteralPath $requiredPath)) {
        throw "Falta un componente del puente: $requiredPath"
    }
}

$env:PATH = "$opusBin;$(Split-Path -Parent $ffmpegExe);$env:PATH"

try {
    Invoke-RestMethod -Uri 'http://127.0.0.1:11434/api/version' -TimeoutSec 2 | Out-Null
} catch {
    Start-Process -FilePath $ollamaExe -ArgumentList 'serve' -WindowStyle Hidden
    $ollamaReady = $false
    foreach ($attempt in 1..20) {
        Start-Sleep -Milliseconds 500
        try {
            Invoke-RestMethod -Uri 'http://127.0.0.1:11434/api/version' -TimeoutSec 2 | Out-Null
            $ollamaReady = $true
            break
        } catch {
            # Continue waiting for Ollama.
        }
    }
    if (-not $ollamaReady) {
        throw 'Ollama no respondió después de 10 segundos.'
    }
}

if (-not $SkipWarmup) {
    Write-Host 'Preparando el cerebro local de Xi...'
    $warmupBody = @{
        model = 'qwen3.5:4b'
        stream = $false
        think = $false
        keep_alive = '30m'
        messages = @(@{ role = 'user'; content = 'Responde solamente: lista.' })
        options = @{ num_ctx = 2048; num_predict = 8 }
    } | ConvertTo-Json -Depth 5

    $warmupRequest = @{
        Uri = 'http://127.0.0.1:11434/api/chat'
        Method = 'Post'
        ContentType = 'application/json'
        Body = $warmupBody
        TimeoutSec = 120
    }
    Invoke-RestMethod @warmupRequest | Out-Null
}

Write-Host 'Xi local disponible en ws://192.168.1.89:8000/xiaozhi/v1/'
Write-Host 'Mantén esta ventana abierta. Ctrl+C detiene el puente.'
Set-Location -LiteralPath $bridgeRoot
& $pythonExe app.py
