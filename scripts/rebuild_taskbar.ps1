$pids = (Get-WmiObject Win32_Process -Filter "name = 'rundll32.exe'" | where { $_.CommandLine -like 'taskbar' }).ProcessId
foreach ($pidx in $pids) {
    Stop-Process -Id $pidx -Force
}

xmake r --yes shell taskbar -- -taskbar