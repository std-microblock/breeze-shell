$pids = (Get-WmiObject Win32_Process -Filter "name = 'rundll32.exe'" | where { $_.CommandLine -like '-breeze-asan' }).ProcessId
foreach ($pidx in $pids) {
    Stop-Process -Id $pidx -Force
}

xmake f --toolchain=clang-cl -m releasedbg -y --asan=y
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
xmake r --yes -v asan_test