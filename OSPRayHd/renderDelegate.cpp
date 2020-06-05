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
#include <iostream>
using namespace std;

// PXR 
#include <pxr/imaging/hd/resourceRegistry.h>

#include <pxr/imaging/hd/camera.h>
#include <pxr/imaging/hd/bprim.h>

// OIIO
#include <OpenImageIO/imageio.h>
OIIO_NAMESPACE_USING;

// OSPRAY
#include <ospray/ospray.h>

// OSPRAYHD

#include "config.h"
#include "instancer.h"
#include "renderParam.h"
#include "renderPass.h"

#include "material.h"
//#include "mesh.h"
#include "subd.h"

#include "renderDelegate.h"

// DEVICE
OSPDevice device = NULL;

PXR_NAMESPACE_OPEN_SCOPE

// STATICS
std::mutex HdOSPRayRenderDelegate::_mutexResourceRegistry;
std::atomic_int HdOSPRayRenderDelegate::_counterResourceRegistry;
HdResourceRegistrySharedPtr HdOSPRayRenderDelegate::_resourceRegistry;

// RENDER SETTINGS/PARAMS

TF_DEFINE_PUBLIC_TOKENS(HdOSPRayRenderSettingsTokens, HDOSPRAY_RENDER_SETTINGS_TOKENS);

//TF_DEFINE_PUBLIC_TOKENS(HdOSPRayTokens, HDOSPRAY_TOKENS);

void HdOSPRayRenderDelegate::_InitParams()
{
	// Initialize the settings and settings descriptors.
	_settingDescriptors.push_back(
	{ "samplesToConvergence",
		HdOSPRayRenderSettingsTokens->samplesToConvergence,
		VtValue(int(HdOSPRayConfig::GetInstance().samplesToConvergence)) });
	_settingDescriptors.push_back(
	{ "useDenoiser",
		HdOSPRayRenderSettingsTokens->useDenoiser,
		VtValue(bool(HdOSPRayConfig::GetInstance().useDenoiser)) });

	_PopulateDefaultSettings(_settingDescriptors);
};

HdRenderParam* HdOSPRayRenderDelegate::GetRenderParam() const
{
	return _renderParam.get();
};

HdRenderSettingDescriptorList
HdOSPRayRenderDelegate::GetRenderSettingDescriptors() const
{
	return _settingDescriptors;
};

// SUPPORTED TYPES

// RPRIM
const TfTokenVector HdOSPRayRenderDelegate::SUPPORTED_RPRIM_TYPES = {
    HdPrimTypeTokens->mesh,
};

TfTokenVector const&
HdOSPRayRenderDelegate::GetSupportedRprimTypes() const
{
	return SUPPORTED_RPRIM_TYPES;
}

HdRprim* HdOSPRayRenderDelegate::CreateRprim(TfToken const& typeId,
	SdfPath const& rprimId,
	SdfPath const& instancerId)
{
	if (typeId == HdPrimTypeTokens->mesh)
	{
		return new HdOSPRaySubd(rprimId, instancerId);
		//return new HdOSPRayMesh(rprimId, instancerId);
	}
	else
	{
		TF_CODING_ERROR("Unknown Rprim Type %s", typeId.GetText());
	};

	return nullptr;
}

void HdOSPRayRenderDelegate::DestroyRprim(HdRprim* rPrim)
{
	delete rPrim;
}

// INSTANCER
HdInstancer*
HdOSPRayRenderDelegate::CreateInstancer(HdSceneDelegate* delegate,
	SdfPath const& id,
	SdfPath const& instancerId)
{
	return NULL;
	//return new HdOSPRayInstancer(delegate, id, instancerId);
}

void
HdOSPRayRenderDelegate::DestroyInstancer(HdInstancer* instancer)
{
	delete instancer;
}

// SPRIM
const TfTokenVector HdOSPRayRenderDelegate::SUPPORTED_SPRIM_TYPES = {
    HdPrimTypeTokens->camera,
    HdPrimTypeTokens->material,
};

TfTokenVector const&
HdOSPRayRenderDelegate::GetSupportedSprimTypes() const
{
	return SUPPORTED_SPRIM_TYPES;
}

