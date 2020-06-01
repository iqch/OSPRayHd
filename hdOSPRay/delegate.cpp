// CRT

#include <iostream>
using namespace std;

// PXR
//#include <pxr/imaging/glf/glew.h>

#include <pxr/pxr.h>

//#include <pxr/imaging/hd/extComputation.h>
#include <pxr/imaging/hd/resourceRegistry.h>
#include <pxr/imaging/hd/tokens.h>

//#include <pxr/imaging/hd/camera.h>

#include <pxr/imaging/hd/rprim.h>
#include <pxr/imaging/hd/sprim.h>
#include <pxr/imaging/hd/bprim.h>

PXR_NAMESPACE_USING_DIRECTIVE

// OSPRAY

// HDOSPRAY

#include "config.h"
#include "param.h"

#include "delegate.h"

//#include "renderer.h"

#include "pass.h"


//#include "instancer.h"

#include "mesh.h"


// RENDER SETTINGS
#define HDOSPRAY_RENDER_SETTINGS_TOKENS                                        \
    (ambientOcclusionSamples)(samplesPerFrame)(useDenoiser)(maxDepth)(         \
           aoDistance)(samplesToConvergence)(ambientLight)(eyeLight)(          \
           keyLight)(fillLight)(backLight)(pathTracer)(staticDirectionalLights)(tileSize)

TF_DECLARE_PUBLIC_TOKENS(HdOSPRayRenderSettingsTokens,
	HDOSPRAY_RENDER_SETTINGS_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdOSPRayRenderSettingsTokens, 
	HDOSPRAY_RENDER_SETTINGS_TOKENS);

HdRenderSettingDescriptorList
HdOSPRayRenderDelegate::GetRenderSettingDescriptors() const { return _settingDescriptors; };

HdRenderParam* HdOSPRayRenderDelegate::GetRenderParam() const {	return _renderParam.get(); };

// SUPPORTED TYPES
// RPRIM
const TfTokenVector HdOSPRayRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
	HdPrimTypeTokens->mesh,
};

TfTokenVector const&
HdOSPRayRenderDelegate::GetSupportedRprimTypes() const
{
	return SUPPORTED_RPRIM_TYPES;
};

HdRprim *
HdOSPRayRenderDelegate::CreateRprim(TfToken const& typeId,
	SdfPath const& rprimId,
	SdfPath const& instancerId)
{
	std::cout << "Create Tiny Rprim type=" << typeId.GetText()
		<< " id=" << rprimId
		<< " instancerId=" << instancerId
		<< std::endl;

	if (typeId == HdPrimTypeTokens->mesh)
	{
		return new HdOSPRayMesh(rprimId, instancerId);
	}
	else {
		TF_CODING_ERROR("Unknown Rprim type=%s id=%s",
			typeId.GetText(),
			rprimId.GetText());
	}
	return nullptr;
}

void
HdOSPRayRenderDelegate::DestroyRprim(HdRprim *rPrim)
{
	std::cout << "Destroy Tiny Rprim id=" << rPrim->GetId() << std::endl;
	delete rPrim;
}

// INSTANCER SUPPORT
HdInstancer *
HdOSPRayRenderDelegate::CreateInstancer(
	HdSceneDelegate *delegate,
	SdfPath const& id,
	SdfPath const& instancerId)
{
	TF_CODING_ERROR("Creating Instancer not supported id=%s instancerId=%s",
		id.GetText(), instancerId.GetText());
	return nullptr;
}

void
HdOSPRayRenderDelegate::DestroyInstancer(HdInstancer *instancer)
{
	TF_CODING_ERROR("Destroy instancer not supported");
}

// SPRIM
const TfTokenVector HdOSPRayRenderDelegate::SUPPORTED_SPRIM_TYPES =
{
	//HdPrimTypeTokens->camera,
	//HdPrimTypeTokens->material,
};

TfTokenVector const&
HdOSPRayRenderDelegate::GetSupportedSprimTypes() const
{
	return SUPPORTED_SPRIM_TYPES;
};

HdSprim *
HdOSPRayRenderDelegate::CreateSprim(TfToken const& typeId,
	SdfPath const& sprimId)
{
	TF_CODING_ERROR("Unknown Sprim type=%s id=%s",
		typeId.GetText(),
		sprimId.GetText());
	return nullptr;
}

