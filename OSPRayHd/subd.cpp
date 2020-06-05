// CRT
#include <iostream>
using namespace std;

// PXR

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/imaging/hd/meshUtil.h>
#include <pxr/imaging/hd/smoothNormals.h>

// OPENSUBD ?
#include <pxr/imaging/pxOsd/tokens.h>
#include <pxr/imaging/pxOsd/refinerFactory.h>
#include <pxr/imaging/pxOsd/meshTopology.h>
#include <pxr/imaging/pxOsd/subdivTags.h>

#include <opensubdiv/far/topologyRefiner.h>
#include <opensubdiv/far/primvarRefiner.h>

//using namespace ;
using namespace OpenSubdiv::OPENSUBDIV_VERSION::Far;
//using namespace Far;

struct Vertex {

	// Minimal required interface ----------------------
	Vertex() { }

	Vertex(Vertex const & src) {
		_position[0] = src._position[0];
		_position[1] = src._position[1];
		_position[2] = src._position[2];
	}

	void Clear(void * = 0) {
		_position[0] = _position[1] = _position[2] = 0.0f;
	}

	void AddWithWeight(Vertex const & src, float weight) {
		_position[0] += weight*src._position[0];
		_position[1] += weight*src._position[1];
		_position[2] += weight*src._position[2];
	}

	// Public interface ------------------------------------
	void SetPosition(float x, float y, float z) {
		_position[0] = x;
		_position[1] = y;
		_position[2] = z;
	}

	const float * GetPosition() const {
		return _position;
	}

	const float& operator[](int pos) const { return _position[pos]; };

private:
	float _position[3];
};

// OSPRAY

#include <ospray/ospray.h>
#include <ospray/ospray_util.h>

#include <ospcommon/math/vec.h>
#include <ospcommon/math/AffineSpace.h>

// HDOSPRAY

#include "renderParam.h"
#include "subd.h"

///////////////////////////////////////////

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
	HdOSPRayTokens,
	(st)
);
// clang-format on

