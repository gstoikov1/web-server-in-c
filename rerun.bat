@echo off

call compile.bat

if %errorlevel% neq 0 (
    exit /b %errorlevel%
)

start /wait "" server.exe