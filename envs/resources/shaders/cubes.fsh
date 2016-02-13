precision mediump float;

varying vec2 vtexcoord;

uniform sampler2D tex;
uniform float width;
uniform float height;
uniform float time;

void main()
{
  vec2 p = vtexcoord;
  float x = vtexcoord.x * width;
  float y = vtexcoord.y * height;
  float c = 0.0;

  vec4 res = vec4(int(mod(x * 1.75, 2.0)) - int(mod(y * 1.75, 2.0)), int(mod(x * 1.75, 2.0)) - int(mod(y * 1.75, 2.0)), int(mod(x * 1.75, 2.0)) - int(mod(y * 1.75, 2.0)), 1.0);
  res.r -= abs(sin(x / 50.0 + time));
  res.b -= abs(cos(y / 50.0 + time));
  res.g -= abs(tan((x + y) / 150.0 - time));

  gl_FragColor = res;
}
