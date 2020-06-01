#pragma once

#include <pxr/pxr.h>
#include <pxr/imaging/hd/renderPass.h>

PXR_NAMESPACE_USING_DIRECTIVE



/// \class HdTinyRenderPass
///
/// HdRenderPass represents a single render iteration, rendering a view of the
/// scene (the HdRprimCollection) for a specific viewer (the camera/viewport
/// parameters in HdRenderPassState) to the current draw target.
///
class HdOSPRayRenderPass final : public HdRenderPass
{
public:
	/// Renderpass constructor.
	///   \param index The render index containing scene data to render.
	///   \param collection The initial rprim collection for this renderpass.
	HdOSPRayRenderPass(HdRenderIndex *index, HdRprimCollection const &collection);

	/// Renderpass destructor.
	virtual ~HdOSPRayRenderPass();

protected:

	/// Draw the scene with the bound renderpass state.
	///   \param renderPassState Input parameters (including viewer parameters)
	///                          for this renderpass.
	///   \param renderTags Which rendertags should be drawn this pass.
	void _Execute(
		HdRenderPassStateSharedPtr const& renderPassState,
		TfTokenVector const &renderTags) override;
};
