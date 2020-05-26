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

#ifndef HDOSPRAY_MATERIAL_H
#define HDOSPRAY_MATERIAL_H

// CRT

// PXR
#include <pxr/pxr.h>
#include <pxr/imaging/hd/material.h>

using namespace pxr;

// OSPRAY
#include "ospray/ospray.h"

// BOOST
//#include <boost/shared_ptr.hpp>

//PXR_NAMESPACE_OPEN_SCOPE

//typedef boost::shared_ptr<class HdStTextureResource>
//      HdStTextureResourceSharedPtr;

/// OSPRay hdMaterial
///  supports uvtextures and ptex when pxr_oiio_plugin
///  and pxr_ptex_plugin are enabled.
///  The OSPRay representation can be accessed via
///  GetOSPRayMaterial().
class HdOSPRayMaterial final : public HdMaterial {
public:
    HdOSPRayMaterial(SdfPath const& id);

    virtual ~HdOSPRayMaterial();

    /// Synchronizes state from the delegate to this object.
    virtual void Sync(HdSceneDelegate* sceneDelegate,
                      HdRenderParam* renderParam,
                      HdDirtyBits* dirtyBits) override;

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    virtual HdDirtyBits GetInitialDirtyBitsMask() const override
    {
        return AllDirty;
    }

    /// Causes the shader to be reloaded.
    virtual void Reload() override
    {
    }

    /// Create a default material based on the renderer type specified in config
    static OSPMaterial CreateDefaultMaterial(GfVec4f color);

    /// Summary flag. Returns true if the material is bound to one or more
    /// textures and any of those textures is a ptex texture.
    /// If no textures are bound or all textures are uv textures, then
    /// the method returns false.
    inline bool HasPtex() const
    {
        return hasPtex;
    }

    // Return the OSPMaterial object generated by Sync
    inline const OSPMaterial GetOSPRayMaterial() const
    {
        return _ospMaterial;
    }

protected:
    // update osp representations for material
    void _UpdateOSPRayMaterial();
    // fill in material parameters based on usdPreviewSurface node
    void _ProcessUsdPreviewSurfaceNode(HdMaterialNode node);
    // parse texture node params and set them to appropriate map_ texture var
    void _ProcessTextureNode(HdMaterialNode node, TfToken textureName);

    struct HdOSPRayTexture {
        std::string file;
        enum class WrapType { NONE, BLACK, CLAMP, REPEAT, MIRROR };
        WrapType wrapS, wrapT;
        GfVec4f scale;
        enum class ColorType { NONE, RGBA, RGB, R, G, B, A };
        ColorType type;
        OSPTexture ospTexture { nullptr };
        bool isPtex { false };
    };

    GfVec3f diffuseColor { 0.5f, 0.5f, 0.5f };
    GfVec3f emissiveColor { 0.f, 0.f, 0.f };
    GfVec3f specularColor { 1.f, 1.f, 1.f };
    float metallic { 0.f };
    float roughness { 0.f };
    GfVec4f clearcoat;
    float clearcoatRoughness;
    float ior { 1.f };
    float opacity { 1.f };
    float normal { 1.f };
    bool hasPtex { false };

    HdOSPRayTexture map_diffuseColor;
    HdOSPRayTexture map_emissiveColor;
    HdOSPRayTexture map_specularColor;
    HdOSPRayTexture map_metallic;
    HdOSPRayTexture map_roughness;
    HdOSPRayTexture map_clearcoat;
    HdOSPRayTexture map_clearcoatRoughness;
    HdOSPRayTexture map_ior;
    HdOSPRayTexture map_opacity;
    HdOSPRayTexture map_normal;
    HdOSPRayTexture map_displacement;

    OSPMaterial _ospMaterial { nullptr };
};

//PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDOSPRAY_MATERIAL_H