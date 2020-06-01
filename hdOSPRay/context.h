#pragma once

// PXR
#include "pxr/pxr.h"
#include "pxr/base/gf/matrix4f.h"

#include "pxr/imaging/hd/rprim.h"

PXR_NAMESPACE_USING_DIRECTIVE


// OSPRAYHD
#include "sampler.h"



/// \class HdOSPRayPrototypeContext
///
/// A small bit of state attached to each bit of prototype geometry in OSPRay,
/// for the benefit of HdOSPRayRenderPass::_TraceRay.
///
struct HdOSPRayPrototypeContext {
	/// A pointer back to the owning HdOSPRay rprim.
	HdRprim* rprim;
	/// A name-indexed map of primvar samplers.
	TfHashMap<TfToken, HdOSPRayPrimvarSampler*, TfToken::HashFunctor>
		primvarMap;
	/// A copy of the primitive params for this rprim.
	VtIntArray primitiveParams;
};

///
/// \class HdOSPRayInstanceContext
///
/// A small bit of state attached to each bit of instanced geometry in OSPRay
///
struct HdOSPRayInstanceContext {
	/// The object-to-world transform, for transforming normals to worldspace.
	GfMatrix4f objectToWorldMatrix;
	/// The scene the prototype geometry lives in, for passing to
};
