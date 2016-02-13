precision mediump float;

varying vec2 vtexcoord;

uniform sampler2D tex;
uniform float width;
uniform float height;
uniform float time;

#define MAX_ITERATION 	(3)

void main()
{
  vec2 p = vtexcoord * 8.0 - vec2(20.0);
  vec2 i = p;
  float c = 1.0;
  float inten = 0.05;

  for (int n = 0; n < MAX_ITERATION; n++) {
    float t = time * (1.0 - (3.0 / float(n + 1)));

    i = p + vec2(cos(t - i.x) + sin(t + i.y), sin(t - i.y) + cos(t + i.x));
    c += 1.0 / length(vec2(p.x / (sin(i.x + t) / inten), p.y / (cos(i.y + t) / inten)));
  }

  c /= float(MAX_ITERATION);
  c = 1.5 - sqrt(c);

  gl_FragColor.rgb = vec3(0.001, 0.1, 0.1) * (1.0 / (1.0 - (c + 0.05)));
  gl_FragColor.a = 1.0;
}
