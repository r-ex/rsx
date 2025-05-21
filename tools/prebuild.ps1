param (
    [string]$projectDir
)

function Write-VSParsableErrorAndExit {
    param (
        [string]$Message,
        [int]$ErrorCode,
        [int]$Line = $MyInvocation.ScriptLineNumber
    )

    $file = $MyInvocation.ScriptName
    $formatted = "$file($Line): error PS{0:D4}: $Message" -f $ErrorCode
    [Console]::Error.WriteLine($formatted)
    exit $ExitCode
}

if (-not $projectDir) {
    Write-VSParsableErrorAndExit "Missing `projectDir` param" 1
}

$URL = "https://cdn.unrealengine.com/dependencies/UnrealEngine-40594131/3578368bc5d11cd2e80c63f7063c2828954c1562"
$OUT_PATH = Join-Path $projectDir "\thirdparty\radaudio\radaudio_decoder_win64.lib"
$START_OFFSET = 1693906
$SIZE = 330314
$PACK_UNCOMPRESSED_SIZE = 2024220
$RADA_HASH = "F7089A7A48B304CB54D00EB2BB75FD56A30923550BA4B74BAAE6FD7697BD1807"

if (Test-Path $OUT_PATH) {
    exit 0
}

try {
    $response = Invoke-WebRequest -Uri $URL -UseBasicParsing
}
catch {
    Write-VSParsableErrorAndExit "Error during request to UE CDN: $_" 2
}

try {
    $gzipStream = New-Object System.IO.Compression.GzipStream(
        $response.RawContentStream,
        [System.IO.Compression.CompressionMode]::Decompress
    )
    $memoryStream = New-Object System.IO.MemoryStream
    $gzipStream.CopyTo($memoryStream)
    $decompressedData = $memoryStream.ToArray()
}
catch {
    Write-VSParsableErrorAndExit "Error during decompression: $_" 3
}
finally {
    if ($gzipStream) { $gzipStream.Dispose() }
    if ($memoryStream) { $memoryStream.Dispose() }
}

if ($decompressedData.Length -ne $PACK_UNCOMPRESSED_SIZE) {
    Write-VSParsableErrorAndExit "Decompressed data size mismatch; expected $PACK_UNCOMPRESSED_SIZE, got $($decompressedData.Length)" 4
}

$dataSlice = $decompressedData[$START_OFFSET..($START_OFFSET + $SIZE - 1)]

$sha256 = [System.Security.Cryptography.SHA256]::Create()
$hashBytes = $sha256.ComputeHash($dataSlice)
$hash = [BitConverter]::ToString($hashBytes).Replace("-", "")

if ($hash.ToUpper() -ne $RADA_HASH.ToUpper()) {
    Write-VSParsableErrorAndExit "SHA-256 hash mismatch; got $hash" 5
}

try {
    [System.IO.File]::WriteAllBytes($OUT_PATH, $dataSlice)
}
catch {
    Write-VSParsableErrorAndExit "Error writing file to $($OUT_PATH): $_" 6
}

exit 0
