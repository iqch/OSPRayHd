#pragma once

// CRT
#include <mutex>
#include <vector>

using namespace std;

// PXR
#include <pxr/pxr.h>

#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/imaging/hd/enums.h>
#include <pxr/imaging/hd/mesh.h>
#include <pxr/imaging/hd/vertexAdjacency.h>

// OSPRAY
#include <ospray/ospray.h>

// HDOSPRAY
#include "renderParam.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdOSPRaySubd final : public HdMesh {
public:
	HF_MALLOC_TAG_NEW("new HdOSPRaySubd");

	HdOSPRaySubd(SdfPath const& id, SdfPath const& instancerId = SdfPath());
	virtual ~HdOSPRaySubd() = default;

	virtual HdDirtyBits GetInitialDirtyBitsMask() const override;

	virtual void Finalize(HdRenderParam* renderParam) override;


	virtual void Sync(HdSceneDelegate* sceneDelegate,
		HdRenderParam* renderParam, HdDirtyBits* dirtyBits,
		TfToken const& reprToken) override;

	virtual HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override; 
	virtual void _InitRepr(TfToken const& reprToken, HdDirtyBits* dirtyBits) override;


protected:

	vector<OSPGeometry> _mesh;
	vector<OSPGeometricModel> _model;
	OSPGroup _group { NULL };
	OSPInstance  _instance { NULL };

	// Cached scene data. VtArrays are reference counted, so as long as we
	// only call const accessors keeping them around doesn't incur a buffer
	// copy.
	int _refineLevel;
	HdMeshTopology _topology;
	GfMatrix4f _transform;
	VtVec3fArray _points;
	VtVec2fArray _texcoords;
	VtVec3fArray _normals;
	VtVec4fArray _colors;
	//VtVec4fArray _opacites;
	// materials

	// Derived scene data:
	VtVec3fArray _computedNormals;

	Hd_VertexAdjacency _adjacency;
	bool _adjacencyValid;
	bool _normalsValid;

	// Draw styles.
	int _tessellationRate{ 32 };

	// A local cache of primvar scene data. "data" is a copy-on-write handle to
	// the actual primvar buffer, and "interpolation" is the interpolation mode
	// to be used. This cache is used in _PopulateRtMesh to populate the
	// primvar sampler map in the prototype context, which is used for shading.
	struct PrimvarSource {
		VtValue data;
		HdInterpolation interpolation;
	};
	TfHashMap<TfToken, PrimvarSource, TfToken::HashFunctor> _primvarSourceMap;

	// This class does not support copying.
	HdOSPRaySubd(const HdOSPRaySubd&) = delete;
	HdOSPRaySubd& operator=(const HdOSPRaySubd&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE


