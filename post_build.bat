@echo off

echo "compiling shaders"
for /r %%i in (*.vert) do (
    echo %VULKAN_SDK%/Bin/glslc %%i -o bin/%%~ni.vert.spv
    %VULKAN_SDK%/Bin/glslc %%i -o bin/%%~ni.vert.spv
)

for /r %%i in (*.frag) do (
    echo %VULKAN_SDK%/Bin/glslc %%i -o bin/%%~ni.frag.spv
    %VULKAN_SDK%/Bin/glslc %%i -o bin/%%~ni.frag.spv
)
