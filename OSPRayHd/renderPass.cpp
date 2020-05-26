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

// PXR
#include <pxr/imaging/glf/glew.h>

#include <pxr/imaging/hd/perfLog.h>
#include <pxr/imaging/hd/renderPassState.h>

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/work/loops.h>

// OSPRAYHD

#include "renderParam.h"
#include "renderPass.h"

#include "config.h"
#include "context.h"
#include "mesh.h"
#include "renderDelegate.h"


//PXR_NAMESPACE_OPEN_SCOPE

inline float
rad(float deg)
{
    return deg * M_PI / 180.0f;
}

HdOSPRayRenderPass::HdOSPRayRenderPass(
       HdRenderIndex* index, HdRprimCollection const& collection,
       OSPRenderer renderer, std::atomic<int>* sceneVersion,
       std::shared_ptr<HdOSPRayRenderParam> renderParam)
    : HdRenderPass(index, collection)
    , _pendingResetImage(false)
    , _pendingModelUpdate(true)
    , _renderer(renderer)
    , _sceneVersion(sceneVersion)
    , _lastRenderedVersion(0)
    , _lastRenderedModelVersion(0)
    , _width(0)
    , _height(0)
    , _inverseViewMatrix(1.0f) // == identity
    , _inverseProjMatrix(1.0f) // == identity
    , _clearColor(0.0707f, 0.0707f, 0.0707f)
    , _renderParam(renderParam)
{
    _camera = ospNewCamera("perspective");
    ospSetVec4f(_renderer, "bgColor", _clearColor[0], _clearColor[1],
             _clearColor[2], 1.f);

    ospSetObject(_renderer, "camera", _camera);

    ospSetInt(_renderer, "maxDepth", 8);
    ospSetInt(_renderer, "aoDistance", 15.0f);
    ospSetInt(_renderer, "shadowsEnabled", true);
    ospSetInt(_renderer, "maxContribution", 2.f);
    ospSetInt(_renderer, "minContribution", 0.05f);
    ospSetInt(_renderer, "epsilon", 0.001f);
    ospSetInt(_renderer, "useGeometryLights", 0);

    ospCommit(_renderer);

//#if HDOSPRAY_ENABLE_DENOISER
    _denoiserDevice = oidn::newDevice();
    _denoiserDevice.commit();
    _denoiserFilter = _denoiserDevice.newFilter("RT");
//#endif
}

HdOSPRayRenderPass::~HdOSPRayRenderPass()
{
}

void
HdOSPRayRenderPass::ResetImage()
{
    // Set a flag to clear the sample buffer the next time Execute() is called.
    _pendingResetImage = true;
}

bool
HdOSPRayRenderPass::IsConverged() const
{
    // A super simple heuristic: consider ourselves converged after N
    // samples. Since we currently uniformly sample the framebuffer, we can
    // use the sample count from pixel(0,0).
    unsigned int samplesToConvergence
           = HdOSPRayConfig::GetInstance().samplesToConvergence;
    return ((unsigned int)_numSamplesAccumulated >= samplesToConvergence);
}

void
HdOSPRayRenderPass::_MarkCollectionDirty()
{
    // If the drawable collection changes, we should reset the sample buffer.
    _pendingResetImage = true;
    _pendingModelUpdate = true;
}

