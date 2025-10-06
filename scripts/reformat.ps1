Get-ChildItem -Path ./src -Recurse -Include "*.h", "*.cpp", "*.hpp", "*.cc" | ForEach-Object {
    clang-format -i -style=file $_.FullName
}