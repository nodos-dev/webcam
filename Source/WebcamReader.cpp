#include <Nodos/PluginHelpers.hpp>
#include <nosVulkanSubsystem/Helpers.hpp>

#include "WebcamStream.h"
#include "nosUtil/Stopwatch.hpp"
#include "Webcam_generated.h"

namespace nos::webcam
{
NOS_REGISTER_NAME(StreamInfo);
NOS_REGISTER_NAME(BufferToWrite);
NOS_REGISTER_NAME(Output);

struct WebcamReaderNode : public NodeContext
{
	using NodeContext::NodeContext;
	// Execution
	virtual nosResult ExecuteNode(nosNodeExecuteParams* params)
	{
		nos::NodeExecuteParams execParams(params);

		auto* streamInfo = execParams.GetPinData<webcam::WebcamStreamInfo>(NSN_StreamInfo);
		if (!streamInfo || !streamInfo->id())
			return NOS_RESULT_FAILED;
		
		auto stream = WebcamStreamManager::GetInstance().GetStream(*streamInfo->id());
		if(!stream)
			return NOS_RESULT_FAILED;
		auto sample = stream->ReadSample();
		// First sample is not valid, try again
		if (sample.Size == 0)
		{
			sample = stream->ReadSample();
			if (sample.Size == 0)
				return NOS_RESULT_FAILED;
		}
		nosResourceShareInfo bufToWrite = vkss::ConvertToResourceInfo(*execParams.GetPinData<nos::sys::vulkan::Buffer>(NSN_BufferToWrite));
		if (bufToWrite.Info.Buffer.Size != sample.Size)
			nosEngine.LogE("Buffer size mismatch!");
		
		uint8_t* mapped = nosVulkan->Map(&bufToWrite);
		if (mapped == nullptr) 
		{
			nosEngine.LogE("Failed to map buffer!");
			return NOS_RESULT_FAILED;
		}
		
		memcpy(mapped, sample.Data, std::min(uint32_t(sample.Size), bufToWrite.Info.Buffer.Size));
		nosEngine.SetPinValue(execParams[NSN_Output].Id, nos::Buffer::From(vkss::ConvertBufferInfo(bufToWrite)));
		return NOS_RESULT_SUCCESS;
	}
};
nosResult RegisterWebcamReader(nosNodeFunctions* outFunc)
{
	NOS_BIND_NODE_CLASS(NOS_NAME_STATIC("nos.webcam.WebcamReader"), nos::webcam::WebcamReaderNode, outFunc);
	return NOS_RESULT_SUCCESS;
}
}; // namespace nos::webcam
