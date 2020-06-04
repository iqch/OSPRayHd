// CRT
#include <iostream>
using namespace std;

// PXR

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/imaging/hd/meshUtil.h>
#include <pxr/imaging/hd/smoothNormals.h>

// OPENSUBD ?
#include <pxr/imaging/pxOsd/tokens.h>
#include <pxr/imaging/pxOsd/refinerFactory.h>
#include <pxr/imaging/pxOsd/meshTopology.h>
#include <pxr/imaging/pxOsd/subdivTags.h>

// OSPRAY

#include <ospray/ospray.h>

#include <ospcommon/math/vec.h>
#include <ospcommon/math/AffineSpace.h>

// HDOSPRAY

#include "renderParam.h"
#include "subd.h"

///////////////////////////////////////////

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
	HdOSPRayTokens,
	(st)
);
// clang-format on

void HdOSPRaySubd::Sync(HdSceneDelegate* sceneDelegate,
	HdRenderParam* _renderParam, HdDirtyBits* dirtyBits,
	TfToken const& reprToken)
{
	HD_TRACE_FUNCTION();
	HF_MALLOC_TAG_FUNCTION();

	SdfPath const& id = GetId();

	_MeshReprConfig::DescArray descs = _GetReprDesc(reprToken);
	const HdMeshReprDesc& desc = descs[0];

	HdOSPRayRenderParam* renderParam = (HdOSPRayRenderParam*)_renderParam;

	// MATERIAL // ?
	if (*dirtyBits & HdChangeTracker::DirtyMaterialId)
	{
		_SetMaterialId(sceneDelegate->GetRenderIndex().GetChangeTracker(),
			sceneDelegate->GetMaterialId(id));
	};

	// Create ospray geometry objects.

	//////////////////////////////////////////////////////////////////

	// POOR MAN DEGRADATION METRCS COMPUTING
	bool deg = desc.geomStyle == HdMeshGeomStyleHullEdgeOnly;

	if (deg != renderParam->degrading)
	{
		renderParam->degrading = deg;
		//cout << "CHANGED DEGRADING TO " << deg << endl;
	};

	if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->points))
	{
		VtValue value = sceneDelegate->Get(id, HdTokens->points);
		_points = value.Get<VtVec3fArray>();
		if (_points.size() > 0) _normalsValid = false;
	};

	if (HdChangeTracker::IsDisplayStyleDirty(*dirtyBits, id))
	{
		HdDisplayStyle const displayStyle = sceneDelegate->GetDisplayStyle(id);
		_topology = HdMeshTopology(_topology, displayStyle.refineLevel);
	};

	if (HdChangeTracker::IsTransformDirty(*dirtyBits, id))
	{
		_transform = GfMatrix4f(sceneDelegate->GetTransform(id));
	};

	if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, id))
	{
		// ...SHOILD I PASS THIS TOO?
		_UpdateVisibility(sceneDelegate, dirtyBits);
	};

	if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->normals)
		|| HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->primvar)
		|| HdChangeTracker::IsPrimvarDirty(*dirtyBits, id,
			HdTokens->displayColor)
		//|| HdChangeTracker::IsPrimvarDirty(*dirtyBits, id,
		//	HdTokens->displayOpacity)
		|| HdChangeTracker::IsPrimvarDirty(*dirtyBits, id,
			HdOSPRayTokens->st))
	{
		_UpdatePrimvarSources(sceneDelegate, *dirtyBits);
	};

	if (HdChangeTracker::IsDisplayStyleDirty(*dirtyBits, id))
	{
		_refineLevel = _topology.GetRefineLevel();
	};

	if (HdChangeTracker::IsSubdivTagsDirty(*dirtyBits, id) && _topology.GetRefineLevel() > 0)
	{
		_topology.SetSubdivTags(sceneDelegate->GetSubdivTags(id));
	};

	if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id))
	{
		// ...
	}


	/////////////////////////////////////////////////////////////////

	//if (*dirtyBits & HdChangeTracker::DirtyTopology)
	//{
	//	// TODO: update material here?
	//};

};


// CTOR/DTOR/INIT MACHINERY

HdOSPRaySubd::HdOSPRaySubd(SdfPath const& id, SdfPath const& instancerId) : HdMesh(id, instancerId) {};

void
HdOSPRaySubd::Finalize(HdRenderParam* renderParam)
{
	if (_instance) ospRelease(_instance);
	if (_group) ospRelease(_group);
	//if (_model) ospRelease(_model);
	//if (_mesh) ospRelease(_mesh);
};

HdDirtyBits
HdOSPRaySubd::GetInitialDirtyBitsMask() const
{
	int mask = HdChangeTracker::Clean | HdChangeTracker::InitRepr
		| HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyTopology
		| HdChangeTracker::DirtyTransform | HdChangeTracker::DirtyVisibility
		| HdChangeTracker::DirtyDisplayStyle
		| HdChangeTracker::DirtySubdivTags | HdChangeTracker::DirtyPrimvar
		| HdChangeTracker::DirtyNormals | HdChangeTracker::DirtyInstanceIndex;
		//| HdChangeTracker::AllDirty;

	return (HdDirtyBits)mask;
};

void
HdOSPRaySubd::_InitRepr(TfToken const& reprToken, HdDirtyBits* dirtyBits)
{
	TF_UNUSED(dirtyBits);
	_ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(), _ReprComparator(reprToken));

	if (it == _reprs.end())
	{
		_reprs.emplace_back(reprToken, HdReprSharedPtr());
	};
};

HdDirtyBits HdOSPRaySubd::_PropagateDirtyBits(HdDirtyBits bits) const { return bits; }; // PURE VIRTUAL AND USELESS

PXR_NAMESPACE_CLOSE_SCOPE