HdSprim*
HdOSPRayRenderDelegate::CreateSprim(TfToken const& typeId,
	SdfPath const& sprimId)
{
	if (typeId == HdPrimTypeTokens->camera)
	{
		return new HdCamera(sprimId);
	}
	else if (typeId == HdPrimTypeTokens->material)
	{
		return new HdOSPRayMaterial(sprimId);
	}
	else
	{
	TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
	};

	return nullptr;
}

HdSprim*
HdOSPRayRenderDelegate::CreateFallbackSprim(TfToken const& typeId)
{
	// For fallback sprims, create objects with an empty scene path.
	// They'll use default values and won't be updated by a scene delegate.
	return CreateSprim(typeId, SdfPath::EmptyPath());
}

void
HdOSPRayRenderDelegate::DestroySprim(HdSprim* sPrim)
{
	delete sPrim;
}

// BPRIM
const TfTokenVector HdOSPRayRenderDelegate::SUPPORTED_BPRIM_TYPES = {
	HdPrimTypeTokens->renderBuffer
	//HdPrimTypeTokens->openvdb, ? volume
};

TfTokenVector const&
HdOSPRayRenderDelegate::GetSupportedBprimTypes() const
{
	return SUPPORTED_BPRIM_TYPES;
}

HdBprim*
HdOSPRayRenderDelegate::CreateBprim(TfToken const& typeId,
	SdfPath const& bprimId)
{
	if (typeId == HdPrimTypeTokens->renderBuffer) {
		return new HdOSPRayRenderBuffer(bprimId);
	}
	else {
		TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
	}
	return nullptr;
}

HdBprim*
HdOSPRayRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
	return CreateBprim(typeId, SdfPath::EmptyPath());
}

void
HdOSPRayRenderDelegate::DestroyBprim(HdBprim* bPrim)
{
	delete bPrim;
}

// CTOR/DTOR/INIT
HdOSPRayRenderDelegate::HdOSPRayRenderDelegate() : HdRenderDelegate()
{
    _Initialize();
}

HdOSPRayRenderDelegate::HdOSPRayRenderDelegate(
       HdRenderSettingsMap const& settingsMap)
    : HdRenderDelegate(settingsMap)
{
    _Initialize();
}

void
HdOSPRayRenderDelegate::_Initialize()
{
	if (device == NULL)
	{
		int ac = 1;
		std::string initArgs = HdOSPRayConfig::GetInstance().initArgs;
		std::stringstream ss(initArgs);
		std::string arg;
		std::vector<std::string> args;
		while (ss >> arg)
		{
			args.push_back(arg);
		};

		ac = static_cast<int>(args.size() + 1);
		const char** av = new const char*[ac];
		av[0] = "ospray";
		for (int i = 1; i < ac; i++)
		{
			av[i] = args[i - 1].c_str();
		};
		int init_error = ospInit(&ac, av);
		if (init_error != OSP_NO_ERROR)
		{
			std::cerr << "FATAL ERROR DURING INITIALIZATION!" << std::endl;
			return;
		}

		device = ospGetCurrentDevice();
		if (device == nullptr) 
		{
			std::cerr << "FATAL ERROR DURING GETTING CURRENT DEVICE!" << std::endl;
			return;
		};

		ospDeviceSetStatusFunc(device, HdOSPRayRenderDelegate::HandleOSPRayStatusMsg);
		ospDeviceSetErrorFunc(device, HdOSPRayRenderDelegate::HandleOSPRayError);

		ospDeviceCommit(device);

		delete[] av;
	}

    ospLoadModule("ptex");
	ospLoadModule("denoiser");

    _renderParam = std::make_shared<HdOSPRayRenderParam>(&_sceneVersion);

	_renderParam->_material = ospNewMaterial("pathtracer", "principled");

	const char* file = "F:\\Clouds\\YandexDisk\\Projects\\OSPRayHd\\tests\\test2.jpg";
	OSPTexture texture = HdOSPRayMaterial::LoadOIIOTexture2D(file,true);
	
	ospSetVec3f(_renderParam->_material, "ks", 1.0f,1.0f,1.0f);
	ospSetFloat(_renderParam->_material, "ns", 200);

	//ospSetObject(_renderParam->_material, "map_baseColor", texture);


	//ospSetFloat(_renderParam->_material, "attenuationDistance", 0.25);
	//ospSetVec3f(_renderParam->_material, "attenuationColor", 1.0, 0.45, 0.15);
	ospCommit(_renderParam->_material);

    // Initialize one resource registry for all OSPRay plugins
    std::lock_guard<std::mutex> guard(_mutexResourceRegistry);

    if (_counterResourceRegistry.fetch_add(1) == 0)
	{
        _resourceRegistry.reset(new HdResourceRegistry());
	};

	_InitParams();
}

