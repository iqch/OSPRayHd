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

// CRT 

// PXR

#include <pxr/imaging/glf/glew.h>

#include <pxr/base/gf/vec3f.h>
#include <pxr/imaging/hd/material.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/usd/sdf/assetPath.h>


#include <pxr/imaging/hio/glslfx.h>

#include <pxr/imaging/hdSt/resourceRegistry.h>
#include <pxr/imaging/hdSt/shaderCode.h>
#include <pxr/imaging/hdSt/surfaceShader.h>
#include <pxr/imaging/hdSt/material.h>

#include <pxr/imaging/hd/sceneDelegate.h>

using namespace pxr;

// OIIO
#include <OpenImageIO/imageio.h>

OIIO_NAMESPACE_USING;

// OSPRAY

#include <ospray\ospray.h>
#include <ospray\ospray_util.h>

// OSPRAYHD

#include "config.h"

#include "material.h"
#include "context.h"




//PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    HdOSPRayMaterialTokens,
    (UsdPreviewSurface)
    (HwPtexTexture_1)
);

TF_DEFINE_PRIVATE_TOKENS(
    HdOSPRayTokens,
    (UsdPreviewSurface)
    (diffuseColor)
    (specularColor)
    (emissiveColor)
    (metallic)
    (roughness)
    (clearcoat)
    (clearcoatRoughness)
    (ior)
    (color)
    (opacity)
    (UsdUVTexture)
    (normal)
    (displacement)
    (file)
    (filename)
    (scale)
    (wrapS)
    (wrapT)
    (repeat)
    (mirror)
    (HwPtexTexture_1)
);
// clang-format on

OSPTextureFormat
osprayTextureFormat(int depth, int channels, bool preferLinear = false)
{
    if (depth == 1) {
        if (channels == 1)
            return preferLinear ? OSP_TEXTURE_R8 : OSP_TEXTURE_L8;
        if (channels == 2)
            return preferLinear ? OSP_TEXTURE_RA8 : OSP_TEXTURE_LA8;
        if (channels == 3)
            return preferLinear ? OSP_TEXTURE_RGB8 : OSP_TEXTURE_SRGB;
        if (channels == 4)
            return preferLinear ? OSP_TEXTURE_RGBA8 : OSP_TEXTURE_SRGBA;
    } else if (depth == 4) {
        if (channels == 1)
            return OSP_TEXTURE_R32F;
        if (channels == 3)
            return OSP_TEXTURE_RGB32F;
        if (channels == 4)
            return OSP_TEXTURE_RGBA32F;
    }

    return OSP_TEXTURE_FORMAT_INVALID;
}

/// creates ptex texture and sets to file, does not commit
OSPTexture
LoadPtexTexture(std::string file)
{
    if (file == "")
        return nullptr;
    OSPTexture ospTexture = ospNewTexture("ptex");
    ospSetString(ospTexture, "filename", file.c_str());
    return ospTexture;
}
struct vec2i { int x, y;  };

// creates 2d osptexture from file, does not commit
OSPTexture
LoadOIIOTexture2D(std::string file, bool nearestFilter = false)
{
    ImageInput* in = ImageInput::open(file.c_str());
    if (!in) {
        std::cerr << "#osp: failed to load texture '" + file + "'" << std::endl;
        return nullptr;
    }

    const ImageSpec& spec = in->spec();
	//osp::vec2i size;
	vec2i size; // BY JOE
    size.x = spec.width;
    size.y = spec.height;
    int channels = spec.nchannels;
    const bool hdr = spec.format.size() > 1;
    int depth = hdr ? 4 : 1;
    const size_t stride = size.x * channels * depth;
    unsigned char* data
           = (unsigned char*)malloc(sizeof(unsigned char) * size.y * stride);

    in->read_image(hdr ? TypeDesc::FLOAT : TypeDesc::UINT8, data);
    in->close();
    ImageInput::destroy(in);

    // flip image (because OSPRay's textures have the origin at the lower left
    // corner)
    for (int y = 0; y < size.y / 2; y++) {
        unsigned char* src = &data[y * stride];
        unsigned char* dest = &data[(size.y - 1 - y) * stride];
        for (size_t x = 0; x < stride; x++)
            std::swap(src[x], dest[x]);
    }

	//OSPData ospData = ospNewData(stride * size.y, OSP_UCHAR, data);
	OSPData ospData = ospNewSharedData(data,OSP_TEXTURE, stride * size.y, OSP_UCHAR);
    ospCommit(ospData);

    OSPTexture ospTexture = ospNewTexture("texture2d");
	//ospSet1i(ospTexture, "type", (int)osprayTextureFormat(depth, channels));
	ospSetInt(ospTexture, "type", (int)osprayTextureFormat(depth, channels));
	//ospSet1i(ospTexture, "flags", nearestFilter ? OSP_TEXTURE_FILTER_NEAREST : 0);
	ospSetInt(ospTexture, "flags",nearestFilter ? OSP_TEXTURE_FILTER_NEAREST : 0);
	//ospSet2i(ospTexture, "size", size.x, size.y);
	ospSetVec2i(ospTexture, "size", size.x, size.y);

	ospSetObjectAsData(ospTexture, "data", OSP_TEXTURE, ospData);
	//ospSetObjectAsData(ospTexture, "data", ospData. ospData);

    return ospTexture;
}