HdSprim *
HdOSPRayRenderDelegate::CreateFallbackSprim(TfToken const& typeId)
{
	TF_CODING_ERROR("Creating unknown fallback sprim type=%s",
		typeId.GetText());
	return nullptr;
};

void
HdOSPRayRenderDelegate::DestroySprim(HdSprim *sPrim)
{
	TF_CODING_ERROR("Destroy Sprim not supported");
};


// BPRIM
const TfTokenVector HdOSPRayRenderDelegate::SUPPORTED_BPRIM_TYPES =
{
	//HdPrimTypeTokens->renderBuffer,
};

TfTokenVector const&
HdOSPRayRenderDelegate::GetSupportedBprimTypes() const
{
	return SUPPORTED_BPRIM_TYPES;
}

HdBprim *
HdOSPRayRenderDelegate::CreateBprim(TfToken const& typeId, SdfPath const& bprimId)
{
	TF_CODING_ERROR("Unknown Bprim type=%s id=%s",
		typeId.GetText(),
		bprimId.GetText());
	return nullptr;
}

HdBprim *
HdOSPRayRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
	TF_CODING_ERROR("Creating unknown fallback bprim type=%s",
		typeId.GetText());
	return nullptr;
}

void
HdOSPRayRenderDelegate::DestroyBprim(HdBprim *bPrim)
{
	TF_CODING_ERROR("Destroy Bprim not supported");
}

// STATICS
std::mutex HdOSPRayRenderDelegate::_mutexResourceRegistry;
std::atomic_int HdOSPRayRenderDelegate::_counterResourceRegistry;
HdResourceRegistrySharedPtr HdOSPRayRenderDelegate::_resourceRegistry;

// CTOR/DTOR
HdOSPRayRenderDelegate::~HdOSPRayRenderDelegate()
{
	cout << "HDOSPRAY : RD DELETE\n";
	_resourceRegistry.reset();
}

// RESUORCES
HdResourceRegistrySharedPtr
HdOSPRayRenderDelegate::GetResourceRegistry() const
{
	return _resourceRegistry;
}

void
HdOSPRayRenderDelegate::CommitResources(HdChangeTracker *tracker)
{
	std::cout << "=> CommitResources RenderDelegate" << std::endl;
}

// RENDER PASS

HdRenderPassSharedPtr
HdOSPRayRenderDelegate::CreateRenderPass(
	HdRenderIndex *index,
	HdRprimCollection const& collection)
{
	std::cout << "Create RenderPass with Collection="
		<< collection.GetName() << std::endl;

	return HdRenderPassSharedPtr(new HdOSPRayRenderPass(index, collection));
}






///* static */
///*void
//HdEmbreeRenderDelegate::HandleRtcError(const RTCError code, const char* msg)
//{
//	// Forward RTC error messages through to hydra logging.
//	switch (code) {
//	case RTC_UNKNOWN_ERROR:
//		TF_CODING_ERROR("Embree unknown error: %s", msg);
//		break;
//	case RTC_INVALID_ARGUMENT:
//		TF_CODING_ERROR("Embree invalid argument: %s", msg);
//		break;
//	case RTC_INVALID_OPERATION:
//		TF_CODING_ERROR("Embree invalid operation: %s", msg);
//		break;
//	case RTC_OUT_OF_MEMORY:
//		TF_CODING_ERROR("Embree out of memory: %s", msg);
//		break;
//	case RTC_UNSUPPORTED_CPU:
//		TF_CODING_ERROR("Embree unsupported CPU: %s", msg);
//		break;
//	case RTC_CANCELLED:
//		TF_CODING_ERROR("Embree cancelled: %s", msg);
//		break;
//	default:
//		TF_CODING_ERROR("Embree invalid error code: %s", msg);
//		break;
//	}
//}*/
//
///*static void _RenderCallback(Renderer *renderer,
//	HdRenderThread *renderThread)
//{
//	renderer->Clear();
//	renderer->Render(renderThread);
//}*/
//
HdOSPRayRenderDelegate::HdOSPRayRenderDelegate() : HdRenderDelegate()
{
	cout << "HDOSPRAY : RD CREATE()\n";
	_Initialize();
}

