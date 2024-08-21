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
enum WebcamNodes : int
{	// CPU nodes
	WebcamReader = 0,
	WebcamStream,
	Count
};

nosResult RegisterWebcamReader(nosNodeFunctions* function);
nosResult RegisterWebcamStream(nosNodeFunctions* function);

struct WebcamPluginFunctions : nos::PluginFunctions
{
	nosResult ExportNodeFunctions(size_t& outSize, nosNodeFunctions** outList) override
	{
		outSize = static_cast<size_t>(WebcamNodes::Count);
		if (!outList)
			return NOS_RESULT_SUCCESS;

		WebcamStreamManager::Start();

		NOS_RETURN_ON_FAILURE(RegisterWebcamReader(outList[(int)WebcamNodes::WebcamReader]))
		NOS_RETURN_ON_FAILURE(RegisterWebcamStream(outList[(int)WebcamNodes::WebcamStream]))
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
