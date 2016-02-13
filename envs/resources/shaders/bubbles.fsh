precision mediump float;

varying vec2 vtexcoord;

uniform sampler2D tex;
uniform float width;
uniform float height;
uniform float time;

const float radius = 0.05;
const int bubbles = 64;

void main()
{
  vec2 uv = -1.0 + 2.0 * vtexcoord;
  uv.x *= width / height;

  vec3 c = vec3(0.2, 0.2, 0.4);

  for (int i = 0; i < bubbles; i++) {
    float pha = tan(float(i) * 6.0 + 1.0) * 0.5 + 0.5;
    float siz = pow(cos(float(i) * 2.4 + 5.0) * 0.5 + 0.5, 4.0);
    float pox = cos(float(i) * 3.55 + 4.1) * width / height;
    float rad = radius + sin(float(i)) * 0.1 + 0.08;
    vec2 pos = vec2(pox + sin(time / 15.0 + pha + siz), -1.0 - rad + (2.0 + 2.0 * rad) * mod(pha + 0.1 * (time / 5.0) * (0.2 + 0.8 * siz), 1.0)) * vec2(1.0, 1.0);
    float dis = length(uv - pos);
    vec3 col = mix(vec3(0.1, 0.2, 0.1), vec3(0.2, 0.1, 0.1), 0.5 + 0.5 * sin(float(i) * sin(time * pox * 0.03) + 1.9));

    c += col.xyz * (1.0 - smoothstep(rad * (0.65 + 0.20 * sin(pox * time)), rad, dis)) * (1.0 - cos(pox * time));
  }

  gl_FragColor = vec4(c, 1.0);
}
