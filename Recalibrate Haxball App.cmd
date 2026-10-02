@echo off
rem Optional: measures this PC's input latency for a few minutes, stores the
rem winning profile, then opens the game. Close Haxball App first.
start "" powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File "%~dp0scripts\run.ps1"