void
HdOSPRayRenderPass::_Execute(HdRenderPassStateSharedPtr const& renderPassState,
                             TfTokenVector const& renderTags)
{
    HdRenderDelegate* renderDelegate = GetRenderIndex()->GetRenderDelegate();

    // Update camera
    auto inverseViewMatrix
           = renderPassState->GetWorldToViewMatrix().GetInverse();
    auto inverseProjMatrix
           = renderPassState->GetProjectionMatrix().GetInverse();

    if (inverseViewMatrix != _inverseViewMatrix
        || inverseProjMatrix != _inverseProjMatrix) 
	{
        ResetImage();
        _inverseViewMatrix = inverseViewMatrix;
        _inverseProjMatrix = inverseProjMatrix;
    }

    float aspect = _width / float(_height);
    ospSetFloat(_camera, "aspect", aspect);
    GfVec3f origin = GfVec3f(0, 0, 0);
    GfVec3f dir = GfVec3f(0, 0, -1);
    GfVec3f up = GfVec3f(0, 1, 0);
    dir = _inverseProjMatrix.Transform(dir);
    origin = _inverseViewMatrix.Transform(origin);
    dir = _inverseViewMatrix.TransformDir(dir).GetNormalized();
    up = _inverseViewMatrix.TransformDir(up).GetNormalized();

    double prjMatrix[4][4];
    renderPassState->GetProjectionMatrix().Get(prjMatrix);
    float fov = 2.0 * std::atan(1.0 / prjMatrix[1][1]) * 180.0 / M_PI;

    ospSetVec3f(_camera, "pos", origin[0], origin[1], origin[2]);
    ospSetVec3f(_camera, "dir", dir[0], dir[1], dir[2]);
    ospSetVec3f(_camera, "up", up[0],up[1],up[1]);
    ospSetFloat(_camera, "fovy", fov);
    ospCommit(_camera);

    // XXX: Add collection and renderTags support.
    // update conig options
    int currentSettingsVersion = renderDelegate->GetRenderSettingsVersion();
    if (_lastSettingsVersion != currentSettingsVersion)
	{
        _samplesToConvergence = renderDelegate->GetRenderSetting<int>(
               HdOSPRayRenderSettingsTokens->samplesToConvergence, 100);
        float aoDistance = renderDelegate->GetRenderSetting<float>(
               HdOSPRayRenderSettingsTokens->aoDistance, 10.f);
        _useDenoiser = renderDelegate->GetRenderSetting<bool>(
               HdOSPRayRenderSettingsTokens->useDenoiser, false);
        int spp = renderDelegate->GetRenderSetting<int>(
               HdOSPRayRenderSettingsTokens->samplesPerFrame, 1);
        int aoSamples = renderDelegate->GetRenderSetting<int>(
               HdOSPRayRenderSettingsTokens->ambientOcclusionSamples, 0);
        int maxDepth = renderDelegate->GetRenderSetting<int>(
               HdOSPRayRenderSettingsTokens->maxDepth, 5);
        _ambientLight = renderDelegate->GetRenderSetting<bool>(
               HdOSPRayRenderSettingsTokens->ambientLight, false);
        //default static ospray directional lights
        _staticDirectionalLights = renderDelegate->GetRenderSetting<bool>(
               HdOSPRayRenderSettingsTokens->staticDirectionalLights, false);
        //eye, key, fill, and back light are copied from USD GL (Storm) defaults.
        _eyeLight = renderDelegate->GetRenderSetting<bool>(
               HdOSPRayRenderSettingsTokens->eyeLight, false);
        _keyLight = renderDelegate->GetRenderSetting<bool>(
               HdOSPRayRenderSettingsTokens->keyLight, false);
        _fillLight = renderDelegate->GetRenderSetting<bool>(
               HdOSPRayRenderSettingsTokens->fillLight, false);
        _backLight = renderDelegate->GetRenderSetting<bool>(
               HdOSPRayRenderSettingsTokens->backLight, false);
        if (spp != _spp || aoSamples != _aoSamples || aoDistance != aoDistance
            || maxDepth != _maxDepth) {
            _spp = spp;
            _aoSamples = aoSamples;
            _maxDepth = maxDepth;
            ospSetInt(_renderer, "spp", _spp);
            ospSetInt(_renderer, "aoSamples", _aoSamples);
            ospSetFloat(_renderer, "aoDistance", _aoSamples);
            ospSetInt(_renderer, "maxDepth", _maxDepth);
            ospCommit(_renderer);
        }
        _lastSettingsVersion = currentSettingsVersion;
        ResetImage();
    }
    // XXX: Add clip planes support.

    // add lights
    GfVec3f up_light = up;
    GfVec3f dir_light = dir;
    if (_staticDirectionalLights) {
        up_light = {0.f, 1.f, 0.f};
        dir_light = {-.1f, -.1f, -.8f};
    }
    GfVec3f right_light = GfCross(dir_light, up_light);
    std::vector<OSPLight> lights;
    if (_ambientLight) 
	{
        OSPLight ambient = ospNewLight( "ambient"); // _renderer,
		// !!!!! ADD TO RENDERER
        ospSetVec3f(ambient, "color", 1.f, 1.f, 1.f);
        ospSetFloat(ambient, "intensity", 0.45f);
        ospCommit(ambient);
        lights.push_back(ambient);
    }

    if (_eyeLight) 
	{
		// !!!!! ADD TO RENDERER ??
		OSPLight eyeLight = ospNewLight("directional");
        ospSetVec3f(eyeLight, "color", 1.f, 232.f / 255.f, 166.f / 255.f);
        ospSetVec3f(eyeLight, "direction", dir_light[0], dir_light[1], dir_light[2]);
        ospSetFloat(eyeLight, "intensity", 3.3f);
        ospCommit(eyeLight);
        lights.push_back(eyeLight);
    }
    const float angularDiameter = 4.5f;
    const float glToPTLightIntensityMultiplier = 1.5f;
    if (_keyLight) {
		OSPLight keyLight = ospNewLight("directional");
        auto keyHorz = -1.0f / tan(rad(45.0f)) * right_light;
        auto keyVert = 1.0f / tan(rad(70.0f)) * up_light;
        auto lightDir = -(keyVert + keyHorz);
        ospSetVec3f(keyLight, "color", .8f, .8f, .8f);
        ospSetVec3f(keyLight, "direction", lightDir[0], lightDir[1], lightDir[2]);
        ospSetFloat(keyLight, "intensity", glToPTLightIntensityMultiplier);
        ospSetFloat(keyLight, "angularDiameter", angularDiameter);
        ospCommit(keyLight);
        lights.push_back(keyLight);
    }
    if (_fillLight) 
	{
        OSPLight fillLight = ospNewLight( "directional"); //_renderer,
        auto fillHorz = 1.0f / tan(rad(30.0f)) * right_light;
        auto fillVert = 1.0f / tan(rad(45.0f)) * up_light;
        auto lightDir = (fillVert + fillHorz);
        ospSetVec3f(fillLight, "color", .6f, .6f, .6f);
        ospSetVec3f(fillLight, "direction", lightDir[0], lightDir[1], lightDir[2]);
        ospSetFloat(fillLight, "intensity", glToPTLightIntensityMultiplier);
        ospSetFloat(fillLight, "angularDiameter", angularDiameter);
        ospCommit(fillLight);
        lights.push_back(fillLight);
    }
    if (_backLight) {
        auto backLight = ospNewLight("directional"); // _renderer, 
        auto backHorz = 1.0f / tan(rad(60.0f)) * right_light;
        auto backVert = -1.0f / tan(rad(60.0f)) * up_light;
        auto lightDir = (backHorz + backVert);
        ospSetVec3f(backLight, "color", .6f, .6f, .6f);
        ospSetVec3f(backLight, "direction", lightDir[0], lightDir[1], lightDir[2]);
        ospSetFloat(backLight, "intensity", glToPTLightIntensityMultiplier);
        ospSetFloat(backLight, "angularDiameter", angularDiameter);
        ospCommit(backLight);
        lights.push_back(backLight);
    }
    OSPData lightArray = ospNewData(lights.size(), OSP_OBJECT, &(lights[0]));
    ospSetData(_renderer, "lights", lightArray);
    ospRelease(lightArray);
    ospCommit(_renderer);

    // If the viewport has changed, resize the sample buffer.
    GfVec4f vp = renderPassState->GetViewport();
    if (_width != vp[2] || _height != vp[3]) {
        _width = vp[2];
        _height = vp[3];
        if (_frameBuffer)
            ospRelease(_frameBuffer);
        _frameBuffer = ospNewFrameBuffer(_width, _height, OSP_FB_RGBA32F,
               OSP_FB_COLOR | OSP_FB_ACCUM |
//#if HDOSPRAY_ENABLE_DENOISER
                      OSP_FB_NORMAL | OSP_FB_ALBEDO |
//#endif
                      0);
        ospCommit(_frameBuffer);
        _colorBuffer.resize(_width * _height);
        _normalBuffer.resize(_width * _height);
        _albedoBuffer.resize(_width * _height);
        _denoisedBuffer.resize(_width * _height);
        _pendingResetImage = true;
        _denoiserDirty = true;
    }

    int currentSceneVersion = _sceneVersion->load();
    if (_lastRenderedVersion != currentSceneVersion) {
        ResetImage();
        _lastRenderedVersion = currentSceneVersion;
    }

    int currentModelVersion = _renderParam->GetModelVersion();
    if (_lastRenderedModelVersion != currentModelVersion) {
        ResetImage();
        _lastRenderedModelVersion = currentModelVersion;
        _pendingModelUpdate = true;
    }

    if (_pendingModelUpdate) {
        // release resources from last committed scene
        if (oldModel) {
            for (auto instance : oldInstances) {
                ospRelease(instance);
            }
            ospRelease(oldModel);
            oldModel = nullptr;
            oldInstances.resize(0);
        }

        // create new model and populate with mesh instances
        OSPModel model = ospNewModel();
        oldInstances.reserve(_renderParam->GetInstances().size());
        for (auto instance : _renderParam->GetInstances()) {
            ospAddGeometry(model, instance);
            oldInstances.push_back(instance);
        }
        ospCommit(model);
        ospSetObject(_renderer, "model", model);
        oldModel = model;
        ospCommit(_renderer);
        _pendingModelUpdate = false;
        _renderParam->ClearInstances();
    }

    // Reset the sample buffer if it's been requested.
    if (_pendingResetImage) {
        ospFrameBufferClear(_frameBuffer, OSP_FB_ACCUM);
        _pendingResetImage = false;
        _numSamplesAccumulated = 0;
        if (_useDenoiser) 
		{
            _spp = HdOSPRayConfig::GetInstance().samplesPerFrame;
            ospSetInt(_renderer, "spp", _spp);
            ospCommit(_renderer);
        }
    }

    // Render the frame
    ospRenderFrame(_frameBuffer, _renderer,
                   OSP_FB_COLOR | OSP_FB_ACCUM |
//#if HDOSPRAY_ENABLE_DENOISER
                          OSP_FB_NORMAL | OSP_FB_ALBEDO |
//#endif
                          0);
    _numSamplesAccumulated += std::max(1, _spp);

    // Resolve the image buffer: find the average color per pixel by
    // dividing the summed color by the number of samples;
    // and convert the image into a GL-compatible format.
    const void* rgba = ospMapFrameBuffer(_frameBuffer, OSP_FB_COLOR);
    memcpy((void*)&_colorBuffer[0], rgba, _width * _height * 4 * sizeof(float));
    ospUnmapFrameBuffer(rgba, _frameBuffer);
    if (_useDenoiser && _numSamplesAccumulated >= _denoiserSPPThreshold) {
        int newSPP
               = std::max((int)HdOSPRayConfig::GetInstance().samplesPerFrame, 1)
               * 4;
        if (_spp != newSPP) {
            ospSetInt(_renderer, "spp", _spp);
            ospCommit(_renderer);
        }
        Denoise();
    }

    // Blit!
    glDrawPixels(_width, _height, GL_RGBA, GL_FLOAT, &_colorBuffer[0]);
}

