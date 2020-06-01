#pragma once

// CRT
#include <mutex>
using namespace std;

// PXR
#include <pxr/pxr.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/renderThread.h>

#include <pxr/base/tf/staticTokens.h>

PXR_NAMESPACE_USING_DIRECTIVE


// OSPRAY
#include <ospray/ospray.h>

// HDOSPRAY
#include "param.h"
//#include "renderer.h"


class HdOSPRayRenderDelegate final : public HdRenderDelegate 
{
public:
	HdOSPRayRenderDelegate();
	HdOSPRayRenderDelegate(HdRenderSettingsMap const& settingsMap);
	virtual ~HdOSPRayRenderDelegate();

	virtual HdRenderParam *GetRenderParam() const override;


	virtual const TfTokenVector &GetSupportedRprimTypes() const override;
	virtual const TfTokenVector &GetSupportedSprimTypes() const override;
	virtual const TfTokenVector &GetSupportedBprimTypes() const override;

	virtual HdResourceRegistrySharedPtr GetResourceRegistry() const override;

	/// Returns a list of user-configurable render settings.
	/// This is a reflection API for the render settings dictionary; it need
	/// not be exhaustive, but can be used for populating application settings
	/// UI.
	virtual HdRenderSettingDescriptorList
		GetRenderSettingDescriptors() const override;


	//virtual bool IsPauseSupported() const override;
	//virtual bool Pause() override;
	//virtual bool Resume() override;

	virtual HdRenderPassSharedPtr CreateRenderPass(HdRenderIndex *index,
		HdRprimCollection const& collection) override;

	virtual HdInstancer *CreateInstancer(HdSceneDelegate *delegate,
		SdfPath const& id,
		SdfPath const& instancerId);

	virtual void DestroyInstancer(HdInstancer *instancer);

	virtual HdRprim *CreateRprim(TfToken const& typeId,
		SdfPath const& rprimId,
		SdfPath const& instancerId) override;
	virtual void DestroyRprim(HdRprim *rPrim) override;

	virtual HdSprim *CreateSprim(TfToken const& typeId,
		SdfPath const& sprimId) override;
	virtual HdSprim *CreateFallbackSprim(TfToken const& typeId) override;
	virtual void DestroySprim(HdSprim *sPrim) override;

	virtual HdBprim *CreateBprim(TfToken const& typeId,
		SdfPath const& bprimId) override;
	virtual HdBprim *CreateFallbackBprim(TfToken const& typeId) override;
	virtual void DestroyBprim(HdBprim *bPrim) override;

	virtual void CommitResources(HdChangeTracker *tracker) override;

	//virtual TfToken GetMaterialBindingPurpose() const override { return HdTokens->full; }

	//virtual HdAovDescriptor
	//	GetDefaultAovDescriptor(TfToken const& name) const override;

	//virtual VtDictionary GetRenderStats() const override;

private:
	static const TfTokenVector SUPPORTED_RPRIM_TYPES;
	static const TfTokenVector SUPPORTED_SPRIM_TYPES;
	static const TfTokenVector SUPPORTED_BPRIM_TYPES;

	/// Resource registry used in this render delegate
	static std::mutex _mutexResourceRegistry;
	static std::atomic_int _counterResourceRegistry;
	static HdResourceRegistrySharedPtr _resourceRegistry;

	// This class does not support copying.
	HdOSPRayRenderDelegate(const HdOSPRayRenderDelegate &) = delete;
	HdOSPRayRenderDelegate &operator =(const HdOSPRayRenderDelegate &) = delete;

	// initialization routine. - DO WE NEED IT?
	void _Initialize();

	// Handle for an embree "device", or library state.
	//RTCDevice _rtcDevice;
	OSPDevice _device; // ? REALLY WE NEED IT

	// Handle for the top-level embree scene, mirroring the Hydra scene.
	//RTCScene _rtcScene;
	OSPWorld _world;
	OSPRenderer _renderer;


	// A version counter for edits to _scene.
	std::atomic<int> _sceneVersion;

	// A shared HdEmbreeRenderParam object that stores top-level embree state;
	// passed to prims during Sync().
	std::shared_ptr<HdOSPRayRenderParam> _renderParam;

	// A background render thread for running the actual renders in. The
	// render thread object manages synchronization between the scene data
	// and the background-threaded renderer.
	HdRenderThread _renderThread;

	// An ospray renderer object, to perform the actual raytracing.
	//Renderer _renderer;

	// A list of render setting exports.
	HdRenderSettingDescriptorList _settingDescriptors;

	// A callback that interprets ospray error codes and injects them into
	// the hydra logging system.
	static void HandleOSPRayStatusMsg(const char *messageText);
	static void HandleOSPRayError(OSPError, const char *errorDetails);
};