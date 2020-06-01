// CRT
#include <iostream>
#include <algorithm>

#include <math.h>

using namespace std;

#define NOMINMAX

// QT

//#include <QtGui\QImage>
//#include <QtGui\QPixmap>

// OSPRAY

#include <ospray\ospray.h>
#include <ospray\ospray_cpp.h>

// OIIO

#include <OpenImageIO/imageio.h>

//using namespace OIIO;
OIIO_NAMESPACE_USING;



int main(int argc, char* argv[])
{

	int init_error = ospInit(&argc, (const char**)argv);
	if (init_error != OSP_NO_ERROR)
	{
		std::cerr << "FATAL ERROR DURING INITIALIZATION!" << std::endl;
		return -1;
	}

	OSPDevice device = ospGetCurrentDevice();
	if (device == nullptr)
	{
		std::cerr << "FATAL ERROR DURING GETTING CURRENT DEVICE!" << std::endl;
		return -1;
	};

	ospDeviceSetStatusFunc(device, [](const char* msg) { std::cout << msg; });
	//ospDeviceSetStatusFunc(device, HdOSPRayRenderDelegate::HandleOSPRayStatusMsg);
	//ospDeviceSetErrorFunc(device, HdOSPRayRenderDelegate::HandleOSPRayError);
	ospDeviceSetErrorFunc(device, [](OSPError e, const char* msg) { std::cerr << "OSPRAY ERROR [" << e << "]: " << msg << std::endl; });

	ospDeviceCommit(device);

	ospLoadModule("denoiser");

	OSPRenderer renderer = ospNewRenderer("pathtracer");

	ospSetInt(renderer, "pixelSamples", 1);
	ospSetVec4f(renderer, "backgroundColor", 0.0, 0.0, 0.0, 1.0);
	

	ospCommit(renderer);

	// CAMERA
	OSPCamera camera = ospNewCamera("perspective");


	ospSetFloat(camera, "aspect", 1.0);

	ospSetVec3f(camera, "position", 1.0, 0.0, 0.0);
	ospSetVec3f(camera, "direction", -1.0, 0.0, 0.0);
	ospSetVec3f(camera, "up", 0.0, 1.0, 0.0);
	ospSetFloat(camera, "fovy", 45.0);
	ospSetFloat(camera, "nearClip", 0.05);
	ospSetVec2f(camera, "imageStart", 0.0, 1.0);
	ospSetVec2f(camera, "imageEnd", 1.0, 0.0);

	ospCommit(camera);

	// WORLD

	OSPWorld world = ospNewWorld();

	// JUST ONE DAYLIGHT

	OSPLight sunlight = ospNewLight("sunSky"); // _renderer, 
	ospSetVec3f(sunlight, "direction", 0.0, -0.5, 0.0);
	ospSetFloat(sunlight, "intensity", 1);
	ospCommit(sunlight);

	ospSetObjectAsData(world, "light", OSP_LIGHT, sunlight);

	// SCENE

	OSPGeometry S = ospNewGeometry("sphere");

	float pos[] = { 0.0,0.0,0.25, 0.0,0.0,-0.25};

	OSPData poss = ospNewSharedData(pos, OSP_VEC3F,2);
	ospCommit(poss);

	ospSetObject(S, "sphere.position", poss);
	ospSetFloat(S, "radius", 0.25);

	ospCommit(S);

	OSPGeometricModel M = ospNewGeometricModel(S);


	OSPMaterial MT = ospNewMaterial("pathtracer", "metal");

	ospSetFloat(MT, "attenuationDistance", 0.25);
	ospSetVec3f(MT, "attenuationColor", 1.0, 0.45, 0.15);
	

	ospCommit(MT);

	ospSetObject(M, "material", MT);
	ospCommit(M);

	OSPGroup G = ospNewGroup();

	OSPData geometricModels = ospNewSharedData1D(&M, OSP_GEOMETRIC_MODEL, 1);
	ospSetObject(G, "geometry", geometricModels);
	ospCommit(G);

	// put the group in an instance (a group can be instanced more than once)
	OSPInstance instance = ospNewInstance(G);
	ospCommit(instance);

	// put the instance in the world
	OSPData instances = ospNewSharedData1D(&instance, OSP_INSTANCE, 1);
	ospSetObject(world, "instance", instances);

	// RENDER

	ospCommit(world);

	int width = 512;
	int height = 512;

	OSPFrameBuffer framebuffer = ospNewFrameBuffer(width, height, OSP_FB_RGBA32F,
		OSP_FB_COLOR | OSP_FB_ACCUM | OSP_FB_VARIANCE);
	ospCommit(framebuffer);

	int samples = 64;

	for (int i = 0; i < samples-1; i++)
	{
		ospRenderFrameBlocking(framebuffer, renderer, camera, world);
		cout << "VAR : " << ospGetVariance(framebuffer) << endl;
	};

	// DENOISE
	OSPImageOperation dnz = ospNewImageOperation("denoiser");
	ospCommit(dnz);

	OSPData ops = ospNewSharedData1D(&dnz, OSP_IMAGE_OPERATION, 1);

	ospSetObject(framebuffer, "imageOperation", ops);
	ospCommit(framebuffer);

	ospRenderFrameBlocking(framebuffer, renderer, camera, world);


	 // CONVERT

	float* RGBA = (float*)ospMapFrameBuffer(framebuffer);
	unsigned char* rgba = new unsigned char[width*height * 4];

	for (int i = 0; i < width*height * 4; i++)
	{
		if (RGBA[i] < 0.0f)
		{
			rgba[i] = 0;
			continue;
		};
		if (RGBA[i] > 1.0f)
		{
			rgba[i] = 255;
			continue;
		};
		rgba[i] = 255 * pow(RGBA[i],0.46);
	};

	// SAVE
	const char *filename = "f:/temp/out.png";
	const int channels = 4; // RGB
	ImageOutput* out = ImageOutput::create(filename);
	if (!out) return -1;
	ImageSpec spec(width, height, channels, TypeDesc::UINT8);
	out->open(filename, spec);
	out->write_image(TypeDesc::UINT8, rgba);
	out->close();

	// EXIT

	system(filename);

	return 0;
}