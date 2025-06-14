#version 460 core

layout (location=0) in vec2 uv;
layout (location=1) in vec2 camPos;
layout (location=0) out vec4 out_FragColor;
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

float log10(float x) {
  return log(x) / log(10.0);
}

float satf(float x) {
  return clamp(x, 0.0, 1.0);
}

vec2 satv(vec2 x) {
  return clamp(x, vec2(0.0), vec2(1.0));
}

float max2(vec2 v) {
  return max(v.x, v.y);
}

vec4 gridColor(vec2 uv, vec2 camPos) {
  vec2 dudv = vec2(
    length(vec2(dFdx(uv.x), dFdy(uv.x))),
    length(vec2(dFdx(uv.y), dFdy(uv.y)))
  );

  float lodLevel = max(0.0, log10((length(dudv) * gridMinPixelsBetweenCells) / gridCellSize) + 1.0);
  float lodFade = fract(lodLevel);

  // cell sizes for lod0, lod1 and lod2
  float lod0 = gridCellSize * pow(10.0, floor(lodLevel));
  float lod1 = lod0 * 10.0;
  float lod2 = lod1 * 10.0;

  // each anti-aliased line covers up to 4 pixels
  dudv *= 4.0;

  // set grid coordinates to the centers of anti-aliased lines for subsequent alpha calculations
  uv += dudv * 0.5;

  // calculate absolute distances to cell line centers for each lod and pick max X/Y to get coverage alpha value
  float lod0a = max2( vec2(1.0) - abs(satv(mod(uv, lod0) / dudv) * 2.0 - vec2(1.0)) );
  float lod1a = max2( vec2(1.0) - abs(satv(mod(uv, lod1) / dudv) * 2.0 - vec2(1.0)) );
  float lod2a = max2( vec2(1.0) - abs(satv(mod(uv, lod2) / dudv) * 2.0 - vec2(1.0)) );

  uv -= camPos;

  // blend between falloff colors to handle LOD transition
  vec4 c = lod2a > 0.0 ? gridColorThick : lod1a > 0.0 ? mix(gridColorThick, gridColorThin, lodFade) : gridColorThin;

  // calculate opacity falloff based on distance to grid extents
  float opacityFalloff = (1.0 - satf(length(uv) / gridSize));

  // blend between LOD level alphas and scale with opacity falloff
  c.a *= (lod2a > 0.0 ? lod2a : lod1a > 0.0 ? lod1a : (lod0a * (1.0-lodFade))) * opacityFalloff;

  return c;
}

void main() {
    vec4 grid_color = gridColor(uv, camPos);
    out_FragColor = grid_color; // render grid line color
}

