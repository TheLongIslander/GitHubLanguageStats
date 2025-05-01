@echo off
setlocal

:: Check and install vcpkg if missing
IF NOT EXIST vcpkg (
    git clone https://github.com/microsoft/vcpkg.git
    cd vcpkg
    bootstrap-vcpkg.bat
    cd ..
)

:: Install required libraries (x64)
.\vcpkg\vcpkg install libgit2:x64-windows curl:x64-windows nlohmann-json:x64-windows

:: Set include and lib paths
set INCLUDE=%CD%\vcpkg\installed\x64-windows\include
set LIB=%CD%\vcpkg\installed\x64-windows\lib

:: Download cloc if not already present
IF NOT EXIST cloc.exe (
    powershell -Command "Invoke-WebRequest https://github.com/AlDanial/cloc/releases/latest/download/cloc-1.96.exe -OutFile cloc.exe"
)

:: Create obj directory
IF NOT EXIST obj (
    mkdir obj
)

:: Detect compiler and build accordingly
IF EXIST "C:\Program Files\Microsoft Visual Studio" (
    :: Compile each source to obj
    for %%f in (src\*.cpp) do (
        cl /EHsc /std:c++17 /I%INCLUDE% /Foobj\ %%f
    )
    :: Link all objs
    cl obj\*.obj /link /LIBPATH:%LIB% libgit2.lib libcurl.lib /OUT:github_stats.exe
) ELSE (
    :: MinGW fallback (all-in-one compile)
    g++ -std=c++17 -I%INCLUDE% src\*.cpp -o github_stats.exe -L%LIB% -lgit2 -lcurl
)

endlocal
echo Done. Run github_stats.exe
