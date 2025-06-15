#!/bin/bash

echo "Compiling shaders..."

for file in assets/shaders/*.vert.glsl; do
    base=$(basename "$file" .glsl)
    echo glslc "$file" -o assets/shaders/"$base".spv
    glslc -fshader-stage=vertex "$file" -o assets/shaders/"$base".spv
done

for file in assets/shaders/*.frag.glsl; do
    base=$(basename "$file" .glsl)
    echo glslc "$file" -o assets/shaders/"$base".spv
    glslc -fshader-stage=fragment "$file" -o assets/shaders/"$base".spv
done

