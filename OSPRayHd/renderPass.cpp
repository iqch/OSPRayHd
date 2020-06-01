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
//#include <pxr/imaging/glf/glew.h>

#include <pxr/imaging/hd/perfLog.h>
#include <pxr/imaging/hd/renderPassState.h>

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/work/loops.h>

// OIIO

#include <OpenImageIO/imageio.h>
OIIO_NAMESPACE_USING;

// OSPRAYHD

#include "renderParam.h"
#include "renderBuffer.h"
#include "renderPass.h"

#include "config.h"
#include "context.h"
#include "mesh.h"
#include "renderDelegate.h"


PXR_NAMESPACE_OPEN_SCOPE

inline float rad(float deg) { return deg * M_PI / 180.0f; }

HdOSPRayRenderPass::HdOSPRayRenderPass(
       HdRenderIndex* index, HdRprimCollection const& collection,
       std::atomic<int>* sceneVersion,
       std::shared_ptr<HdOSPRayRenderParam> renderParam)
    : HdRenderPass(index, collection)
    , _pendingResetImage(false)
    , _pendingModelUpdate(true)
    , _sceneVersion(sceneVersion)
    , _lastRenderedVersion(0)
    , _lastRenderedModelVersion(0)
    , _width(0)
    , _height(0)
    , _inverseViewMatrix(1.0f) // == identity
    , _inverseProjMatrix(1.0f) // == identity
    , _renderParam(renderParam)
	//, _colorBuffer(SdfPath::EmptyPath())
{
	_renderer = ospNewRenderer("pathtracer");

	ospSetInt(_renderer, "pixelSamples", 1);
	//ospSetVec4f(_renderer, "backgroundColor", 0.0, 0.0, 0.0, 1.0);

	ospCommit(_renderer);


    _camera = ospNewCamera("perspective");
	ospSetFloat(_camera, "nearClip", 0.05);
	//ospSetVec2f(_camera, "imageStart", 0.0, 1.0);
	//ospSetVec2f(_camera, "imageEnd", 1.0, 0.0);
	ospCommit(_camera);

	_world = ospNewWorld();

	// JUST ONE DAYLIGHT

	_sunlight = ospNewLight("sunSky");
	ospSetVec3f(_sunlight, "direction", 0.0, 0.5, 0.5);
	ospSetFloat(_sunlight, "intensity", 1);
	ospCommit(_sunlight);

	ospSetObjectAsData(_world, "light", OSP_LIGHT, _sunlight);

	//if(false)
	{
		_geometry = ospNewGeometry("sphere");

		static float pos[] = { 0.0,0.0,0.0, 0.5,0.0,0.0 };

		_positions = ospNewSharedData(pos, OSP_VEC3F, 2);
		ospCommit(_positions);

		ospSetObject(_geometry, "sphere.position", _positions);
		ospSetFloat(_geometry, "radius", 0.15);

		ospCommit(_geometry);

		 _model = ospNewGeometricModel(_geometry);


		_material = ospNewMaterial("pathtracer", "obj");

		//ospSetFloat(MT, "attenuationDistance", 0.25);
		//ospSetVec3f(MT, "attenuationColor", 1.0, 0.45, 0.15);
		ospCommit(_material);

		ospSetObject(_model, "material", _material);
		ospCommit(_model);

		_group = ospNewGroup();

		OSPData geometricModels = ospNewSharedData1D(&_model, OSP_GEOMETRIC_MODEL, 1);
		ospSetObject(_group, "geometry", geometricModels);
		ospCommit(_group);

		// put the group in an instance (a group can be instanced more than once)
		_instance = ospNewInstance(_group);
		ospCommit(_instance);

		// put the instance in the world
		_instances = ospNewSharedData1D(&_instance, OSP_INSTANCE, 1);
		ospSetObject(_world, "instance", _instances);
	}

	ospCommit(_world);

	_denoise = ospNewImageOperation("denoiser");
	ospCommit(_denoise);
};

HdOSPRayRenderPass::~HdOSPRayRenderPass() 
{
	// SHOULD WE CLEAR OSPWorld?
	ospRelease(_camera);
	ospRelease(_world);
	ospRelease(_renderer);
	ospRelease(_denoise);
};

void HdOSPRayRenderPass::ResetImage()
{
    // Set a flag to clear the sample buffer the next time Execute() is called.
    _pendingResetImage = true;
};

