// Copyright MediaZ Teknoloji A.S. All Rights Reserved.

#include <Nodos/PluginAPI.h>

#include <Builtins_generated.h>

#include <Nodos/PluginHelpers.hpp>

#include <nosVulkanSubsystem/nosVulkanSubsystem.h>
#include <nosVulkanSubsystem/Helpers.hpp>

#include "nosUtil/Stopwatch.hpp"
#include "WebcamStream.h"
#include "softcam.h"

NOS_INIT_WITH_MIN_REQUIRED_MINOR(0)
NOS_VULKAN_INIT();

NOS_BEGIN_IMPORT_DEPS()
	NOS_VULKAN_IMPORT()
NOS_END_IMPORT_DEPS()

namespace nos::webcam
{
bool IS_SOFTCAM_DRIVER_FOUND = false;
enum WebcamNodes : int
{	// CPU nodes
    WebcamReader = 0,
    WebcamStream,
    WebcamWriter,
    Count
};

nosResult RegisterWebcamReader(nosNodeFunctions* function);
nosResult RegisterWebcamStream(nosNodeFunctions* function);
nosResult RegisterWebcamWriter(nosNodeFunctions* function);

static constexpr char WARNING_FAILED_TO_FIND_DRIVER[] = "Failed to find Softcam driver for WebcamWriter node. Webcam output feature won't work.";
bool CheckSoftcamDriver() {
    // Initialize COM library
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM: " << std::hex << hr << std::endl;
        return hr;
    }

    // Replace with your driver's CLSID and IID
    CLSID clsidDriver = scGetCameraDriverClassID();  // Define your CLSID here

    // Try to create an instance of the COM object
    IUnknown* pUnknown = nullptr;
    hr = CoCreateInstance(clsidDriver, nullptr, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&pUnknown);
    if (FAILED(hr)) {
        if (hr == REGDB_E_CLASSNOTREG) {
            std::cerr << "The COM driver is not registered." << std::endl;
        }
        else {
            std::cerr << "Failed to create COM instance: " << std::hex << hr << std::endl;
        }
        CoUninitialize();
        return false;
    }

    // Release the base COM object
    pUnknown->Release();

    // Uninitialize COM library
    CoUninitialize();
    return true;
}

struct WebcamPluginFunctions : nos::PluginFunctions
{
	nosResult ExportNodeFunctions(size_t& outSize, nosNodeFunctions** outList) override
	{
		outSize = static_cast<size_t>(WebcamNodes::Count);
		if (!outList)
			return NOS_RESULT_SUCCESS;

        IS_SOFTCAM_DRIVER_FOUND = CheckSoftcamDriver();
        if (!IS_SOFTCAM_DRIVER_FOUND) {
            nosModuleStatusMessage mes;
            mes.Message = WARNING_FAILED_TO_FIND_DRIVER;
            mes.MessageType = NOS_MODULE_STATUS_MESSAGE_TYPE_WARNING;
            mes.ModuleId = nosEngine.Module->Id;
            mes.UpdateType = NOS_MODULE_STATUS_MESSAGE_UPDATE_TYPE_APPEND;
            nosEngine.SendModuleStatusMessageUpdate(&mes);
        }

		WebcamStreamManager::Start();

		NOS_RETURN_ON_FAILURE(RegisterWebcamReader(outList[(int)WebcamNodes::WebcamReader]))
		NOS_RETURN_ON_FAILURE(RegisterWebcamStream(outList[(int)WebcamNodes::WebcamStream]))
        NOS_RETURN_ON_FAILURE(RegisterWebcamWriter(outList[(int)WebcamNodes::WebcamWriter]))
		return NOS_RESULT_SUCCESS;
	}

	nosResult OnPreUnloadPlugin() override
	{
		WebcamStreamManager::Stop();
		return NOS_RESULT_SUCCESS;
	}
};

NOS_EXPORT_PLUGIN_FUNCTIONS(WebcamPluginFunctions)
} // namespace nos::test
