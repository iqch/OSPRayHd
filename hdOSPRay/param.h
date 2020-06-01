#pragma once

// PXR
#include <pxr/pxr.h>
#include <pxr/imaging/hd/renderDelegate.h>

PXR_NAMESPACE_USING_DIRECTIVE

// OSPRAY
#include "ospray/ospray.h"

class HdOSPRayRenderParam final : public HdRenderParam
{
public:
	HdOSPRayRenderParam(OSPRenderer renderer, OSPWorld world, std::atomic<int>* sceneVersion)
		: _renderer(renderer)
		, _world(world)
		, _sceneVersion(sceneVersion)
	{};
	virtual ~HdOSPRayRenderParam() = default;

	OSPRenderer GetOSPRayRenderer() { return _renderer; };

	void UpdateModelVersion() { _modelVersion++; };

	int GetModelVersion() { return _modelVersion.load(); };

	// thread safe.  Instances added to scene and released by renderPass.
	void AddInstance(const OSPInstance instance)
	{
		std::lock_guard<std::mutex> lock(_ospMutex);
		_ospInstances.emplace_back(instance);
	};

	// thread safe.  Instances added to scene and released by renderPass.
	void AddInstances(const std::vector<OSPInstance>& instances)
	{
		std::lock_guard<std::mutex> lock(_ospMutex);
		_ospInstances.insert(_ospInstances.end(),
			instances.begin(), instances.end());
	};

	void ClearInstances() { _ospInstances.resize(0); };

	// not thread safe
	std::vector<OSPInstance>& GetInstances() { return _ospInstances; };

private:
	// mutex over ospray calls to the global model and global instances. OSPRay
	// is not thread safe
	std::mutex _ospMutex;
	// meshes populate global instances.  These are then committed by the
	// renderPass into a scene.
	std::vector<OSPInstance> _ospInstances;
	OSPWorld _world;
	OSPRenderer _renderer;
	/// A version counter for edits to _scene.
	std::atomic<int>* _sceneVersion;
	// version of osp model.  Used for tracking image changes
	std::atomic<int> _modelVersion{ 1 };
};


