$ErrorActionPreference = 'Stop'

$secureKey = Read-Host 'Pega tu OpenAI API key' -AsSecureString
$pointer = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($secureKey)
try {
    $plainKey = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($pointer)
    if ([string]::IsNullOrWhiteSpace($plainKey)) {
        throw 'No se recibió ninguna clave.'
    }
    [Environment]::SetEnvironmentVariable('OPENAI_API_KEY', $plainKey, 'User')
    Write-Host 'OPENAI_API_KEY guardada para el usuario actual. No se escribió en el repositorio.'
} finally {
    [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($pointer)
    $plainKey = $null
}
