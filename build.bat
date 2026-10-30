@echo off
REM Windows : lancer depuis "x64 Native Tools Command Prompt for VS" (faust et cmake dans le PATH)
cmake -B build -DCMAKE_BUILD_TYPE=Release %*
if errorlevel 1 exit /b 1
cmake --build build --config Release
echo.
echo VST3 : build\EulerDisk_artefacts\Release\VST3\Euler Disk.vst3
echo Copier ce dossier dans  C:\Program Files\Common Files\VST3\
