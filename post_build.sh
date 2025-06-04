echo "compiling shaders"
    
for file in assets/shaders/*.vert; do
    echo glslc ${file} -o assets/shaders/$(basename "$file" .vert).vert.spv
    glslc "$file" -o assets/shaders/$(basename "$file" .vert).vert.spv
done 

for file in assets/shaders/*.frag; do
    echo glslc ${file} -o assets/shaders/$(basename "$file" .frag).frag.spv
    glslc "$file" -o assets/shaders/$(basename "$file" .frag).frag.spv
done 

