Add-Type -AssemblyName System.IO.Compression.FileSystem
Add-Type -AssemblyName System.IO.Compression
$fmuPath = 'C:/Users/shi/Desktop/matlab_git/GWDataFlowSimulator/FmuExportEngine/bin/Release/test.fmu'
$sourceDir = 'C:\Users\shi\Desktop\matlab_git\GWDataFlowSimulator\FmuExportEngine\bin\Release\_fmu_temp_test'
if (Test-Path $fmuPath) { Remove-Item $fmuPath -Force }
try {
    $zip = [System.IO.Compression.ZipFile]::Open($fmuPath, [System.IO.Compression.ZipArchiveMode]::Create)
    $files = Get-ChildItem -Path $sourceDir -Recurse -File
    foreach ($file in $files) {
        $relativePath = $file.FullName.Substring($sourceDir.Length)
        if ($relativePath.StartsWith('\') -or $relativePath.StartsWith('/')) {
            $relativePath = $relativePath.Substring(1)
        }
        # �ؼ�������б���滻Ϊ��б�ܣ����� ZIP ��׼
        $entryName = $relativePath -replace '\\', '/'
        $entry = $zip.CreateEntry($entryName, [System.IO.Compression.CompressionLevel]::Optimal)
        $fileBytes = [System.IO.File]::ReadAllBytes($file.FullName)
        $entryStream = $entry.Open()
        $entryStream.Write($fileBytes, 0, $fileBytes.Length)
        $entryStream.Close()
    }
    $zip.Dispose()
} catch { Write-Error $_.Exception.Message; exit 1 }