HdOSPRayMaterial::HdOSPRayMaterial(SdfPath const& id)
    : HdMaterial(id)
{
    diffuseColor = GfVec3f(1, 1, 1);
}

HdOSPRayMaterial::~HdOSPRayMaterial()
{
    if (_ospMaterial)
        ospRelease(_ospMaterial);
}

/// Synchronizes state from the delegate to this object.
void
HdOSPRayMaterial::Sync(HdSceneDelegate* sceneDelegate,
                       HdRenderParam* renderParam, HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_UNUSED(renderParam);

    if (*dirtyBits & HdMaterial::DirtyResource) {
        // update material
        VtValue networkMapResource
               = sceneDelegate->GetMaterialResource(GetId());
        HdMaterialNetworkMap networkMap
               = networkMapResource.Get<HdMaterialNetworkMap>();
        HdMaterialNetwork matNetwork;

        if (networkMap.map.empty())
            std::cout << "material network map was empty!!!!!\n";

        // get material network from network map
        TF_FOR_ALL (itr, networkMap.map) {
            auto& network = itr->second;
            TF_FOR_ALL (node, network.nodes) {
                if (node->identifier
                    == HdOSPRayMaterialTokens->UsdPreviewSurface)
                    matNetwork = network;
            }
        }

        TF_FOR_ALL (node, matNetwork.nodes) {
            if (node->identifier == HdOSPRayTokens->UsdPreviewSurface)
                _ProcessUsdPreviewSurfaceNode(*node);
            else if (node->identifier == HdOSPRayTokens->UsdUVTexture
                     || node->identifier == HdOSPRayTokens->HwPtexTexture_1) {

                // find texture inputs and outputs
                auto relationships = matNetwork.relationships;
                auto relationship = std::find_if(
                       relationships.begin(), relationships.end(),
                       [&node](HdMaterialRelationship const& rel) {
                           return rel.inputId == node->path;
                       });
                if (relationship == relationships.end()) {
                    continue; // node isn't actually used
                }

                TfToken texNameToken = relationship->outputName;
                _ProcessTextureNode(*node, texNameToken);
            }
        }

        _UpdateOSPRayMaterial();

        *dirtyBits = Clean;
    }
}

void
HdOSPRayMaterial::_UpdateOSPRayMaterial()
{
    if (_ospMaterial)
        ospRelease(_ospMaterial);

    _ospMaterial = CreateDefaultMaterial(
           { diffuseColor[0], diffuseColor[1], diffuseColor[2], 1.f });

    if (map_diffuseColor.ospTexture) {
        ospSetObject(_ospMaterial, "baseColorMap", map_diffuseColor.ospTexture);
        ospSetObject(_ospMaterial, "map_Kd", map_diffuseColor.ospTexture);
    }
    if (map_metallic.ospTexture) {
        ospSetObject(_ospMaterial, "metallicMap", map_metallic.ospTexture);
        metallic = 1.0f;
    }
    if (map_roughness.ospTexture) {
        ospSetObject(_ospMaterial, "roughnessMap", map_roughness.ospTexture);
        roughness = 1.0f;
    }
    if (map_normal.ospTexture) {
        ospSetObject(_ospMaterial, "normalMap", map_normal.ospTexture);
        normal = 1.f;
    }
	//ospSet1f(_ospMaterial, "ior", ior);
	ospSetFloat(_ospMaterial, "ior", ior);
	//ospSet3fv(_ospMaterial, "baseColor", diffuseColor.data());
	float* cd = diffuseColor.data();
	ospSetVec3f(_ospMaterial, "baseColor", cd[0],cd[1],cd[2]);
	//ospSet1f(_ospMaterial, "metallic", metallic);
	ospSetFloat(_ospMaterial, "metallic", metallic);
	//ospSet1f(_ospMaterial, "roughness", roughness);
	ospSetFloat(_ospMaterial, "roughness", roughness);
	//ospSet1f(_ospMaterial, "normal", normal);
	ospSetFloat(_ospMaterial, "normal", normal);

    ospCommit(_ospMaterial);
}

