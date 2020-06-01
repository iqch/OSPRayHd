// CRT
#include <iostream>
using namespace std;

// PXR

//#include <pxr/imaging/glf/glew.h>

#include <pxr/pxr.h>

//#include <pxr/imaging/glf/drawTarget.h>
//#include <pxr/imaging/glf/image.h>

#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hd/renderDelegate.h>

//#include <pxr/imaging/hf/pluginDesc.h>


//#include <pxr/imaging/hgi/hgi.h>


#include "pxr/base/gf/matrix4f.h"


//#include <pxr/base/tf/token.h>


#include <pxr/imaging/hd/unitTestDelegate.h>
//#include <pxr/imaging/hdSt/glConversions.h>
//#include <pxr/imaging/hdSt/unitTestGLDrawing.h>

#include <pxr/imaging/hdx/renderTask.h>

//#include <pxr/usdImaging/usdImaging/delegate.h>

//#include <pxr/imaging/hd/camera.h>
//#include <pxr/imaging/hd/renderBuffer.h>

PXR_NAMESPACE_USING_DIRECTIVE

//using namespace pxr;

// QT ?

int
main(int argc, char* argv[])
{
	HdRendererPluginRegistry& reg = HdRendererPluginRegistry::GetInstance();

	HfPluginDescVector plugins;
	reg.GetPluginDescs(&plugins);

	//vector<HdRendererPlugin*> plugs;

	//HdRendererPlugin* plugin = NULL;
	//for (HfPluginDesc& p : plugins)
	//{
	//	cout << p.id << endl;
	//	plugs.push_back(reg.GetRendererPlugin(TfToken(p.id)));
	//};

	HdRendererPlugin* plugin = reg.GetRendererPlugin(TfToken("HdEzraPlugin"));


	HdRenderDelegate* renderDelegate = plugin->CreateRenderDelegate();

	// Hydra initialization

	HdRenderIndex *renderIndex = HdRenderIndex::New(renderDelegate, {});
	
	//UsdImagingDelegate sceneDelegate(renderIndex, SdfPath::AbsoluteRootPath());

	HdUnitTestDelegate *sceneDelegate = new HdUnitTestDelegate(renderIndex,
		SdfPath::AbsoluteRootPath());

	sceneDelegate->AddCube(SdfPath("/MyCube1"), GfMatrix4f(1));

	SdfPath renderTask("/renderTask");
	sceneDelegate->AddTask<HdxRenderTask>(renderTask);
	sceneDelegate->UpdateTask(renderTask, HdTokens->params,	VtValue(HdxRenderTaskParams()));
	sceneDelegate->UpdateTask(renderTask,HdTokens->collection,
		VtValue(HdRprimCollection(HdTokens->geometry,HdReprSelector(HdReprTokens->refined))));

	HdEngine engine;
	HdTaskSharedPtrVector tasks = { renderIndex->GetTask(renderTask) };
	engine.Execute(renderIndex, &tasks);

	// Create your task graph
	//HdRprimCollection collection;
	//HdRenderPassSharedPtr renderPass(renderDelegate->CreateRenderPass(renderIndex, collection));
	//HdRenderPassStateSharedPtr renderPassState(renderDelegate->CreateRenderPassState());

	//HdTaskSharedPtr taskRender(new HdxRenderTask(&sceneDelegate, SdfPath("/")));

	//HdTaskSharedPtrVector tasks = { taskRender };
	// Populate scene graph and generate image
	//sceneDelegate.Populate();
	//engine.Execute(renderIndex, &tasks);
	// Change time causes invalidations, and generate image
	//sceneDelegate.SetTime(1);
	//engine.Execute(renderIndex, &tasks);


	plugin->DeleteRenderDelegate(renderDelegate);

	return EXIT_SUCCESS;

}


