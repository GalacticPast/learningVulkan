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
::
::echo %CC% %c_file_names% -o bin/%assembly% %includes% %compiler_flags%
::%CC% %c_file_names% -o bin/%assembly% %includes% %compiler_flags%


@echo OFF

setlocal EnableDelayedExpansion

::Get a list of all the .c files.
set c_file_names=

FOR /R %%f in (*.c) do (
    set c_file_names=!c_file_names! %%f
)

set assembly=learningVulkan
set compilerFlags=-g -shared -Wvarargs -Wall -Werror

set includeFlags=-Isrc -I%VULKAN_SDK%/Include
set linkerFlags=-luser32 -lvulkan-1 -L%VULKAN_SDK%/Lib
set defines=-D_DEBUG -DKEXPORT -D_CRT_SECURE_NO_WARNINGS -DPLATFORM_WINDOWS

echo "Building %assembly%%..."
clang %c_file_names% %compilerFlags% -o bin/%assembly%.exe %defines% %includeFlags% %linkerFlags%
