::@echo off
::setlocal enabledelayedexpansion  
::
::if not exist bin mkdir bin 
::
::set CC= "clang"
::set includes="-Isrc -IC:\VulkanSDK\1.3.296.0\Include"
::set assembly="learningVulkan"
::set compiler_flags="-D_DEBUG -Wall -Wextra -g3 -Wconversion -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion -fsanitize=undefined -fsanitize-trap"
::set linkerflags= -luser32.lib -lvulkan-1 -LC:\VulkanSDK\1.3.296.0\Lib 
::
::set c_file_names=
::for /R %%f in (*.c) do (
::    set c_file_names=!c_file_names! %%f
::)
::%VULKAN_SDK%/Bin/glslc %%i -o bin/%%~ni.spv
::
::echo %CC% %c_file_names% -o bin/%assembly% %includes% %compiler_flags%
::%CC% %c_file_names% -o bin/%assembly% %includes% %compiler_flags%


@echo OFF

setlocal EnableDelayedExpansion

for /r %%i in (*.frag, *.vert) do (
    echo %VULKAN_SDK%/Bin/glslc %%i -o bin/%%~ni.spv
    %VULKAN_SDK%/Bin/glslc %%i -o bin/%%~ni.spv
)


::Get a list of all the .c files.
set c_file_names=

for /R %%f in (*.c) do (
    set c_file_names=!c_file_names! %%f
)

set assembly=learningVulkan
set compiler_flags=-D_DEBUG -Wall -Wextra -g -Wconversion -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion -fsanitize=undefined -fsanitize-trap

set includes=-Isrc -I%VULKAN_SDK%/Include
set linker_flags= -luser32 -lvulkan-1 -L%VULKAN_SDK%/Lib

echo "Building %assembly%%..."
echo clang %c_file_names% %compiler_flags% -o bin/%assembly% %includes% %linker_flags% 
clang %c_file_names% %compiler_flags% -o bin/%assembly%.exe %includes% %linker_flags% 
