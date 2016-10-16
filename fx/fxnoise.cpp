#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <fastnoise.h>

#include <fxnoise.h>

struct fxnoise {
	FastNoise *fastnoise;
};

struct fxnoise *nemofx_noise_create(void)
{
	struct fxnoise *noise;

	noise = (struct fxnoise *)malloc(sizeof(struct fxnoise));
	if (noise == NULL)
		return NULL;
	memset(noise, 0, sizeof(struct fxnoise));

	noise->fastnoise = new FastNoise;

	return noise;
}

void nemofx_noise_destroy(struct fxnoise *noise)
{
	delete noise->fastnoise;

	free(noise);
}

void nemofx_noise_set_seed(struct fxnoise *noise, int seed)
{
	noise->fastnoise->SetSeed(seed);
}

int nemofx_noise_get_seed(struct fxnoise *noise)
{
	return noise->fastnoise->GetSeed();
}

void nemofx_noise_set_frequency(struct fxnoise *noise, float frequency)
{
	noise->fastnoise->SetFrequency(frequency);
}

void nemofx_noise_set_interpolation(struct fxnoise *noise, const char *interp)
{
	if (strcasecmp(interp, "linear") == 0)
		noise->fastnoise->SetInterp(FastNoise::Linear);
	else if (strcasecmp(interp, "hermite") == 0)
		noise->fastnoise->SetInterp(FastNoise::Hermite);
	else
		noise->fastnoise->SetInterp(FastNoise::Quintic);
}

void nemofx_noise_set_type(struct fxnoise *noise, const char *type)
{
	if (strcasecmp(type, "value") == 0)
		noise->fastnoise->SetNoiseType(FastNoise::Value);
	else if (strcasecmp(type, "value_fractal") == 0)
		noise->fastnoise->SetNoiseType(FastNoise::ValueFractal);
	else if (strcasecmp(type, "gradient") == 0)
		noise->fastnoise->SetNoiseType(FastNoise::Gradient);
	else if (strcasecmp(type, "gradient_fractal") == 0)
		noise->fastnoise->SetNoiseType(FastNoise::GradientFractal);
	else if (strcasecmp(type, "simplex") == 0)
		noise->fastnoise->SetNoiseType(FastNoise::Simplex);
	else if (strcasecmp(type, "simplex_fractal") == 0)
		noise->fastnoise->SetNoiseType(FastNoise::SimplexFractal);
	else if (strcasecmp(type, "cellular") == 0)
		noise->fastnoise->SetNoiseType(FastNoise::Cellular);
	else if (strcasecmp(type, "whitenoise") == 0)
		noise->fastnoise->SetNoiseType(FastNoise::WhiteNoise);
}

void nemofx_noise_set_fractal_octaves(struct fxnoise *noise, unsigned int octaves)
{
	noise->fastnoise->SetFractalOctaves(octaves);
}

void nemofx_noise_set_fractal_lacunarity(struct fxnoise *noise, float lacunarity)
{
	noise->fastnoise->SetFractalLacunarity(lacunarity);
}

void nemofx_noise_set_fractal_gain(struct fxnoise *noise, float gain)
{
	noise->fastnoise->SetFractalGain(gain);
}

void nemofx_noise_set_fractal_type(struct fxnoise *noise, const char *type)
{
	if (strcasecmp(type, "fbm") == 0)
		noise->fastnoise->SetFractalType(FastNoise::FBM);
	else if (strcasecmp(type, "billow") == 0)
		noise->fastnoise->SetFractalType(FastNoise::Billow);
	else if (strcasecmp(type, "rigidmulti") == 0)
		noise->fastnoise->SetFractalType(FastNoise::RigidMulti);
}

void nemofx_noise_set_cellular_distance_function(struct fxnoise *noise, const char *func)
{
	if (strcasecmp(func, "euclidean") == 0)
		noise->fastnoise->SetCellularDistanceFunction(FastNoise::Euclidean);
	else if (strcasecmp(func, "manhattan") == 0)
		noise->fastnoise->SetCellularDistanceFunction(FastNoise::Manhattan);
	else if (strcasecmp(func, "natural") == 0)
		noise->fastnoise->SetCellularDistanceFunction(FastNoise::Natural);
}

void nemofx_noise_set_cellular_return_type(struct fxnoise *noise, const char *type)
{
	if (strcasecmp(type, "cellvalue") == 0)
		noise->fastnoise->SetCellularReturnType(FastNoise::CellValue);
	else if (strcasecmp(type, "noiselookup") == 0)
		noise->fastnoise->SetCellularReturnType(FastNoise::NoiseLookup);
	else if (strcasecmp(type, "distance") == 0)
		noise->fastnoise->SetCellularReturnType(FastNoise::Distance);
	else if (strcasecmp(type, "distance2") == 0)
		noise->fastnoise->SetCellularReturnType(FastNoise::Distance2);
	else if (strcasecmp(type, "distance2add") == 0)
		noise->fastnoise->SetCellularReturnType(FastNoise::Distance2Add);
	else if (strcasecmp(type, "distance2sub") == 0)
		noise->fastnoise->SetCellularReturnType(FastNoise::Distance2Sub);
	else if (strcasecmp(type, "distance2mul") == 0)
		noise->fastnoise->SetCellularReturnType(FastNoise::Distance2Mul);
	else if (strcasecmp(type, "distance2div") == 0)
		noise->fastnoise->SetCellularReturnType(FastNoise::Distance2Div);
}

void nemofx_noise_set_cellular_noise_lookup(struct fxnoise *noise, struct fxnoise *lookup)
{
	noise->fastnoise->SetCellularNoiseLookup(lookup->fastnoise);
}

void nemofx_noise_set_position_warp(struct fxnoise *noise, float amp)
{
	noise->fastnoise->SetPositionWarpAmp(amp);
}

void nemofx_noise_dispatch(struct fxnoise *noise, void *buffer, int32_t width, int32_t height)
{
	uint8_t *pixels = (uint8_t *)buffer;
	float n;
	int x, y;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			n = noise->fastnoise->GetNoise(x, y) * 255.0f;

			pixels[(y * width) * 4 + x * 4 + 0] = n;
			pixels[(y * width) * 4 + x * 4 + 1] = n;
			pixels[(y * width) * 4 + x * 4 + 2] = n;
			pixels[(y * width) * 4 + x * 4 + 3] = 255;
		}
	}
}
