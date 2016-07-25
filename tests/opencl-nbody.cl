#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

__kernel void clear(__global char *framebuffer, int width, int height)
{
	int x = get_global_id(0);
	int y = get_global_id(1);

	framebuffer[(y * width + x) * 4 + 0] = 0x0;
	framebuffer[(y * width + x) * 4 + 1] = 0x0;
	framebuffer[(y * width + x) * 4 + 2] = 0x0;
	framebuffer[(y * width + x) * 4 + 3] = 0xff;
}

__kernel void dispatch(__global float *velocities, __global float *positions, __global char *framebuffer, float dt, int count, int width, int height)
{
	int id = get_global_id(0);
	float ax, ay;
	float dx, dy;
	float invr, invr3;
	float f;
	int x, y;
	int i;

	ax = 0.0f;
	ay = 0.0f;

	for (i = 0; i < count; i++) {
		dx = positions[i * 2 + 0] - positions[id * 2 + 0];
		dy = positions[i * 2 + 1] - positions[id * 2 + 1];

		invr = 1.0f / sqrt(dx * dx + dy * dy + 1.0f);
		invr3 = invr * invr * invr;

		f = 1.0f * invr3;

		ax += f * dx;
		ay += f * dy;
	}

	positions[id * 2 + 0] = positions[id * 2 + 0] + velocities[id * 2 + 0] * dt + 0.5f * ax * dt * dt;
	positions[id * 2 + 1] = positions[id * 2 + 1] + velocities[id * 2 + 1] * dt + 0.5f * ay * dt * dt;
	velocities[id * 2 + 0] = velocities[id * 2 + 0] + ax * dt;
	velocities[id * 2 + 1] = velocities[id * 2 + 1] + ay * dt;

	x = floor(positions[id * 2 + 0]);
	y = floor(positions[id * 2 + 1]);

	framebuffer[(y * width + x) * 4 + 0] = 0x0;
	framebuffer[(y * width + x) * 4 + 1] = 0xff;
	framebuffer[(y * width + x) * 4 + 2] = 0xff;
	framebuffer[(y * width + x) * 4 + 3] = 0xff;
}
