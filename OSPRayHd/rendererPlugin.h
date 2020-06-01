//
// Copyright 2018 Intel
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef HDOSPRAY_RENDERER_PLUGIN_H
#define HDOSPRAY_RENDERER_PLUGIN_H

#include <pxr/pxr.h>
#include <pxr/imaging/hd/rendererPlugin.h>
//
//using namespace pxr;

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HdOSPRayRendererPlugin
///
/// A registered child of HdxRendererPlugin, this is the class that gets
/// loaded when a hydra application asks to draw with a certain renderer.
/// It supports rendering via creation/destruction of renderer-specific
/// classes. The render delegate is the hydra-facing entrypoint into the
/// renderer; it's responsible for creating specialized implementations of hydra
/// prims (which translate scene data into drawable representations) and hydra
/// renderpasses (which draw the scene to the framebuffer).
///
class HdOSPRayRendererPlugin final : public HdRendererPlugin {
public:
	HdOSPRayRendererPlugin();
    virtual ~HdOSPRayRendererPlugin() = default;

    /// Construct a new render delegate of type HdOSPRayRenderDelegate.
    /// OSPRay render delegates own the OSPRay scene object, so a new render
    /// delegate should be created for each instance of HdRenderIndex.
    ///   \return A new HdOSPRayRenderDelegate object.
	//__declspec(dllexport)
    virtual HdRenderDelegate* CreateRenderDelegate() override;

    /// Destroy a render delegate created by this class's CreateRenderDelegate.
    ///   \param renderDelegate The render delegate to delete.
	//__declspec(dllexport)
    virtual void
    DeleteRenderDelegate(HdRenderDelegate* renderDelegate) override;

    /// Checks to see if the OSPRay plugin is supported on the running system
    ///
	//__declspec(dllexport)
    virtual bool IsSupported() const override;

private:
    // This class does not support copying.
    HdOSPRayRendererPlugin(const HdOSPRayRendererPlugin&) = delete;
    HdOSPRayRendererPlugin& operator=(const HdOSPRayRendererPlugin&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDOSPRAY_RENDERER_PLUGIN_H