void HdOSPRaySubd::Sync(HdSceneDelegate* sceneDelegate,
	HdRenderParam* _renderParam, HdDirtyBits* dirtyBits,
	TfToken const& reprToken)
{
	HD_TRACE_FUNCTION();
	HF_MALLOC_TAG_FUNCTION();

	SdfPath const& id = GetId();

	_MeshReprConfig::DescArray descs = _GetReprDesc(reprToken);
	const HdMeshReprDesc& desc = descs[0];

	HdOSPRayRenderParam* renderParam = (HdOSPRayRenderParam*)_renderParam;

	// MATERIAL // ?
	if (*dirtyBits & HdChangeTracker::DirtyMaterialId)
	{
		_SetMaterialId(sceneDelegate->GetRenderIndex().GetChangeTracker(),
			sceneDelegate->GetMaterialId(id));
	};

	// Create ospray geometry objects.

	//////////////////////////////////////////////////////////////////

	// POOR MAN DEGRADATION METRCS COMPUTING
	bool deg = desc.geomStyle == HdMeshGeomStyleHullEdgeOnly;

	if (deg != renderParam->degrading)
	{
		renderParam->degrading = deg;
		//cout << "CHANGED DEGRADING TO " << deg << endl;
	};

	// VISIBILITY - WHICH VISIBILITY,BTW?
	if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, id))
	{
		// ...SHOILD I PASS THIS TOO?
		_UpdateVisibility(sceneDelegate, dirtyBits);
		//if (!IsVisible())
		//{
		//	ospRelease(_instance);
		//	_instance = NULL;
		//	return;
		//}
	};

	// TRANSFORM
	if (HdChangeTracker::IsTransformDirty(*dirtyBits, id))
	{
		_transform = GfMatrix4f(sceneDelegate->GetTransform(id));
	};

	// SHAPE
	while (
		HdChangeTracker::IsDisplayStyleDirty(*dirtyBits, id) ||
		HdChangeTracker::IsTopologyDirty(*dirtyBits, id) ||
		HdChangeTracker::IsSubdivTagsDirty(*dirtyBits, id) ||
		HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->points) ||
		HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->normals) ||
		// WHAT??? HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->primvar) ||
		HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->displayColor) ||
		// SORRY, NO OPACITY SUPPORT
		//|| HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->displayOpacity)
		HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdOSPRayTokens->st)
		)
	{
		ospRelease(_instance);
		_instance = NULL;

		//for (OSPGeometry& g : _mesh) ospRelease(g);
		//for(OSPGeometricModel& m : _model) ospRelease(m);
		_mesh.clear();
		_model.clear();

		HdDisplayStyle const displayStyle = sceneDelegate->GetDisplayStyle(id);
		_refineLevel = displayStyle.refineLevel;

		cout << "REFINE : " << _refineLevel << endl;

		_topology = GetMeshTopology(sceneDelegate).GetPxOsdMeshTopology(); // , _refineLevel); // NO HYDRA REFINEMET PLEASE
		_topology.SetSubdivTags(sceneDelegate->GetSubdivTags(id));

		//cout << "TOPOLOGY 1: " << _topology << endl;
		//cout << "SCHEME : " << _topology.GetScheme() << endl;

		PxOsdTopologyRefinerSharedPtr refiner = PxOsdRefinerFactory::Create(_topology);

		refiner->RefineUniform(TopologyRefiner::UniformOptions(_refineLevel));

		vector<Vertex>  vbuffer(refiner->GetNumVerticesTotal());
		Vertex * verts = &vbuffer[0];

		_points = sceneDelegate->Get(id, HdTokens->points).Get<VtVec3fArray>();
		for (int i = 0; i<_points.size(); ++i)
		{
			verts[i].SetPosition(_points[i][0], _points[i][1], _points[i][2]);
		}

		PrimvarRefiner primvarRefiner(*refiner.get());

		Vertex * src = verts;
		for (int level = 0; level < _refineLevel; level++)
		{
			Vertex * dst = src + refiner->GetLevel(level).GetNumVertices();
			primvarRefiner.Interpolate(level+1, src, dst);
			src = dst;
		};

		const TopologyLevel& lv = refiner->GetLevel(_refineLevel);

		//cout << "NPT : " << lv.GetNumVertices() << endl;

		//OSPData points = ospNewSharedData1D(
		//	&vbuffer[_points.size()], OSP_VEC3F,
		//	lv.GetNumVertices());
		//ospCommit(points);

		static unsigned int IDX[] = { 3,2,1,0 };
		OSPData indices = ospNewSharedData(IDX, OSP_VEC4UI, 1);
		ospCommit(indices);

		for (int i = 0; i < lv.GetNumFaces(); i++)
		{
			OSPGeometry m = ospNewGeometry("mesh");
			ospSetObject(m, "index", indices);

			ConstIndexArray ia = lv.GetFaceVertices(i);
			//int idx[4] = { ia[0], ia[1], ia[2], ia[3] };

			vector<Vertex> pt(4);

			int offset = refiner->GetNumVerticesTotal() - lv.GetNumVertices();

			pt[0] = vbuffer[offset + ia[0]];
			pt[1] = vbuffer[offset + ia[1]];
			pt[2] = vbuffer[offset + ia[2]];
			pt[3] = vbuffer[offset + ia[3]];

			float* P = new float[12];
			int ii = 0;
			//cout <<  "\tPT: ";
			for (Vertex& v : pt)
			{
				P[ii] = v[0]; ii++;
				P[ii] = v[1]; ii++;
				P[ii] = v[2]; ii++;
			};

			OSPData points = ospNewSharedData(P, OSP_VEC3F,4);
			ospCommit(points);
			ospSetObject(m, "vertex.position", points);

			ospCommit(m);

			_mesh.push_back(m);
		}

		//cout << "FTOT : " << ref->GetNumFacesTotal() << endl;

		//L.GetNumFaces()
		//L.GetFaceVertices()

		//L.GetNumFaceVertices()




		//cout << "TOPOLOGY 2: " << _topology << endl;


		//_faces = _topology.GetFaceVertexCounts();

		//cout << "FACES : " << _faces.size() << endl;

		//int tf = 0;

		//bool valid = true;
		//for (int i : _faces)
		//{
		//	if (i == 4) continue;
		//	if (i == 3) 
		//	{
		//		tf++;  continue;
		//	};

		//	valid = false;
		//	break;
		//};

		//if (!valid)
		//{
		//	cout << "3/4-mesh required" << endl;
		//	break;
		//};

		//_points = sceneDelegate->Get(id, HdTokens->points).Get<VtVec3fArray>();
		//OSPData points = ospNewSharedData1D(_points.cdata(), OSP_VEC3F, _points.size());
		//ospCommit(points);

		//cout << "POINTS : " << _points.size() << endl;


		//_indices = _topology.GetFaceVertexIndices();



		//const int* idx = _indices.cdata();

		//for(int i = 0; i < _faces.size(); i++)
		//{
		//	OSPGeometry m = ospNewGeometry("mesh");
		//	ospSetObject(m, "vertex.position", points);

		//	OSPData indices = ospNewSharedData1D(idx, OSP_VEC4UI, 1);
		//	ospCommit(indices);
		//	ospSetObject(m, "index", indices);

		//	idx += 4;

		//	ospCommit(m);

		//	_mesh.push_back(m);
		//};

		//_mesh = ospNewGeometry("mesh");

		//OSPData faces = ospNewSharedData1D(_faces.cdata(), OSP_UINT, _faces.size());
		//ospCommit(faces);
		//ospSetObject(_mesh, "face", faces);

		//_indices = _topology.GetFaceVertexIndices();
		//OSPData indices = ospNewSharedData1D(_indices.cdata(),
		//	OSP_VEC4UI, _indices.size()/4);
		//	/*OSP_UINT, _indices.size());*/
		//ospCommit(indices);
		//ospSetObject(_mesh, "index", indices);


		//ospSetObject(_mesh, "vertex.position", points);

		//float level = std::max(0,2 << _refineLevel * 2 - 1);
		//ospSetFloat(_mesh, "level", level);
		//ospSetInt(_mesh, "mode", OSP_SUBDIVISION_PIN_BOUNDARY);

		//ospCommit(_mesh);

		break;
	};

	/////////////////////////////////////////////////////////////////

	if (_mesh.size() == 0)
	{
		//ospRelease(_instance);
		return;
	};

	for (OSPGeometry& m : _mesh)
	{
		OSPGeometricModel M = ospNewGeometricModel(m);
		ospSetObject(M, "material", renderParam->_material);
		ospCommit(M);
		_model.push_back(M);
	}

	_group = ospNewGroup();

	OSPData geometricModels = ospNewSharedData(&_model[0], OSP_GEOMETRIC_MODEL, _model.size());
	ospCommit(geometricModels);

	ospSetObject(_group, "geometry", geometricModels);
	ospCommit(_group);

	_instance = ospNewInstance(_group);
	ospCommit(_instance);

	renderParam->AddInstance(_instance);

	//ospRetain(_instance);
};

