// CRT
#include <iostream>

using namespace std;


// PXR
#include <pxr/pxr.h>

PXR_NAMESPACE_USING_DIRECTIVE

// HDOSPRAY
#include "pass.h"




HdOSPRayRenderPass::HdOSPRayRenderPass(
	HdRenderIndex *index,
	HdRprimCollection const &collection)
	: HdRenderPass(index, collection)
{
}

HdOSPRayRenderPass::~HdOSPRayRenderPass()
{
	cout << "Destroying renderPass" << std::endl;
}

void
HdOSPRayRenderPass::_Execute(
	HdRenderPassStateSharedPtr const& renderPassState,
	TfTokenVector const &renderTags)
{
	cout << "=> Execute RenderPass" << std::endl;
}