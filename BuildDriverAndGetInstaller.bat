@echo off
:: Set paths
set BUILD_DIR=External/softcam/temp_build
set SOURCE_DIR=External/softcam
set OUTPUT_DIR=Binaries

if not exist "%SOURCE_DIR%" (
	echo Failed to find Softcam library
	exit /b 1
)

:: Create the build directory if it doesn't exist
if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

:: Run CMake with the specified option
cmake -DBUILD_SOFTCAM_DRIVER=ON -S "%SOURCE_DIR%" -B "%BUILD_DIR%"
if errorlevel 1 (
    echo CMake configuration failed!
    exit /b 1
)

:: Build the project
cmake --build "%BUILD_DIR%" --config Release
if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

:: Ensure the output directory exists
if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
)

:: Copy the output files to the Driver folder
copy /y "%BUILD_DIR%\src\softcam\Release\softcam.dll" "%OUTPUT_DIR%\"
copy /y "%BUILD_DIR%\examples\softcam_installer\Release\softcam_installer.exe" "%OUTPUT_DIR%\"

:: Check if the files were copied successfully
if exist "%OUTPUT_DIR%\softcam.dll" (
    echo softcam.dll copied successfully.
) else (
    echo Failed to copy softcam.dll.
)

if exist "%OUTPUT_DIR%\softcam_installer.exe" (
    echo softcam_installer.exe copied successfully.
) else (
    echo Failed to copy softcam_installer.exe.
)

:: Delete the build directory
echo Removing build directory...
rmdir /s /q "%BUILD_DIR%"
if errorlevel 1 (
    echo Failed to remove build directory.
) else (
    echo Build directory removed successfully.
)

echo Build and cleanup process completed.
pause
