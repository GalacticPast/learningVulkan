echo "compiling shaders"
    
for file in assets/shaders/*.vert; do
    echo glslc ${file} -o bin/$(basename "$file" .vert).spv
    glslc "$file" -o bin/$(basename "$file" .vert).spv
done 

for file in assets/shaders/*.frag; do
    echo glslc ${file} -o bin/$(basename "$file" .frag).spv
    glslc "$file" -o bin/$(basename "$file" .frag).spv
done 

