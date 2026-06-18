@echo off
REM To run DEBUG build, pass in DEBUG as an argument

set CMAKE_ARGS=-DFETCHCONTENT_SOURCE_DIR_RAYLIB=./thirdparty/raylib/
set SUB_DIR=Debug
set EXE_NAME=rlplays_game
:parse_args
if "%~1"=="" goto end_parse_args
if /i "%~1"=="DEBUG" (
    set CMAKE_ARGS=%CMAKE_ARGS% -DDEBUG=1
    set SUB_DIR=Debug
    echo Debug mode enabled
    goto next_arg
)
if /i "%~1"=="RELEASE" (
    set CMAKE_ARGS=%CMAKE_ARGS% -DRELEASE=1
    set SUB_DIR=Release
    echo Release mode enabled
    goto next_arg
)
if /i "%~1"=="EDITOR" (
    set CMAKE_ARGS=%CMAKE_ARGS%  -DRLPLAYS_EDITOR=1
    echo Editor and Debug mode enabled
    goto next_arg
)
if /i "%~1"=="CONVERTER" (
    set CMAKE_ARGS=%CMAKE_ARGS% -DRLPLAYS_CONVERTER=1
    set EXE_NAME=rlplays_converter
    echo Editor and Debug mode enabled
    goto next_arg
)
if /i "%~1"=="TEST" (
    set CMAKE_ARGS=%CMAKE_ARGS% -DRLPLAYS_TEST=1 -DDEBUG=1 -DDEBUG_TRACE=1
    set EXE_NAME=rlplays_test
    echo Test Debug mode enabled
    goto next_arg
)
REM Add other flags as needed
echo Unknown argument: %~1

:next_arg
shift
goto parse_args

:end_parse_args

echo ...............................
echo Building code using %CMAKE_ARGS%
echo To clean: call clean-build.cmd
echo ...............................
cmake -S ./gameui/ -B build %CMAKE_ARGS%
if %ERRORLEVEL% neq 0 (
    echo Build failed
    echo TIP: Make sure you have run clean-build.cmd between runs especially if cmake target changes.
    exit /b 1
)
cmake --build ./build -j20  
if %ERRORLEVEL% neq 0 (
    echo Build failed
    echo TIP: Make sure you have run clean-build.cmd between runs especially if cmake target changes.
    exit /b 1
)

echo ...............................
echo Launching .\build\%EXE_NAME%\%SUB_DIR%\%EXE_NAME%.exe
echo ...............................
.\build\%EXE_NAME%\%SUB_DIR%\%EXE_NAME%.exe
