precision mediump float;

varying vec2 vtexcoord;

uniform sampler2D tex;
uniform float width;
uniform float height;
uniform float time;

const int pixels = 200;

void main()
{
	float sum = 0.0;
	float size = width / 800.0;

	for (int i = 0; i < pixels; i++) {
		vec2 p = vec2(width / 4.0, height / 4.0);
		float t = (float(i) + time) / 8.0;
		float c = (float(i) * 4.0);

		p.x += tan(8.0 * t + c) * width * 0.2;
		p.y += sin(t) * height * 0.8;

		sum += size / length(vec2(vtexcoord.x * width, vtexcoord.y * height) - p);
	}

	gl_FragColor = vec4(sum * 0.1, sum * 0.5, sum, 1.0);
}
