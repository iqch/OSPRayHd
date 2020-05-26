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
#ifndef HDOSPRAY_SAMPLER_H
#define HDOSPRAY_SAMPLER_H

// CRT
#include <cstddef>

// PXR


#include <pxr/imaging/glf/glew.h>
#include <pxr/pxr.h>

#include <pxr/imaging/hd/enums.h>
#include <pxr/imaging/hd/vtBufferSource.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/gf/vec2d.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec2i.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec3i.h>
#include <pxr/base/gf/vec4d.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/gf/vec4i.h>

using namespace pxr;

//PXR_NAMESPACE_OPEN_SCOPE

/// \class HdOSPRayTypeHelper
///
/// A utility class that helps map between C++ types and Hd type tags.
class HdOSPRayTypeHelper {
public:
    /// Return the HdTupleType corresponding to the given C++ type.
    template <typename T>
    static HdTupleType GetTupleType();

    /// Define a type that can hold one sample of any primvar.
    typedef char PrimvarTypeContainer[sizeof(GfMatrix4d)];
};

// Define template specializations of HdOSPRayTypeHelper methods for
// all our supported types...
#define TYPE_HELPER(T, type)                                                   \
    template <>                                                                \
    inline HdTupleType HdOSPRayTypeHelper::GetTupleType<T>()                   \
    {                                                                          \
        return HdTupleType { type, 1 };                                        \
    }

TYPE_HELPER(bool, HdTypeBool)
TYPE_HELPER(char, HdTypeInt8)
TYPE_HELPER(short, HdTypeInt16)
TYPE_HELPER(unsigned short, HdTypeUInt16)
TYPE_HELPER(int, HdTypeInt32)
TYPE_HELPER(GfVec2i, HdTypeInt32Vec2)
TYPE_HELPER(GfVec3i, HdTypeInt32Vec3)
TYPE_HELPER(GfVec4i, HdTypeInt32Vec4)
TYPE_HELPER(unsigned int, HdTypeUInt32)
TYPE_HELPER(float, HdTypeFloat)
TYPE_HELPER(GfVec2f, HdTypeFloatVec2)
TYPE_HELPER(GfVec3f, HdTypeFloatVec3)
TYPE_HELPER(GfVec4f, HdTypeFloatVec4)
TYPE_HELPER(double, HdTypeDouble)
TYPE_HELPER(GfVec2d, HdTypeDoubleVec2)
TYPE_HELPER(GfVec3d, HdTypeDoubleVec3)
TYPE_HELPER(GfVec4d, HdTypeDoubleVec4)
TYPE_HELPER(GfMatrix4f, HdTypeFloatMat4)
TYPE_HELPER(GfMatrix4d, HdTypeDoubleMat4)
#undef TYPE_HELPER

/// \class HdOSPRayBufferSampler
///
/// A utility class that knows how to sample an element from a type-tagged
/// buffer (like HdVtBufferSource).
///
/// This class provides templated accessors to let the caller directly get the
/// final sample type; it also does bounds checks and type checks.
///
class HdOSPRayBufferSampler {
public:
    /// The constructor takes a reference to a buffer source. The data is
    /// owned externally; the caller is responsible for ensuring the buffer
    /// is alive while Sample() is being called.
    /// \param buffer The buffer being sampled.
    HdOSPRayBufferSampler(HdVtBufferSource const& buffer)
        : _buffer(buffer)
    {
    }

    /// Sample the buffer at element index \p index, and write the sample to
    /// \p value. Interpret \p value as having arity \p numComponents, each of
    /// type \p componentType. These parameters may not match the datatype
    /// declaration of the underlying buffer, in which case Sample returns
    /// false. Sample also returns false if \p index is out of bounds.
    ///
    /// For example, to sample data as GfVec3, \p dataType would be
    /// HdTupleType { HdTypeFloatVec3, 1 }.
    ///
    /// \param index The element index to sample.
    /// \param value The memory to write the value to (only written on success).
    /// \param dataType The HdTupleType describing element values.
    /// \return True if the value was successfully sampled.
    bool Sample(int index, void* value, HdTupleType dataType) const;

    // Convenient, templated frontend for Sample().
    template <typename T>
    bool Sample(int index, T* value) const
    {
        return Sample(index, static_cast<void*>(value),
                      HdOSPRayTypeHelper::GetTupleType<T>());
    }

private:
    HdVtBufferSource const& _buffer;
};
//PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDOSPRAY_SAMPLER_H