HdOSPRayRenderDelegate::HdOSPRayRenderDelegate(HdRenderSettingsMap const&sm) : HdRenderDelegate(sm)
{
	cout << "HDOSPRAY : RD CREATE(settingsMap)\n";
	_Initialize();
}

void HdOSPRayRenderDelegate::HandleOSPRayStatusMsg(const char *messageText)
{
	cout << "HDOSPRAY STMSG : " << messageText << endl;
};

void HdOSPRayRenderDelegate::HandleOSPRayError(OSPError e, const char *errorDetails)
{
	cout << "HDOSPRAY ERROR : " << e << " :: " << errorDetails << endl;
	
	TF_CODING_ERROR("HDOSPRAY ERROR : %s", errorDetails);
};

void HdOSPRayRenderDelegate::_Initialize()
{
	int ac = 1;
	std::string initArgs = HdOSPRayConfig::GetInstance().initArgs;

	cout << "HDOSPRAY : INIT ARGS " << initArgs << endl;

	std::stringstream ss(initArgs);
	std::string arg;
	std::vector<std::string> args;
	while (ss >> arg)
	{
		args.push_back(arg);
	};
	ac = static_cast<int>(args.size() + 1);
	const char** av = new const char*[ac];
	av[0] = "ospray";
	for (int i = 1; i < ac; i++)
	{
		av[i] = args[i - 1].c_str();
	};
	int init_error = ospInit(&ac, av);
	if (init_error != OSP_NO_ERROR)
	{
		std::cerr << "FATAL ERROR DURING INITIALIZATION!" << std::endl;
	}
	else
	{
		_device = ospGetCurrentDevice();
		if (_device == nullptr)
		{
			std::cerr << "FATAL ERROR DURING GETTING CURRENT DEVICE!" << std::endl;
		};

		ospDeviceSetStatusFunc(_device, HandleOSPRayStatusMsg);
		ospDeviceSetErrorFunc(_device, HandleOSPRayError);

		ospDeviceCommit(_device);
	};

	if (ospGetCurrentDevice() == nullptr)
	{
		// user most likely specified bad arguments, retry without them
		ac = 1;
		ospInit(&ac, av);
	};
	delete[] av;

	//#ifdef HDOSPRAY_PLUGIN_PTEX
	//ospLoadModule("ptex");
	//#endif

	if (HdOSPRayConfig::GetInstance().usePathTracing == 1)
		_renderer = ospNewRenderer("pt");
	else
		_renderer = ospNewRenderer("sv");

	// Store top-level OSPRay objects inside a render param that can be
	// passed to prims during Sync().
	//_renderParam = std::make_shared<HdOSPRayRenderParam>(_renderer, &_sceneVersion);

	// Initialize one resource registry for all OSPRay plugins
	std::lock_guard<std::mutex> guard(_mutexResourceRegistry);

	if (_counterResourceRegistry.fetch_add(1) == 0)
	{
		_resourceRegistry.reset(new HdResourceRegistry());
	};

	// Initialize the settings and settings descriptors.
	// _settingDescriptors.resize(11);
	_settingDescriptors.push_back(
	{ "Ambient occlusion samples",
		HdOSPRayRenderSettingsTokens->ambientOcclusionSamples,
		VtValue(int(
			HdOSPRayConfig::GetInstance().ambientOcclusionSamples)) });
	_settingDescriptors.push_back(
	{ "Samples per frame", HdOSPRayRenderSettingsTokens->samplesPerFrame,
		VtValue(int(HdOSPRayConfig::GetInstance().samplesPerFrame)) });
	_settingDescriptors.push_back(
	{ "Toggle denoiser", HdOSPRayRenderSettingsTokens->useDenoiser,
		VtValue(bool(HdOSPRayConfig::GetInstance().useDenoiser)) });
	_settingDescriptors.push_back(
	{ "maxDepth", HdOSPRayRenderSettingsTokens->maxDepth,
		VtValue(int(HdOSPRayConfig::GetInstance().maxDepth)) });
	_settingDescriptors.push_back(
	{ "aoDistance", HdOSPRayRenderSettingsTokens->aoDistance,
		VtValue(float(HdOSPRayConfig::GetInstance().aoDistance)) });
	_settingDescriptors.push_back(
	{ "samplesToConvergence",
		HdOSPRayRenderSettingsTokens->samplesToConvergence,
		VtValue(int(HdOSPRayConfig::GetInstance().samplesToConvergence)) });
	_settingDescriptors.push_back(
	{ "ambientLight", HdOSPRayRenderSettingsTokens->ambientLight,
		VtValue(bool(HdOSPRayConfig::GetInstance().ambientLight)) });
	_settingDescriptors.push_back(
	{ "staticDirectionalLights", HdOSPRayRenderSettingsTokens->staticDirectionalLights,
		VtValue(bool(HdOSPRayConfig::GetInstance().staticDirectionalLights)) });
	_settingDescriptors.push_back(
	{ "eyeLight", HdOSPRayRenderSettingsTokens->eyeLight,
		VtValue(bool(HdOSPRayConfig::GetInstance().eyeLight)) });
	_settingDescriptors.push_back(
	{ "keyLight", HdOSPRayRenderSettingsTokens->keyLight,
		VtValue(bool(HdOSPRayConfig::GetInstance().keyLight)) });
	_settingDescriptors.push_back(
	{ "fillLight", HdOSPRayRenderSettingsTokens->fillLight,
		VtValue(bool(HdOSPRayConfig::GetInstance().fillLight)) });
	_settingDescriptors.push_back(
	{ "backLight", HdOSPRayRenderSettingsTokens->backLight,
		VtValue(bool(HdOSPRayConfig::GetInstance().backLight)) });
	_settingDescriptors.push_back(
	{ "tileSize", HdOSPRayRenderSettingsTokens->tileSize,
		VtValue(int(HdOSPRayConfig::GetInstance().tileSize)) });
	_PopulateDefaultSettings(_settingDescriptors);

	_world = ospNewWorld();
	ospCommit(_world);


	////////////////////////////////////////////////////////////////////


	// Embree has an internal cache for subdivision surface computations.
	// HdEmbree exposes the size as an environment variable.
	//unsigned int subdivisionCache =
	//	HdEmbreeConfig::GetInstance().subdivisionCache;
	//rtcDeviceSetParameter1i(_rtcDevice, RTC_SOFTWARE_CACHE_SIZE,
	//	subdivisionCache); - DO WE NEED ANALOGOUS SETUP FOR OSPRAY?

	// Create the top-level scene.
	//
	// RTC_SCENE_DYNAMIC indicates we'll be updating the scene between draw
	// calls. RTC_INTERSECT1 indicates we'll be casting single rays, and
	// RTC_INTERPOLATE indicates we'll be storing primvars in embree objects
	// and querying them with rtcInterpolate.
	//
	// XXX: Investigate ray packets.
	//_rtcScene = rtcDeviceNewScene(_rtcDevice, RTC_SCENE_DYNAMIC,
	//	RTC_INTERSECT1 | RTC_INTERPOLATE);

	// Store top-level embree objects inside a render param that can be
	// passed to prims during Sync(). Also pass a handle to the render thread.
	_renderParam = std::make_shared<HdOSPRayRenderParam>(
		_renderer, _world, &_sceneVersion);

	// Pass the scene handle to the renderer.
	//_renderer.SetScene(_rtcScene);

	// Set the background render thread's rendering entrypoint to
	// Renderer::Render.
	//_renderThread.SetRenderCallback(
	//	std::bind(HandleOSPRayError, &_renderer, &_renderThread));
	// Start the background render thread.
	_renderThread.StartThread();

	// Initialize one resource registry for all embree plugins
	//std::lock_guard<std::mutex> guard(_mutexResourceRegistry);

	//if (_counterResourceRegistry.fetch_add(1) == 0) 
	//{
	//	_resourceRegistry = std::make_shared<HdResourceRegistry>();
	//}
}

//HdEmbreeRenderDelegate::~HdEmbreeRenderDelegate()
//{
//	// Clean the resource registry only when it is the last Embree delegate
//	{
//		std::lock_guard<std::mutex> guard(_mutexResourceRegistry);
//		if (_counterResourceRegistry.fetch_sub(1) == 1) {
//			_resourceRegistry.reset();
//		}
//	}
//
//	_renderThread.StopThread();
//
//	// Destroy embree library and scene state.
//	_renderParam.reset();
//	rtcDeleteScene(_rtcScene);
//	rtcDeleteDevice(_rtcDevice);
//}
//
