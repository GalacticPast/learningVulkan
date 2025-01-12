@echo off

echo "compiling shaders"
for /r %%i in (*.frag, *.vert) do (
    echo %VULKAN_SDK%/Bin/glslc %%i -o bin/%%~ni.spv
    %VULKAN_SDK%/Bin/glslc %%i -o bin/%%~ni.spv
)
