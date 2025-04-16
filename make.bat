@echo off
IF NOT EXIST vcpkg (
    git clone https://github.com/microsoft/vcpkg.git
    cd vcpkg
    bootstrap-vcpkg.bat
    cd ..
)
.\vcpkg\vcpkg install libgit2 curl nlohmann-json
set INCLUDE=%CD%\vcpkg\installed\x64-windows\include
set LIB=%CD%\vcpkg\installed\x64-windows\lib
IF NOT EXIST cloc.exe (
    powershell -Command "Invoke-WebRequest https://github.com/AlDanial/cloc/releases/latest/download/cloc-1.96.exe -OutFile cloc.exe"
)
IF EXIST "C:\Program Files\Microsoft Visual Studio" (
    cl /EHsc /std:c++17 /I%INCLUDE% src\*.cpp /link /LIBPATH:%LIB% libgit2.lib libcurl.lib
) ELSE (
    g++ -std=c++17 -I%INCLUDE% src\*.cpp -o github_stats.exe -L%LIB% -lgit2 -lcurl
)
echo Done. Run github_stats.exe
