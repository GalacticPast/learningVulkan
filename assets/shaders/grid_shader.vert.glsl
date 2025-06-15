#version 460 core

// extents of grid in world coordinates
float gridSize = 100.0;

// size of one cell
float gridCellSize = 0.025;

// color of thin lines
vec4 gridColorThin = vec4(0.5, 0.5, 0.5, 1.0);

// color of thick lines (every tenth line)
vec4 gridColorThick = vec4(0.0, 0.0, 0.0, 1.0);

// minimum number of pixels between cell lines before LOD switch should occur.
const float gridMinPixelsBetweenCells = 2.0;

layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 view;
    mat4 projection;
    vec4 ambient_color;
    vec3 camera_position;
} global_ubo;

layout (location=0) out vec2 uv;
layout (location=1) out vec2 out_camPos;

const vec3 pos[4] = vec3[4](
  vec3(-1.0, 0.0, -1.0),
  vec3( 1.0, 0.0, -1.0),
  vec3( 1.0, 0.0,  1.0),
  vec3(-1.0, 0.0,  1.0)
);

const int indices[6] = int[6](
  0, 1, 2, 2, 3, 0
);
vec3 origin = vec3(0,0,0);

void main() {
  int idx = indices[gl_VertexIndex];
  vec3 position = pos[idx] * gridSize;


  position.x += global_ubo.camera_position.x;
  position.z += global_ubo.camera_position.z;

  position += origin.xyz;

  out_camPos = global_ubo.camera_position.xz;

    // there souuld be a model matrix but I think its a identity model matrix
  gl_Position = global_ubo.projection * global_ubo.view * vec4(position, 1.0);
  uv = position.xz;
}
