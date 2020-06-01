#pragma once

// CRT
#include <string>
#include <vector>

using namespace std;

// PXR
#include <pxr/base/tf/singleton.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_USING_DIRECTIVE

// OSPRAY

#include <ospray/ospray.h>


/// \class HdOSPRayConfig
///
/// This class is a singleton, holding configuration parameters for HdOSPRay.
/// Everything is provided with a default, but can be overridden using
/// environment variables before launching a hydra process.
///
/// Many of the parameters can be used to control quality/performance
/// tradeoffs, or to alter how HdOSPRay takes advantage of parallelism.
///
/// At startup, this class will print config parameters if
/// *HDOSPRAY_PRINT_CONFIGURATION* is true. Integer values greater than zero
/// are considered "true".
///
class HdOSPRayConfig 
{
public:
	/// \brief Return the configuration singleton.
	static const HdOSPRayConfig& GetInstance();

	/// How many samples does each pixel get per frame?
	///
	/// Override with *HDOSPRAY_SAMPLES_PER_FRAME*.
	unsigned int samplesPerFrame;

	/// How many samples do we need before a pixel is considered
	/// converged?
	///
	/// Override with *HDOSPRAY_SAMPLES_TO_CONVERGENCE*.
	unsigned int samplesToConvergence;

	/// How many ambient occlusion rays should we generate per
	/// camera ray?
	///
	/// Override with *HDOSPRAY_AMBIENT_OCCLUSION_SAMPLES*.
	unsigned int ambientOcclusionSamples;

	/// What should the intensity of the camera light be, specified as a
	/// percent of <1, 1, 1>.  For example, 300 would be <3, 3, 3>.
	///
	/// Override with *HDOSPRAY_CAMERA_LIGHT_INTENSITY*.
	float cameraLightIntensity;

	///  Whether OSPRay uses path tracing or scivis renderer.
	///
	/// Override with *HDOSPRAY_USE_PATHTRACING*.
	bool usePathTracing{ true };

	///  Whether OSPRay uses denoiser
	///
	/// Override with *HDOSPRAY_USE_DENOISER*.
	bool useDenoiser;

	/// Initialization arguments sent to OSPRay.
	///  This can be used to set ospray configurations like mpi.
	///
	/// Override with *HDOSPRAY_INIT_ARGS*.
	std::string initArgs;

	///  Whether OSPRay forces quad meshes for debug
	///
	/// Override with *HDOSPRAY_FORCE_QUADRANGULATE*.
	bool forceQuadrangulate;

	///  Maximum ray depth
	///
	/// Override with *HDOSPRAY_MAX_DEPTH*.
	int maxDepth{ 8 };

	///  Ao rays maximum distance
	///
	/// Override with *HDOSPRAY_AO_DISTANCE*.
	float aoDistance{ 15.0f };

	///  Use an ambient light
	///
	/// Override with *HDOSPRAY_AMBIENT_LIGHT*.
	bool ambientLight{ true };

	///  Use an eye light
	///
	/// Override with *HDOSPRAY_STATIC_DIRECTIONAL_LIGHTS*.
	bool staticDirectionalLights{ true };

	///  Use an eye light
	///
	/// Override with *HDOSPRAY_EYE_LIGHT*.
	bool eyeLight{ false };

	///  Use a key light
	///
	/// Override with *HDOSPRAY_KEY_LIGHT*.
	bool keyLight{ true };

	///  Use a fill light
	///
	/// Override with *HDOSPRAY_FILL_LIGHT*.
	bool fillLight{ true };

	///  Use a back light
	///
	/// Override with *HDOSPRAY_BACK_LIGHT*.
	bool backLight{ true };

	/// Override with *HDOSPRAY_TILE_SIZE*.
	int tileSize{ 16 };

	// meshes populate global instances.  These are then committed by the
	// renderPass into a scene.
	std::vector<OSPInstance> ospInstances;

private:
	// The constructor initializes the config variables with their
	// default or environment-provided override, and optionally prints
	// them.
	HdOSPRayConfig();
	~HdOSPRayConfig() = default;

	HdOSPRayConfig(const HdOSPRayConfig&) = delete;
	HdOSPRayConfig& operator=(const HdOSPRayConfig&) = delete;

	friend class TfSingleton<HdOSPRayConfig>;
};

