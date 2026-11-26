@echo off
rem WE3 Match Preview - quick match launcher (PES6_Match_Preview style)
rem Usage:  run_match_preview.bat                      -> opens the selection screen
rem         run_match_preview.bat Brazil Japan         -> jump straight into Brazil vs Japan (you control Home)
rem         run_match_preview.bat Brazil Japan cpu     -> CPU vs CPU
rem Edit GODOT below if your Godot lives elsewhere (or set the GODOT_EXE environment variable).
setlocal
if "%GODOT_EXE%"=="" set "GODOT_EXE=C:\Users\akihungac\Downloads\Godot_v4.7.2-stable_mono_win64\Godot_v4.7.2-stable_mono_win64\Godot_v4.7.2-stable_mono_win64.exe"
set "PROJECT=%~dp0.."
if "%~2"=="" (
  "%GODOT_EXE%" --path "%PROJECT%" "res://test_tools/match_preview.tscn"
) else (
  set "CTRL=home"
  if /I "%~3"=="cpu" set "CTRL=cpu"
  if /I "%~3"=="away" set "CTRL=away"
  "%GODOT_EXE%" --path "%PROJECT%" "res://test_tools/match_preview.tscn" -- --home=%~1 --away=%~2 --control=%CTRL% --autostart
)
endlocal
