// CRT
#include <iostream>
using namespace std;

// PXR
#include <pxr/pxr.h>
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>

PXR_NAMESPACE_USING_DIRECTIVE

// HDOSPRAY
#include "delegate.h"

//PXR_NAMESPACE_OPEN_SCOPE	

class HdEzraPlugin final : public HdRendererPlugin
{
public:
	HdEzraPlugin();
	virtual ~HdEzraPlugin();

	virtual HdRenderDelegate* CreateRenderDelegate() override;
	virtual HdRenderDelegate* CreateRenderDelegate(HdRenderSettingsMap const&) override;

	virtual void
		DeleteRenderDelegate(HdRenderDelegate* renderDelegate) override;

	virtual bool IsSupported() const override { return true; };

private:
	HdEzraPlugin(const HdEzraPlugin&) = delete;
	HdEzraPlugin& operator=(const HdEzraPlugin&) = delete;
};

TF_REGISTRY_FUNCTION(TfType)
{
	HdRendererPluginRegistry::Define<HdEzraPlugin>();
}

HdEzraPlugin::HdEzraPlugin()
{
	cout << "HDOSPRAY : RP : CREATE" << endl;
};

HdEzraPlugin::~HdEzraPlugin()
{
	cout << "HDOSPRAY : RP : DELETE" << endl;
};

HdRenderDelegate*
HdEzraPlugin::CreateRenderDelegate()
{
	cout << "HDOSPRAY : RP : ASK CREATE RD()..." << endl;
	return new HdOSPRayRenderDelegate();
}

HdRenderDelegate*
HdEzraPlugin::CreateRenderDelegate(
	HdRenderSettingsMap const& settingsMap)
{
	cout << "HDOSPRAY : RP : ASK CREATE RD(settingsMap)..." << endl;
	return new HdOSPRayRenderDelegate(settingsMap);
}

void HdEzraPlugin::DeleteRenderDelegate(HdRenderDelegate *renderDelegate)
{
	cout << "HDOSPRAY : PR : ASK DELETE RD..." << endl;
	if (renderDelegate == NULL) return;
	delete renderDelegate;
}

//PXR_NAMESPACE_CLOSE_SCOPE