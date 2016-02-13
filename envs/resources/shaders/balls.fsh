precision mediump float;

varying vec2 vtexcoord;

uniform sampler2D tex;
uniform float width;
uniform float height;
uniform float time;

vec3 ball(vec2 p, float fx, float fy, float rx, float ry, vec3 c)
{
  vec2 r = vec2(p.x + sin(time * fx) * rx, p.y + cos(time * fx) * ry);

  return pow(0.04 / length(r), 0.6) * c;
}

void main()
{
  vec2 q = vtexcoord;
  vec2 p = vec2((2.0 * q.x - 1.0) * width / height, 2.0 * q.y - 1.0);

  vec3 c = vec3(0.0, 0.0, 0.0);
  c += ball(p, 1.0, 1.0, 0.5, 0.5, vec3(0.4, 0.3, 0.3));
  c += ball(p, 1.5, 1.5, 0.4, 0.2, vec3(0.2, 0.7, 0.2));
  c += ball(p, 2.0, 3.0, 0.5, 0.3, vec3(0.3, 0.6, 0.8));
  c += ball(p, 2.5, 3.5, 0.6, 0.4, vec3(0.7, 0.2, 0.5));
  c += ball(p, 3.0, 4.0, 0.7, 0.5, vec3(0.7, 0.5, 0.4));
  c += ball(p, 1.5, 2.5, 0.8, 0.6, vec3(0.6, 0.4, 0.6));
  c += ball(p, 3.1, 1.5, 0.9, 0.7, vec3(0.2, 0.1, 0.4));

  gl_FragColor = vec4(c, 1.0);
}
