$pids = (Get-WmiObject Win32_Process -Filter "name = 'explorer.exe'" | where { $_.CommandLine -like '*/factory,{75dff2b7-6936-4c06-a8bb-676a7b00b24b}*' }).ProcessId
foreach ($pidx in $pids) {
    Stop-Process -Id $pidx -Force
}


xmake b --yes inject
xmake b --yes shell && xmake r inject new