HdOSPRayRenderDelegate::~HdOSPRayRenderDelegate()
{
    // Clean the resource registry only when it is the last OSPRay delegate
    std::lock_guard<std::mutex> guard(_mutexResourceRegistry);

    if (_counterResourceRegistry.fetch_sub(1) == 1)
	{
        _resourceRegistry.reset();
    }

    _renderParam.reset();
};

// CALLBACKS

void HdOSPRayRenderDelegate::HandleOSPRayStatusMsg(const char *messageText)
{
	cout << "HDOSPRAY STMSG : " << messageText << endl;
};

void HdOSPRayRenderDelegate::HandleOSPRayError(OSPError e, const char *errorDetails)
{
	cout << "HDOSPRAY ERROR : " << e << " :: " << errorDetails << endl;

	TF_CODING_ERROR("HDOSPRAY ERROR : %s", errorDetails);
};



// RESOURCES PROCESSING

HdResourceRegistrySharedPtr
HdOSPRayRenderDelegate::GetResourceRegistry() const
{
	return _resourceRegistry;
}

void
HdOSPRayRenderDelegate::CommitResources(HdChangeTracker* tracker)
{
    // CommitResources() is called after prim sync has finished, but before any
    // tasks (such as draw tasks) have run.
	//std::shared_ptr<HdOSPRayRenderParam>& rp = _renderParam;
    const int modelVersion = _renderParam->GetModelVersion();
    if (modelVersion > _lastCommittedModelVersion) 
	{
        _lastCommittedModelVersion = modelVersion; // POSSIBLE CHANGE TO WORLD VERSION
    }
}

// RECENT

//TfToken
//HdOSPRayRenderDelegate::GetMaterialNetworkSelector() const
//{
//	// Carson: this should be "HdOSPRayTokens->ospray", but we return glslfx so
//	// that we work with many supplied shaders
//	// TODO: is it possible to return both?
//	return HdOSPRayTokens->glslfx;
//	//return HdOSPRayTokens->glslfx;
//}


HdAovDescriptor
HdOSPRayRenderDelegate::GetDefaultAovDescriptor(TfToken const& name) const
{
    if (name == HdAovTokens->color) 
	{
		//return HdAovDescriptor(HdFormatUNorm8Vec4, true, VtValue(GfVec4f(0.0f)));
		return HdAovDescriptor(HdFormatFloat32Vec4, true, VtValue(GfVec4f(0.0f)));
    } 
	//else if (name == HdAovTokens->normal || name == HdAovTokens->Neye) 
	//{
 //       return HdAovDescriptor(HdFormatFloat32Vec3, false, VtValue(GfVec3f(-1.0f)));
 //   } 
	//if (name == HdAovTokens->depth)
	//{
	//	return HdAovDescriptor(HdFormatFloat32, false, VtValue(1.0f));
	//}
    //} else if (name == HdAovTokens->linearDepth) {
    //    return HdAovDescriptor(HdFormatFloat32, false, VtValue(0.0f));
   // } 
	else if (name == HdAovTokens->primId ||
		name == HdAovTokens->instanceId ||
		name == HdAovTokens->elementId)
	{
        return HdAovDescriptor(HdFormatInt32, false, VtValue(-1));
    } 
	else 
	{
        HdParsedAovToken aovId(name);
        if (aovId.isPrimvar) 
		{
            return HdAovDescriptor(HdFormatFloat32Vec3, false, VtValue(GfVec3f(0.0f)));
		};
	};

    return HdAovDescriptor();
};

HdRenderPassSharedPtr
HdOSPRayRenderDelegate::CreateRenderPass(HdRenderIndex* index,
                                         HdRprimCollection const& collection)
{
    return HdRenderPassSharedPtr(
		new HdOSPRayRenderPass(index, collection, &_sceneVersion, _renderParam));
};


//VtDictionary
//HdEmbreeRenderDelegate::GetRenderStats() const
//{
//	VtDictionary stats;
//	stats[HdPerfTokens->numCompletedSamples.GetString()] =
//		_renderer.GetCompletedSamples();
//	return stats;
//}


PXR_NAMESPACE_CLOSE_SCOPE
