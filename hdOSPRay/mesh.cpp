// CRT
#include <iostream>

// PXR

#include <pxr/pxr.h>


PXR_NAMESPACE_USING_DIRECTIVE


// HDOSPRAY

#include "mesh.h"



HdOSPRayMesh::HdOSPRayMesh(SdfPath const& id, SdfPath const& instancerId)
	: HdMesh(id, instancerId)
{
}

HdDirtyBits
HdOSPRayMesh::GetInitialDirtyBitsMask() const
{
	return HdChangeTracker::Clean
		| HdChangeTracker::DirtyTransform;
}

HdDirtyBits
HdOSPRayMesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
	return bits;
}

void
HdOSPRayMesh::_InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits)
{

}

void
HdOSPRayMesh::Sync(HdSceneDelegate *sceneDelegate,
	HdRenderParam   *renderParam,
	HdDirtyBits     *dirtyBits,
	TfToken const   &reprToken)
{
	std::cout << "* (multithreaded) Sync Tiny Mesh id=" << GetId() << std::endl;
}