void
HdOSPRayMaterial::_ProcessUsdPreviewSurfaceNode(HdMaterialNode node)
{
    TF_FOR_ALL (param, node.parameters) {
        const auto& name = param->first;
        const auto& value = param->second;
        if (name == HdOSPRayTokens->diffuseColor) {
            diffuseColor = value.Get<GfVec3f>();
        } else if (name == HdOSPRayTokens->metallic) {
            metallic = value.Get<float>();
        } else if (name == HdOSPRayTokens->roughness) {
            roughness = value.Get<float>();
        } else if (name == HdOSPRayTokens->ior) {
            ior = value.Get<float>();
        } else if (name == HdOSPRayTokens->color) {
            diffuseColor = value.Get<GfVec3f>();
        } else if (name == HdOSPRayTokens->opacity) {
            opacity = value.Get<float>();
        }
    }
}

void
HdOSPRayMaterial::_ProcessTextureNode(HdMaterialNode node, TfToken textureName)
{
    bool isPtex = node.identifier == HdOSPRayTokens->HwPtexTexture_1;

    HdOSPRayTexture texture;
    TF_FOR_ALL (param, node.parameters) {
        const auto& name = param->first;
        const auto& value = param->second;
        if (name == HdOSPRayTokens->file) {
            SdfAssetPath const& path = value.Get<SdfAssetPath>();
            texture.file = path.GetResolvedPath();
            texture.ospTexture = LoadOIIOTexture2D(texture.file);
        } else if (name == HdOSPRayTokens->filename) {
            SdfAssetPath const& path = value.Get<SdfAssetPath>();
            texture.file = path.GetResolvedPath();
            if (isPtex) {
                hasPtex = true;
                texture.isPtex = true;
#ifdef HDOSPRAY_PLUGIN_PTEX
                texture.ospTexture = LoadPtexTexture(texture.file);
#endif
            }
        } else if (name == HdOSPRayTokens->scale) {
            texture.scale = value.Get<GfVec4f>();
        } else if (name == HdOSPRayTokens->wrapS) {
        } else if (name == HdOSPRayTokens->wrapT) {
        } else {
            std::cout << "unhandled token: " << name.GetString() << " "
                      << std::endl;
        }
    }

    if (texture.ospTexture)
        ospCommit(texture.ospTexture);

    if (textureName == HdOSPRayTokens->diffuseColor) {
        map_diffuseColor = texture;
    } else if (textureName == HdOSPRayTokens->metallic) {
        map_metallic = texture;
    } else if (textureName == HdOSPRayTokens->roughness) {
        map_roughness = texture;
    } else if (textureName == HdOSPRayTokens->normal) {
        map_normal = texture;
    } else
        std::cout << "unhandled texToken: " << textureName.GetString()
                  << std::endl;
}

OSPMaterial
HdOSPRayMaterial::CreateDefaultMaterial(GfVec4f color)
{
    std::string rendererType = HdOSPRayConfig::GetInstance().usePathTracing
           ? "pathtracer"
           : "scivis";
    OSPMaterial ospMaterial;
    if (rendererType == "pathtracer") {
		//ospMaterial = ospNewMaterial2(rendererType.c_str(), "Principled");
		ospMaterial = ospNewMaterial(rendererType.c_str(), "Principled");
		//ospSet3f(ospMaterial, "baseColor", color[0], color[1], color[2]);
		ospSetVec3f(ospMaterial, "baseColor", color[0], color[1], color[2]);
    } else {
		//ospMaterial = ospNewMaterial2(rendererType.c_str(), "OBJMaterial");
		ospMaterial = ospNewMaterial(rendererType.c_str(), "OBJMaterial");
        // Carson: apparently colors are actually stored as a single color value
        // for entire object
		//ospSetf(ospMaterial, "Ns", 10.f);
		ospSetFloat(ospMaterial, "Ns", 10.f);
		//ospSet3f(ospMaterial, "Ks", 0.2f, 0.2f, 0.2f);
		ospSetVec3f(ospMaterial, "Ks", 0.2f, 0.2f, 0.2f);
		//ospSet3fv(ospMaterial, "Kd", static_cast<float*>(&color[0]));
		ospSetVec3f(ospMaterial, "Kd", color[0], color[1], color[2]);
		//ospSet1f(ospMaterial, "d", color.data()[3]);
		ospSetFloat(ospMaterial, "d", color.data()[3]);
    }
    ospCommit(ospMaterial);
    return ospMaterial;
}

//PXR_NAMESPACE_CLOSE_SCOPE
