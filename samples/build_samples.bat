@echo off
setlocal
cd /d "%~dp0.."

if not defined VULKAN_SDK (
    echo VULKAN_SDK is not set, install the Vulkan SDK first
    exit /b 1
)

where cl >nul 2>nul && goto :compiler_ready
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq delims=" %%i in (`%VSWHERE% -latest -property installationPath`) do set VS_PATH=%%i
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul || exit /b 1
:compiler_ready

set OBJ=build\x64\Release\samples
set OUT=bin\x64\Release
set CFLAGS=/nologo /std:c++20 /Zc:preprocessor /EHsc /O2 /DNDEBUG /I src /I samples /I "%VULKAN_SDK%\Include"
set LFLAGS=/link /LIBPATH:"%VULKAN_SDK%\Lib" vulkan-1.lib user32.lib
set GLSLC="%VULKAN_SDK%\Bin\glslc.exe" -I src --target-env=vulkan1.3 -O
set REFLECT=%VULKAN_SDK%\Source\SPIRV-Reflect

if not exist %OBJ%\triangle mkdir %OBJ%\triangle
if not exist %OUT%\triangle mkdir %OUT%\triangle
if not exist %OBJ%\pathtracer mkdir %OBJ%\pathtracer
if not exist %OUT%\pathtracer mkdir %OUT%\pathtracer
if not exist %OBJ%\rt_gi mkdir %OBJ%\rt_gi
if not exist %OUT%\rt_gi mkdir %OUT%\rt_gi

echo building triangle
%GLSLC% samples\triangle\triangle.vert -o %OUT%\triangle\triangle.vert.spv || exit /b 1
%GLSLC% samples\triangle\triangle.frag -o %OUT%\triangle\triangle.frag.spv || exit /b 1
cl %CFLAGS% samples\triangle\main.cpp /Fo%OBJ%\triangle\ /Fe%OUT%\triangle\triangle.exe %LFLAGS% || exit /b 1

echo building pathtracer
%GLSLC% samples\pathtracer\pathtrace.comp -o %OUT%\pathtracer\pathtrace.comp.spv || exit /b 1
%GLSLC% samples\pathtracer\tonemap.vert -o %OUT%\pathtracer\tonemap.vert.spv || exit /b 1
%GLSLC% samples\pathtracer\tonemap.frag -o %OUT%\pathtracer\tonemap.frag.spv || exit /b 1
cl /nologo /O2 /DNDEBUG /c "%REFLECT%\spirv_reflect.c" /Fo%OBJ%\pathtracer\ || exit /b 1
cl %CFLAGS% /DGFX_USE_SPIRV_REFLECT=1 /I "%REFLECT%" samples\pathtracer\main.cpp %OBJ%\pathtracer\spirv_reflect.obj /Fo%OBJ%\pathtracer\ /Fe%OUT%\pathtracer\pathtracer.exe %LFLAGS% || exit /b 1

echo building rt_gi
%GLSLC% samples\rt_gi\trace.comp -o %OUT%\rt_gi\trace.comp.spv || exit /b 1
%GLSLC% samples\rt_gi\temporal.comp -o %OUT%\rt_gi\temporal.comp.spv || exit /b 1
%GLSLC% samples\rt_gi\svgf.comp -o %OUT%\rt_gi\svgf.comp.spv || exit /b 1
%GLSLC% samples\rt_gi\fullscreen.vert -o %OUT%\rt_gi\fullscreen.vert.spv || exit /b 1
%GLSLC% samples\rt_gi\composite.frag -o %OUT%\rt_gi\composite.frag.spv || exit /b 1
cl %CFLAGS% /DGFX_USE_SPIRV_REFLECT=1 /I "%REFLECT%" samples\rt_gi\main.cpp %OBJ%\pathtracer\spirv_reflect.obj /Fo%OBJ%\rt_gi\ /Fe%OUT%\rt_gi\rt_gi.exe %LFLAGS% || exit /b 1

echo done