bool HdOSPRayRenderPass::IsConverged() const
{
    // A super simple heuristic: consider ourselves converged after N
    // samples. Since we currently uniformly sample the framebuffer, we can
    // use the sample count from pixel(0,0).
	//unsigned int samplesToConvergence = 10; // HdOSPRayConfig::GetInstance().samplesToConvergence;
    //return ((unsigned int)_numSamplesAccumulated >= samplesToConvergence);

	return _converged;
}

void HdOSPRayRenderPass::_MarkCollectionDirty()
{
    // If the drawable collection changes, we should reset the sample buffer.
    _pendingResetImage = true;
    _pendingModelUpdate = true;
}

void
HdOSPRayRenderPass::_Execute(HdRenderPassStateSharedPtr const& renderPassState,
                             TfTokenVector const& renderTags)
{
	_converged = false;

    HdRenderDelegate* renderDelegate = GetRenderIndex()->GetRenderDelegate();

    // Update camera
    GfMatrix4d inverseViewMatrix
           = renderPassState->GetWorldToViewMatrix().GetInverse();
	GfMatrix4d inverseProjMatrix
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

    ospSetVec3f(_camera, "position", origin[0], origin[1], origin[2]);
    ospSetVec3f(_camera, "direction", dir[0], dir[1], dir[2]);
    ospSetVec3f(_camera, "up", up[0],up[1],up[1]);
	ospSetFloat(_camera, "fovy", fov);
	ospSetFloat(_camera, "nearClip", 0.005);
    ospCommit(_camera);

    // update conig options
    int currentSettingsVersion = renderDelegate->GetRenderSettingsVersion();
    if (_lastSettingsVersion != currentSettingsVersion)
	{
        _samplesToConvergence = renderDelegate->GetRenderSetting<int>(
               HdOSPRayRenderSettingsTokens->samplesToConvergence, 16);
		_useDenoiser = renderDelegate->GetRenderSetting<bool>(
			HdOSPRayRenderSettingsTokens->useDenoiser, true);

        _lastSettingsVersion = currentSettingsVersion;

		cout << "SAMPLES : " << _samplesToConvergence << endl;

        ResetImage();
	};


    // If the viewport has changed, resize the sample buffer.
    GfVec4f vp = renderPassState->GetViewport();
    if (_width != vp[2] || _height != vp[3])
	{
        _width = vp[2];
        _height = vp[3];
		if (_frameBuffer)
		{
			ospRelease(_frameBuffer);
		};
        _frameBuffer = ospNewFrameBuffer(_width, _height, OSP_FB_RGBA32F,
               OSP_FB_COLOR | OSP_FB_DEPTH | OSP_FB_VARIANCE | OSP_FB_ACCUM );
		//cout << "NEW FRAMEBUFFER(" << _width << "x" << _height << ")" << endl;
				
        ospCommit(_frameBuffer);



		_aovBindings = renderPassState->GetAovBindings();

		cout << "AOVS : " << _aovBindings.size() <<  endl;
		for (HdRenderPassAovBinding& aov : _aovBindings)
		{
			cout << aov.aovName << endl;
			cout << aov.renderBuffer << endl;

			if (aov.aovName == "color") _colorBuffer = (HdOSPRayRenderBuffer*)aov.renderBuffer;
			if (aov.aovName == "depth") _depthBuffer = (HdOSPRayRenderBuffer*)aov.renderBuffer;
		};

		//if (aovBindings.size() == 0)
		//{
		//	cout << "BUILD AOV BUFFERS  : RGBA \n";
		//	HdRenderPassAovBinding colorAov;
		//	colorAov.aovName = HdAovTokens->color;
		//	_colorBuffer = new HdOSPRayRenderBuffer();
		//	colorAov.renderBuffer = _colorBuffer;
		//	colorAov.clearValue = VtValue(GfVec4f(0.0707f, 0.0707f, 0.0707f, 1.0f));
		//	aovBindings.push_back(colorAov);

		//	//renderPassState->SetAovBindings(aovBindings);
		//}
		//else
		//{

		//}

		//_colorBuffer->Allocate(GfVec3i(_width, _height, 1), HdFormatFloat32Vec4, false);
		//_depthBuffer->Allocate(GfVec3i(_width,_height,1), HdFormatFloat32, false);
		//_normalBuffer.Allocate(GfVec3i(_width, _height, 1), HdFormatFloat32Vec3, false);
		_pendingResetImage = true;
	};

    int currentSceneVersion = _sceneVersion->load();
    if (_lastRenderedVersion != currentSceneVersion)
	{
        ResetImage();
        _lastRenderedVersion = currentSceneVersion;
	};

    int currentModelVersion = _renderParam->GetModelVersion();
    if (_lastRenderedModelVersion != currentModelVersion)
	{
        ResetImage();
        _lastRenderedModelVersion = currentModelVersion;
        _pendingModelUpdate = true;
	};

    if (_pendingModelUpdate)
	{
		//int instnum = _renderParam->GetInstances().size();
		//vector<OSPInstance>& _instances = _renderParam->GetInstances();	
		//OSPData instances = ospNewSharedData1D(&_instances[0], OSP_INSTANCE, instnum);
		//ospSetObject(_world, "instance", instances);
        //ospCommit(_world);

        _pendingModelUpdate = false;
        _renderParam->ClearInstances();
	};

    // Reset the sample buffer if it's been requested.
    if (_pendingResetImage)
	{
		ospResetAccumulation(_frameBuffer);

		float zeroC[4] = { 0.0f,0.0f,0.0f,1.0f };

		_colorBuffer->Map();
		_colorBuffer->Clear(4,zeroC);
		_colorBuffer->Unmap();

		float one = 10000000000.0;
		_depthBuffer->Map();
		_depthBuffer->Clear(1, &one);
		_depthBuffer->Unmap();

		//float zN[3] = { 0.0f,0.0f,1.0 };
		//_normalBuffer.Map();
		//_normalBuffer.Clear(3, zN);
		//_normalBuffer.Unmap();

        _pendingResetImage = false;
	};

	//_renderParam.
    // Render the frame
	for (int i = 0; i < _samplesToConvergence-(_useDenoiser ? 1 : 0); i++)
	{
		ospRenderFrameBlocking(_frameBuffer, _renderer, _camera, _world);
	};

	OSPData ops = ospNewSharedData1D(&_denoise, OSP_IMAGE_OPERATION, 1);

	if (_useDenoiser)
	{
		ospSetObject(_frameBuffer, "imageOperation", ops);
		ospCommit(_frameBuffer);
		ospRenderFrameBlocking(_frameBuffer, _renderer, _camera, _world);
		ospSetObject(_frameBuffer, "imageOperation", NULL);
		ospCommit(_frameBuffer);
	};
                         
	// COLOR
	if(_colorBuffer != NULL)
	{
		const float* rgba = (const float*)ospMapFrameBuffer(_frameBuffer, OSP_FB_COLOR);
		float* csrc = (float*)(_colorBuffer->Map());

		// ...MOVE DATA TO BUFFER
		//int idx = 0;
		//for (int i = 0; i < _width; i++)
		//for (int j = 0; j < _height; j++)
		//{
		//	_colorBuffer->Write(GfVec3i(i, j, 1), 4, rgba + idx);
		//	idx += 4;
		//};

		memcpy((void*)csrc, rgba, _width * _height * 4 * sizeof(float));

		if(false)
		{
			// SAVE
			const char *filename = "f:/temp/out/outdlg.png";
			const int channels = 4; // RGB
			ImageOutput* out = ImageOutput::create(filename);

			unsigned char* RGBA = new unsigned char[_width*_height * 4];

			for (int i = 0; i < _width*_height * 4; i++)
			{
				if (rgba[i] < 0.0f) { RGBA[i] = 0; continue; };
				if (rgba[i] > 1.0f) { RGBA[i] = 255; continue; };
				RGBA[i] = 255 * pow(rgba[i], 0.46);
			};

			ImageSpec spec(_width, _height, channels, TypeDesc::UINT8);
			out->open(filename, spec);
			out->write_image(TypeDesc::UINT8, RGBA);
			out->close();

			delete[] RGBA;
		}

		_colorBuffer->Unmap();

		_colorBuffer->SetConverged(true);

		ospUnmapFrameBuffer(rgba, _frameBuffer);
	}

	// DEPTH
	if(_depthBuffer!=NULL)
	{
		//const float* depth = (const float*)ospMapFrameBuffer(_frameBuffer, OSP_FB_DEPTH);
		//float* dsrc = (float*)(_depthBuffer->Map());

		//memcpy((void*)dsrc, depth, _width * _height * sizeof(float));

		//_depthBuffer->Unmap();

		//_depthBuffer->SetConverged(true);

		//ospUnmapFrameBuffer(depth, _frameBuffer);
	}

	// ALLES

	_converged = true;
}

PXR_NAMESPACE_CLOSE_SCOPE
