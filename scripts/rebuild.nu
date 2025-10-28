let pids = (ps | where name == "explorer.exe" | where cmd =~ "/factory,{75dff2b7-6936-4c06-a8bb-676a7b00b24b}" | get pid)
for pid in $pids {
    kill -f $pid
}

xmake b --yes inject
xmake b --yes shell
xmake r inject new