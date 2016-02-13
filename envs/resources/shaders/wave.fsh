precision mediump float;

varying vec2 vtexcoord;

uniform sampler2D tex;
uniform float width;
uniform float height;
uniform float time;

void main()
{
  float battery = 1.0;
  vec2 p = vtexcoord * 2.0 - 1.0;
  vec3 c = vec3(1.0 - battery, 0.3, battery);

  c *= abs(1.0 / (sin(p.y + sin(p.x + time) * battery) * 30.0));

  gl_FragColor = vec4(c, 1.0);
}
