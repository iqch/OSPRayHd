// CRT
#include <iostream>
#include <algorithm>

using namespace std;

// PXR
#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/instantiateSingleton.h>

PXR_NAMESPACE_USING_DIRECTIVE

// OSPRAYHD

#include "config.h"


// Instantiate the config singleton.
TF_INSTANTIATE_SINGLETON(HdOSPRayConfig);

// Each configuration variable has an associated environment variable.
// The environment variable macro takes the variable name, a default value,
// and a description...
// clang-format off
TF_DEFINE_ENV_SETTING(HDOSPRAY_SAMPLES_PER_FRAME, -1,
	"Raytraced samples per pixel per frame (must be >= 1)");

TF_DEFINE_ENV_SETTING(HDOSPRAY_SAMPLES_TO_CONVERGENCE, 100,
	"Samples per pixel before we stop rendering (must be >= 1)");

TF_DEFINE_ENV_SETTING(HDOSPRAY_AMBIENT_OCCLUSION_SAMPLES, 0,
	"Ambient occlusion samples per camera ray (must be >= 0; a value of 0 disables ambient occlusion)");

TF_DEFINE_ENV_SETTING(HDOSPRAY_CAMERA_LIGHT_INTENSITY, 300,
	"Intensity of the camera light, specified as a percentage of <1,1,1>.");

TF_DEFINE_ENV_SETTING(HDOSPRAY_PRINT_CONFIGURATION, 0,
	"Should HdOSPRay print configuration on startup? (values > 0 are true)");

TF_DEFINE_ENV_SETTING(HDOSPRAY_USE_PATH_TRACING, 1,
	"Should HdOSPRay use path tracing");

TF_DEFINE_ENV_SETTING(HDOSPRAY_INIT_ARGS, "",
	"Initialization arguments sent to OSPRay");

TF_DEFINE_ENV_SETTING(HDOSPRAY_USE_DENOISER, 0,
	"OSPRay uses denoiser");

TF_DEFINE_ENV_SETTING(HDOSPRAY_FORCE_QUADRANGULATE, 0,
	"OSPRay force Quadrangulate meshes for debug");

TF_DEFINE_ENV_SETTING(HDOSPRAY_TILE_SIZE, 16,
	"OSPRay Rendering Tile Size");

HdOSPRayConfig::HdOSPRayConfig()
{
	// Read in values from the environment, clamping them to valid ranges.
	samplesPerFrame = std::max(-1, TfGetEnvSetting(HDOSPRAY_SAMPLES_PER_FRAME));

	samplesToConvergence = std::max(1, TfGetEnvSetting(HDOSPRAY_SAMPLES_TO_CONVERGENCE));

	ambientOcclusionSamples = std::max(0, TfGetEnvSetting(HDOSPRAY_AMBIENT_OCCLUSION_SAMPLES));

	cameraLightIntensity = (std::max(100, TfGetEnvSetting(HDOSPRAY_CAMERA_LIGHT_INTENSITY)) / 100.0f);

	usePathTracing = TfGetEnvSetting(HDOSPRAY_USE_PATH_TRACING);

	initArgs = TfGetEnvSetting(HDOSPRAY_INIT_ARGS);

	useDenoiser = TfGetEnvSetting(HDOSPRAY_USE_DENOISER);

	forceQuadrangulate = TfGetEnvSetting(HDOSPRAY_FORCE_QUADRANGULATE);

	tileSize = std::min(4, TfGetEnvSetting(HDOSPRAY_TILE_SIZE));


	if (TfGetEnvSetting(HDOSPRAY_PRINT_CONFIGURATION) > 0)
	{
		std::cout
			<< "HdOSPRay Configuration: \n"
			<< "  samplesPerFrame            = "
			<< samplesPerFrame << "\n"
			<< "  samplesToConvergence       = "
			<< samplesToConvergence << "\n"
			<< "  ambientOcclusionSamples    = "
			<< ambientOcclusionSamples << "\n"
			<< "  cameraLightIntensity      = "
			<< cameraLightIntensity << "\n"
			<< "  initArgs                  = "
			<< initArgs << "\n"
			<< "  tileSize                  = "
			<< tileSize << "\n"
			;
	}
}

const HdOSPRayConfig& HdOSPRayConfig::GetInstance()
{
	return TfSingleton<HdOSPRayConfig>::GetInstance();
}

