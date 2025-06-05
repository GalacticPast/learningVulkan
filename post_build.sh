echo "compiling shaders"

for file in assets/shaders/*.vert.glsl; do
    echo glslc ${file} -o assets/shaders/$(basename "$file" .vert).spv
    glslc -fshader-stage=vertex "$file" -o assets/shaders/$(basename "$file" .vert).spv
done

for file in assets/shaders/*.frag.glsl; do
    echo glslc ${file} -o assets/shaders/$(basename "$file" .frag).spv
    glslc -fshader-stage=fragment "$file" -o assets/shaders/$(basename "$file" .frag).spv
done

