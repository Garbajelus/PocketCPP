@echo off
set "PATH=C:\msys64\ucrt64\bin;%PATH%"
(
    echo uci
    echo isready
    echo position startpos
    echo go depth 3
    echo quit
) | "%~dp0pocketcpp.exe"
pause