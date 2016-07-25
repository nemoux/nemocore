#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

__kernel void dispatch(__global char *buffer, int width, int height)
{
	int x = get_global_id(0);
	int y = get_global_id(1);

	buffer[(y * width + x) * 4 + 0] = 0x0;
	buffer[(y * width + x) * 4 + 1] = 0xff;
	buffer[(y * width + x) * 4 + 2] = 0xff;
	buffer[(y * width + x) * 4 + 3] = 0xff;
}
