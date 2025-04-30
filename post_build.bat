@echo off

echo "compiling shaders"
for /r %%i in (*.vert) do (
    echo %VULKAN_SDK%/Bin/glslc %%i -o assets/shaders/%%~ni.vert.spv
    %VULKAN_SDK%/Bin/glslc %%i -o assets/shaders/%%~ni.vert.spv
)

for /r %%i in (*.frag) do (
    echo %VULKAN_SDK%/Bin/glslc %%i -o assets/shaders/%%~ni.frag.spv
    %VULKAN_SDK%/Bin/glslc %%i -o assets/shaders/%%~ni.frag.spv
)
