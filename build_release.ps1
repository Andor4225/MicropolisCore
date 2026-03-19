$cmakePath = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$msbuildPath = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
$projectDir = "c:\Users\Gavin\Desktop\Projects\MicropolisCore"
$buildDir = "$projectDir\build"
$slnPath = "$buildDir\MicropolisCore.sln"

Write-Host "=== Regenerating CMake configuration ==="
& $cmakePath -S $projectDir -B $buildDir
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configure failed with exit code: $LASTEXITCODE"
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "=== Building with MSBuild ==="
& $msbuildPath $slnPath /p:Configuration=Release /t:Build /v:m
$exitCode = $LASTEXITCODE
Write-Host ""
Write-Host "MSBuild exit code: $exitCode"
exit $exitCode