void
HdOSPRayRenderPass::Denoise()
{
    _denoisedBuffer = _colorBuffer;
//#if HDOSPRAY_ENABLE_DENOISER
    const auto size = _width * _height;
    const ospcommon::math::vec4f* rgba
           = (const ospcommon::math::vec4f*)ospMapFrameBuffer(_frameBuffer, OSP_FB_COLOR);
    std::copy(rgba, rgba + size, _colorBuffer.begin());
    ospUnmapFrameBuffer(rgba, _frameBuffer);
    const ospcommon::math::vec3f* normal
           = (const ospcommon::math::vec3f*)ospMapFrameBuffer(_frameBuffer, OSP_FB_NORMAL);
    std::copy(normal, normal + size, _normalBuffer.begin());
    ospUnmapFrameBuffer(normal, _frameBuffer);
    const ospcommon::math::vec3f* albedo
           = (const ospcommon::math::vec3f*)ospMapFrameBuffer(_frameBuffer, OSP_FB_ALBEDO);
    std::copy(albedo, albedo + size, _albedoBuffer.begin());
    ospUnmapFrameBuffer(albedo, _frameBuffer);

    if (_denoiserDirty) 
	{
        _denoiserFilter.setImage("color", (void*)_colorBuffer.data(),
                                 oidn::Format::Float3, _width, _height, 0,
                                 sizeof(ospcommon::math::vec4f));

        _denoiserFilter.setImage("normal", (void*)_normalBuffer.data(),
                                 oidn::Format::Float3, _width, _height, 0,
                                 sizeof(ospcommon::math::vec3f));

        _denoiserFilter.setImage("albedo", (void*)_albedoBuffer.data(),
                                 oidn::Format::Float3, _width, _height, 0,
                                 sizeof(ospcommon::math::vec3f));

        _denoiserFilter.setImage("output", _denoisedBuffer.data(),
                                 oidn::Format::Float3, _width, _height, 0,
                                 sizeof(ospcommon::math::vec4f));

        _denoiserFilter.set("hdr", true);
        _denoiserFilter.commit();
        _denoiserDirty = false;
    }

    _denoiserFilter.execute();
    _colorBuffer = _denoisedBuffer;
    // Carson: we can avoid having 2 buffers
//#endif
}

//PXR_NAMESPACE_CLOSE_SCOPE
