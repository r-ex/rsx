param (
    [string]$projectDir
)

if (-not $projectDir) {
    exit 1
}

$URL = "https://cdn.unrealengine.com/dependencies/UnrealEngine-40594131/3578368bc5d11cd2e80c63f7063c2828954c1562"
$OUT_PATH = $projectDir + "thirdparty\radaudio\radaudio_decoder_win64.lib"
$START_OFFSET = 1693906
$SIZE = 330314

if (Test-Path $OUT_PATH) {
    exit 0
}

$response = Invoke-WebRequest -Uri $URL
$gzipStream = New-Object System.IO.Compression.GzipStream(
    $response.RawContentStream,
    [System.IO.Compression.CompressionMode]::Decompress
)

try {
    $memoryStream = New-Object System.IO.MemoryStream

    $gzipStream.CopyTo($memoryStream)
    $decompressedData = $memoryStream.ToArray()
} finally {
    $gzipStream.Close()
    $memoryStream.Close()
}

$dataSlice = $decompressedData[$START_OFFSET..($START_OFFSET + $SIZE - 1)]

[System.IO.File]::WriteAllBytes($OUT_PATH, $dataSlice)
