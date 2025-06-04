@echo off

echo "compiling shaders"
for /r %%i in (*.vert.glsl) do (
    echo %VULKAN_SDK%/Bin/glslc %%i -o assets/shaders/%%~ni.spv
    %VULKAN_SDK%/Bin/glslc -fshader-stage=vertex %%i -o assets/shaders/%%~ni.spv
)

for /r %%i in (*.frag.glsl) do (
    echo %VULKAN_SDK%/Bin/glslc %%i -o assets/shaders/%%~ni.spv
    %VULKAN_SDK%/Bin/glslc -fshader-stage=fragment %%i -o assets/shaders/%%~ni.spv
)
