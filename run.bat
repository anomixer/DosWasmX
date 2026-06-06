@echo off
set "control_rom=dist\MT32_CONTROL.ROM"
set "pcm_rom=dist\MT32_PCM.ROM"

if not exist "%control_rom%" goto :download
if not exist "%pcm_rom%" goto :download
goto :start_server

:download
echo Roland MT-32 ROM files not found in dist/. Downloading from archive.org...
powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://archive.org/download/mame-versioned-roland-mt-32-and-cm-32l-rom-files/MT-32_v1.07_legacy_ROM_files.zip' -OutFile 'mt32_roms.zip'"
echo Extracting ROM files to dist/...
powershell -Command "Expand-Archive -Path 'mt32_roms.zip' -DestinationPath 'dist' -Force"
del mt32_roms.zip
echo ROM files downloaded and extracted successfully!

:start_server
echo Starting local web server for DOS Wasm X...
npx http-server dist -p 8080
