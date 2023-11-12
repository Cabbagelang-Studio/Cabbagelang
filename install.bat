@echo off

echo Installing Cabbagelang..
set installation_dir=%1

if not exist %1\\ (
    echo Installation directory not specified or doesn't exist, installation aborted.
    exit /b
)

echo Installing at %1

if exist cabbage.exe (
    if exist cabbagew.exe (
        copy cabbage.exe %installation_dir%\\cabbage.exe
        copy cabbagew.exe %installation_dir%\\cabbagew.exe
	mkdir %installation_dir%\\leaves
        setx CABBAGELANG_HOME %installation_dir%
        
    ) else (
        echo cabbagew.exe does not exist, installation aborted.
        exit /b
    )
) else (
    echo cabbage.exe does not exist, installation aborted.
    exit /b
)

echo Installed successfully.