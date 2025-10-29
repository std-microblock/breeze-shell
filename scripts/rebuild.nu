chcp 65001
ps -l | where name == "explorer.exe" | where command =~ "/factory,{75dff2b7-6936-4c06-a8bb-676a7b00b24b}" | each { kill -f $in.pid; }

xmake b --yes inject
xmake b --yes shell
xmake r inject new