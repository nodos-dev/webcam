#include <Nodos/PluginHelpers.hpp>
#include <nosVulkanSubsystem/Helpers.hpp>

#include "WebcamStream.h"
#include "nosUtil/Stopwatch.hpp"
#include "Webcam_generated.h"
#include <SenderAPI.h>

namespace nos::webcam
{
NOS_REGISTER_NAME(Source);
NOS_REGISTER_NAME(Resolution);
NOS_REGISTER_NAME(Format);
NOS_REGISTER_NAME(Run);
NOS_REGISTER_NAME_SPACED(FrameRate, "Frame Rate");

float getFormatSizePerPixel(WebcamTextureFormat format) {
	switch (format)
	{
	case WebcamTextureFormat::NV12:
		return 1.5f;
	case WebcamTextureFormat::YUY2:
		return 2.0f;
	case WebcamTextureFormat::BGR24:
		return 3.0f;
	default:
		return 0.0f;
	}
}

struct WebcamWriterNode : public NodeContext
{
	using NodeContext::NodeContext;

	// Camera's active properties
	static scCamera CamHandle;
	static nos::fb::UUID ActiveNodeId;
	static float ActiveFrameRate;
	static nos::fb::vec2u ActiveResolution;
	static WebcamTextureFormat ActiveFormat;

	// Node specifics
	float FrameRate;
	nos::fb::vec2u Resolution;
	WebcamTextureFormat Format;

	WebcamWriterNode(const nosFbNode* node) : nos::NodeContext(node) {
		AddPinValueWatcher(NSN_FrameRate, [this](nos::Buffer const& newVal, std::optional<nos::Buffer> oldValue)
			{
				FrameRate = *InterpretPinValue<float>(newVal);
			});
		AddPinValueWatcher(NSN_Resolution, [this](nos::Buffer const& newVal, std::optional<nos::Buffer> oldValue)
			{
				Resolution = *InterpretPinValue<nos::fb::vec2u>(newVal);
			});
		AddPinValueWatcher(NSN_Format, [this](nos::Buffer const& newVal, std::optional<nos::Buffer> oldValue)
			{
				Format = *InterpretPinValue<WebcamTextureFormat>(newVal);
			});
		RecreateCamera();
	}
	~WebcamWriterNode() {
		if (CamHandle)
			scDeleteCamera(CamHandle);
	}

	void GetScheduleInfo(nosScheduleInfo* out) override
	{
		*out = nosScheduleInfo{
			.Importance = 1,
			.DeltaSeconds = {1, 60},
			.Type = NOS_SCHEDULE_TYPE_ON_DEMAND,
		};

		if (!CamHandle)
			RecreateCamera();
	}
	int colorV = 0;

	bool IsCameraDifferent() {
		return ActiveFrameRate != FrameRate || ActiveResolution != Resolution || ActiveFormat != Format;
	}

	void OnPinValueChanged(nos::Name pinName, nosUUID pinId, nosBuffer value) override
	{
		if (pinName == NSN_Source || pinName == NSN_Run)
			return;
		if (pinName == NSN_FrameRate)
			FrameRate = *nos::Buffer(value).As<float>();
		if (pinName == NSN_Resolution)
			Resolution = *nos::Buffer(value).As<nos::fb::vec2u>();
		if (pinName == NSN_Format)
			Format = *nos::Buffer(value).As<WebcamTextureFormat>();

		if (ActiveNodeId == NodeId)
			RecreateCamera();
	}

	void OnPinDisconnected(nos::Name pinName) override
	{
		if (pinName == NSN_Source || pinName == NSN_Run)
			if(ActiveNodeId == NodeId)
				ActiveNodeId = {};
	}

