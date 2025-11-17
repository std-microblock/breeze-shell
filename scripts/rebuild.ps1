$pids = (Get-WmiObject Win32_Process -Filter "name = 'explorer.exe'" | where { $_.CommandLine -like '*/factory,{75dff2b7-6936-4c06-a8bb-676a7b00b24b}*' }).ProcessId
foreach ($pidx in $pids) {
    Stop-Process -Id $pidx -Force
}

xmake f --toolchain=clang-cl -m releasedbg -y
xmake b --yes inject
xmake b --yes shell
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
xmake r inject new