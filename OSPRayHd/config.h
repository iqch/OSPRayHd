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
#ifndef HDOSPRAY_CONFIG_H
#define HDOSPRAY_CONFIG_H

// CRT

#include <string>
#include <vector>

using namespace std;

// PXR

#include <pxr/base/tf/singleton.h>
#include <pxr/pxr.h>

using namespace pxr;

// OSPRAY

#include <ospray/ospray.h>


//PXR_NAMESPACE_OPEN_SCOPE

/// \class HdOSPRayConfig
///
/// This class is a singleton, holding configuration parameters for HdOSPRay.
/// Everything is provided with a default, but can be overridden using
/// environment variables before launching a hydra process.
///
/// Many of the parameters can be used to control quality/performance
/// tradeoffs, or to alter how HdOSPRay takes advantage of parallelism.
///
/// At startup, this class will print config parameters if
/// *HDOSPRAY_PRINT_CONFIGURATION* is true. Integer values greater than zero
/// are considered "true".
///
class HdOSPRayConfig {
public:
    /// \brief Return the configuration singleton.
    static const HdOSPRayConfig& GetInstance();

    /// How many samples does each pixel get per frame?
    ///
    /// Override with *HDOSPRAY_SAMPLES_PER_FRAME*.
    unsigned int samplesPerFrame;

    /// How many samples do we need before a pixel is considered
    /// converged?
    ///
    /// Override with *HDOSPRAY_SAMPLES_TO_CONVERGENCE*.
    unsigned int samplesToConvergence;

    /// How many ambient occlusion rays should we generate per
    /// camera ray?
    ///
    /// Override with *HDOSPRAY_AMBIENT_OCCLUSION_SAMPLES*.
    unsigned int ambientOcclusionSamples;

    /// What should the intensity of the camera light be, specified as a
    /// percent of <1, 1, 1>.  For example, 300 would be <3, 3, 3>.
    ///
    /// Override with *HDOSPRAY_CAMERA_LIGHT_INTENSITY*.
    float cameraLightIntensity;

    ///  Whether OSPRay uses path tracing or scivis renderer.
    ///
    /// Override with *HDOSPRAY_USE_PATHTRACING*.
    bool usePathTracing;

    ///  Whether OSPRay uses denoiser
    ///
    /// Override with *HDOSPRAY_USE_DENOISER*.
    bool useDenoiser;

    /// Initialization arguments sent to OSPRay.
    ///  This can be used to set ospray configurations like mpi.
    ///
    /// Override with *HDOSPRAY_INIT_ARGS*.
    std::string initArgs;

    ///  Whether OSPRay forces quad meshes for debug
    ///
    /// Override with *HDOSPRAY_FORCE_QUADRANGULATE*.
    bool forceQuadrangulate;

    ///  Maximum ray depth
    ///
    /// Override with *HDOSPRAY_MAX_DEPTH*.
    int maxDepth { 8 };

    ///  Ao rays maximum distance
    ///
    /// Override with *HDOSPRAY_AO_DISTANCE*.
    float aoDistance { 15.0f };

    ///  Use an ambient light
    ///
    /// Override with *HDOSPRAY_AMBIENT_LIGHT*.
    bool ambientLight { true };

    ///  Use an eye light
    ///
    /// Override with *HDOSPRAY_STATIC_DIRECTIONAL_LIGHTS*.
    bool staticDirectionalLights { true };

    ///  Use an eye light
    ///
    /// Override with *HDOSPRAY_EYE_LIGHT*.
    bool eyeLight { false };

    ///  Use a key light
    ///
    /// Override with *HDOSPRAY_KEY_LIGHT*.
    bool keyLight { true };

    ///  Use a fill light
    ///
    /// Override with *HDOSPRAY_FILL_LIGHT*.
    bool fillLight { true };

    ///  Use a back light
    ///
    /// Override with *HDOSPRAY_BACK_LIGHT*.
    bool backLight { true };

    // meshes populate global instances.  These are then committed by the
    // renderPass into a scene.
    std::vector<OSPGeometry> ospInstances;

private:
    // The constructor initializes the config variables with their
    // default or environment-provided override, and optionally prints
    // them.
    HdOSPRayConfig();
    ~HdOSPRayConfig() = default;

    HdOSPRayConfig(const HdOSPRayConfig&) = delete;
    HdOSPRayConfig& operator=(const HdOSPRayConfig&) = delete;

    friend class TfSingleton<HdOSPRayConfig>;
};

//PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDOSPRAY_CONFIG_H
