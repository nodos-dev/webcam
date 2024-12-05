// Copyright MediaZ Teknoloji A.S. All Rights Reserved.

#include <Nodos/PluginAPI.h>

#include <Builtins_generated.h>

#include <Nodos/PluginHelpers.hpp>

#include <nosVulkanSubsystem/nosVulkanSubsystem.h>
#include <nosVulkanSubsystem/Helpers.hpp>

#include "nosUtil/Stopwatch.hpp"

#include "WebcamStream.h"

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

static constexpr char WARNING_FAILED_TO_FIND_DRIVER[] = "Failed to find Softcam driver for WebcamWriter node";
bool CheckSoftcamDriver() {
    const char* command = R"(reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID" /s /f "softcam.dll")";
    char buffer[256];
    std::string output;

    nosModuleStatusMessage mes;
    mes.Message = WARNING_FAILED_TO_FIND_DRIVER;
    mes.MessageType = nosModuleStatusMessageType::NOS_MODULE_STATUS_MESSAGE_TYPE_WARNING;
    mes.ModuleId = nosEngine.Module->Id;
    mes.UpdateType = nosModuleStatusMessageUpdateType::NOS_MODULE_STATUS_MESSAGE_UPDATE_TYPE_APPEND;


    // Open a pipe to execute the command
    FILE* pipe = _popen(command, "r");
    if (!pipe) {
        nosEngine.SendModuleStatusMessageUpdate(&mes);
        nosEngine.LogE("Failed to execute command");
        return false;
    }

    // Read the output of the command
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;  // Append the output to the string
    }

    // Close the pipe
    int exitStatus = _pclose(pipe);

    // Check if the output contains any results
    if (output.find("End of search: 0 match(es) found.") != std::string::npos) {
        nosEngine.SendModuleStatusMessageUpdate(&mes);
        nosEngine.LogDE(output.c_str(), "No results found for 'softcam.dll'. Please install the driver as written in README.md of the plugin");
        return false;
    }
    else {
        nosEngine.LogDI(output.c_str(), "Results found for 'softcam.dll'");
        return true;
    }
}

struct WebcamPluginFunctions : nos::PluginFunctions
{
	nosResult ExportNodeFunctions(size_t& outSize, nosNodeFunctions** outList) override
	{
		outSize = static_cast<size_t>(WebcamNodes::Count);
		if (!outList)
			return NOS_RESULT_SUCCESS;

        IS_SOFTCAM_DRIVER_FOUND = CheckSoftcamDriver();
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