	nosResult ExecuteNode(nosNodeExecuteParams* params) override
	{
		if(!CamHandle || IsCameraDifferent())
			return NOS_RESULT_FAILED;
		nos::NodeExecuteParams execParams(params);
		unsigned int outBufferSize = Resolution.x() * Resolution.y() * getFormatSizePerPixel(Format);
		
		nosResourceShareInfo inputBuffer{};
		for (size_t i = 0; i < params->PinCount; ++i)
		{
			auto& pin = params->Pins[i];
			if (pin.Name == NSN_Source)
				inputBuffer = vkss::ConvertToResourceInfo(*InterpretPinValue<sys::vulkan::Buffer>(*pin.Data));
		}

		if (!inputBuffer.Memory.Handle || inputBuffer.Memory.Size < outBufferSize)
		{
			nosEngine.LogE("WebcamOut: Invalid input buffer");
			return NOS_RESULT_FAILED;
		}

		auto buffer = nosVulkan->Map(&inputBuffer);
		scSendFrame(reinterpret_cast<scCamera>(CamHandle), buffer);

		nosScheduleNodeParams schedule{
			.NodeId = NodeId,
			.AddScheduleCount = 1
		};
		nosEngine.ScheduleNode(&schedule);
		return NOS_RESULT_SUCCESS;
	}

	void OnPathStart() override
	{
		if (!CamHandle)
			RecreateCamera();
		if (ActiveNodeId != NodeId && ActiveNodeId != nos::fb::UUID()) {
			SetNodeStatusMessage("Another WebcamOut node is already active", nos::fb::NodeStatusMessageType::FAILURE);
			return;
		}

		nosScheduleNodeParams schedule{ .NodeId = NodeId, .AddScheduleCount = 1 };
		nosEngine.ScheduleNode(&schedule);
		ActiveNodeId = NodeId;
		ClearNodeStatusMessages();
	}
	
	softcamTextureFormat GetSoftcamFormatFromWebcamFormat(WebcamTextureFormat format) {
		switch (format)
		{
		case WebcamTextureFormat::BGR24:
			return SOFTCAM_TEXTURE_FORMAT_BGR24;
		case WebcamTextureFormat::NV12:
			return SOFTCAM_TEXTURE_FORMAT_NV12;
		case WebcamTextureFormat::YUY2:
			return SOFTCAM_TEXTURE_FORMAT_YUY2;
		default:
			return SOFTCAM_TEXTURE_FORMAT_UNKNOWN;
		}
	}
	
	void RecreateCamera() {
		if (!IsCameraDifferent() && CamHandle)
			return;
		if(CamHandle)
			DestroyCamera();
		if (!Resolution.x() || !Resolution.y() || FrameRate < FLT_MIN || Format == WebcamTextureFormat::NONE) {
			SetNodeStatusMessage("Invalid parameter for camera", nos::fb::NodeStatusMessageType::FAILURE);
			return;
		}
		if (Format != WebcamTextureFormat::BGR24) {
			SetNodeStatusMessage("Not tested format", nos::fb::NodeStatusMessageType::WARNING);
		}

		CamHandle = scCreateCamera(Resolution.x(), Resolution.y(), FrameRate, GetSoftcamFormatFromWebcamFormat(Format));
		if (!CamHandle)
		{
			SetNodeStatusMessage("Camera creation failed", nos::fb::NodeStatusMessageType::FAILURE);
			return;
		}
		else if (Format == WebcamTextureFormat::BGR24)
			ClearNodeStatusMessages();

		ActiveResolution = Resolution;
		ActiveFrameRate = FrameRate;
		ActiveFormat = Format;
		nosEngine.SendPathRestart(NodeId);
	}
	void DestroyCamera() {
		scDeleteCamera(CamHandle);
		CamHandle = nullptr;
	}
};
scCamera WebcamWriterNode::CamHandle = nullptr;
nos::fb::UUID WebcamWriterNode::ActiveNodeId = {};
float WebcamWriterNode::ActiveFrameRate = 0.0f;
nos::fb::vec2u WebcamWriterNode::ActiveResolution = {0, 0};
WebcamTextureFormat WebcamWriterNode::ActiveFormat = WebcamTextureFormat::NONE;
nosResult RegisterWebcamWriter(nosNodeFunctions* outFunc)
{
	NOS_BIND_NODE_CLASS(NOS_NAME_STATIC("nos.webcam.WebcamWriter"), nos::webcam::WebcamWriterNode, outFunc);
	return NOS_RESULT_SUCCESS;
}
}; // namespace nos::webcam