// CTOR/DTOR/INIT MACHINERY

HdOSPRaySubd::HdOSPRaySubd(SdfPath const& id, SdfPath const& instancerId) : HdMesh(id, instancerId) {};

void
HdOSPRaySubd::Finalize(HdRenderParam* renderParam)
{
	//if (_instance) ospRelease(_instance);
	//if (_group) ospRelease(_group);
	//if (_model) ospRelease(_model);
	//if (_mesh) ospRelease(_mesh);
};

HdDirtyBits
HdOSPRaySubd::GetInitialDirtyBitsMask() const
{
	int mask = HdChangeTracker::Clean | HdChangeTracker::InitRepr
		| HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyTopology
		| HdChangeTracker::DirtyTransform | HdChangeTracker::DirtyVisibility
		| HdChangeTracker::DirtyDisplayStyle
		| HdChangeTracker::DirtySubdivTags | HdChangeTracker::DirtyPrimvar
		| HdChangeTracker::DirtyNormals | HdChangeTracker::DirtyInstanceIndex;
		//| HdChangeTracker::AllDirty;

	return (HdDirtyBits)mask;
};

void
HdOSPRaySubd::_InitRepr(TfToken const& reprToken, HdDirtyBits* dirtyBits)
{
	TF_UNUSED(dirtyBits);
	_ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(), _ReprComparator(reprToken));

	if (it == _reprs.end())
	{
		_reprs.emplace_back(reprToken, HdReprSharedPtr());
	};
};

HdDirtyBits HdOSPRaySubd::_PropagateDirtyBits(HdDirtyBits bits) const { return bits; }; // PURE VIRTUAL AND USELESS

PXR_NAMESPACE_CLOSE_SCOPE
