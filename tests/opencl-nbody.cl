#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

__kernel void mutualgravity(__global float *positions, __global float *velocities, int count, float dt)
{
	int id = get_global_id(0);
	float ax, ay;
	float dx, dy;
	float invr;
	float f;
	int i;

	ax = 0.0f;
	ay = 0.0f;

	for (i = 0; i < count; i++) {
		dx = positions[i * 2 + 0] - positions[id * 2 + 0];
		dy = positions[i * 2 + 1] - positions[id * 2 + 1];

		invr = 1.0f / sqrt(dx * dx + dy * dy + 0.001f);

		f = 30.0f * invr * invr;

		ax += f * dx;
		ay += f * dy;
	}

	velocities[id * 2 + 0] = velocities[id * 2 + 0] + ax * dt;
	velocities[id * 2 + 1] = velocities[id * 2 + 1] + ay * dt;
}

__kernel void gravitywell(__global float *positions, __global float *velocities, int count, float dt, float cx, float cy, float intensity)
{
	int id = get_global_id(0);
	float ax, ay;
	float dx, dy;
	float invr;
	float f;

	dx = cx - positions[id * 2 + 0];
	dy = cy - positions[id * 2 + 1];

	invr = 1.0f / sqrt(dx * dx + dy * dy + 0.001f);

	f = intensity * invr * invr;

	ax = f * dx;
	ay = f * dy;

	velocities[id * 2 + 0] = velocities[id * 2 + 0] + ax * dt;
	velocities[id * 2 + 1] = velocities[id * 2 + 1] + ay * dt;
}

__kernel void update(__global float *positions, __global float *velocities, float dt, int width, int height)
{
	int id = get_global_id(0);
	int x, y;

	positions[id * 2 + 0] = positions[id * 2 + 0] + velocities[id * 2 + 0] * dt;
	positions[id * 2 + 1] = positions[id * 2 + 1] + velocities[id * 2 + 1] * dt;

	x = floor(positions[id * 2 + 0]);
	y = floor(positions[id * 2 + 1]);

	if (0 > x || x >= width || 0 > y || y >= height) {
		velocities[id * 2 + 0] = 0.0f;
		velocities[id * 2 + 1] = 0.0f;
	}
}

__kernel void render(__global char *framebuffer, int width, int height, __global float *positions)
{
	int id = get_global_id(0);
	int x, y;

	x = floor(positions[id * 2 + 0]);
	y = floor(positions[id * 2 + 1]);

	if (0 <= x && x < width && 0 <= y && y < height) {
		framebuffer[(y * width + x) * 4 + 0] = 0x0;
		framebuffer[(y * width + x) * 4 + 1] = 0xff;
		framebuffer[(y * width + x) * 4 + 2] = 0xff;
		framebuffer[(y * width + x) * 4 + 3] = 0xff;
	}
}

__kernel void clear(__global char *framebuffer, int width, int height)
{
	int x = get_global_id(0);
	int y = get_global_id(1);

	framebuffer[(y * width + x) * 4 + 0] = 0x0;
	framebuffer[(y * width + x) * 4 + 1] = 0x0;
	framebuffer[(y * width + x) * 4 + 2] = 0x0;
	framebuffer[(y * width + x) * 4 + 3] = 0xff;
}
