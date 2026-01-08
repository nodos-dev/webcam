#include <Nodos/Plugin.hpp>
#include <nosSysVulkan/Helpers.hpp>

#include "WebcamStream.h"
#include <Nodos/Utils/Stopwatch.hpp>
#include "nosWebcam/Webcam_generated.h"
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

	nosResult OnCreate(nosFbNodePtr node) override
	{
		AddPinValueWatcher<float>(NSN_FrameRate, [this](const float* newVal, std::optional<const float*> oldValue)
			{
				FrameRate = *newVal;
			});
		AddPinValueWatcher<nos::fb::vec2u>(NSN_Resolution, [this](const nos::fb::vec2u* newVal, std::optional<const nos::fb::vec2u*> oldValue)
			{
				Resolution = *newVal;
			});
		AddPinValueWatcher<WebcamTextureFormat>(NSN_Format, [this](const WebcamTextureFormat* newVal, std::optional<const WebcamTextureFormat*> oldValue)
			{
				Format = *newVal;
			});
		RecreateCamera();
		return NOS_RESULT_SUCCESS;
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

	void OnPinValueChanged(nos::Name pinName, uuid const& pinId, nosBuffer value) override
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

	nosResult ExecuteNode(nos::NodeExecuteParams const& params) override
	{
		if(!CamHandle || IsCameraDifferent())
			return NOS_RESULT_FAILED;
		unsigned int outBufferSize = Resolution.x() * Resolution.y() * getFormatSizePerPixel(Format);
		
		auto inputBuffer = params.GetPinObject<sys::vulkan::Buffer>(NSN_Source);
		auto inputBufferInfo = sys::vulkan::GetResourceInfo(inputBuffer);
		if (!inputBuffer || !inputBufferInfo)
		{
			nosEngine.LogE("WebcamWriter: Input buffer is null");
			return NOS_RESULT_FAILED;
		}

		if (!inputBufferInfo->Size < outBufferSize)
		{
			nosEngine.LogE("WebcamWriter: Input buffer size is smaller than required");
			return NOS_RESULT_FAILED;
		}

		auto buffer = nosVulkan->Map(inputBuffer);
		scSendFrame(reinterpret_cast<scCamera>(CamHandle), buffer);

		SendScheduleRequest(1);
		return NOS_RESULT_SUCCESS;
	}

	void OnPathStart() override
	{
		if (!CamHandle)
			RecreateCamera();
		if (ActiveNodeId != NodeId && ActiveNodeId != nos::fb::UUID()) {
			char uri[256] = {};
			size_t uriLength = 0;
			auto res = nosEngine.GetItemUri(nos::uuid(ActiveNodeId), uri, &uriLength);
			if (res != NOS_RESULT_SUCCESS || uriLength > 255)
				return;
			std::string finalUri = uriLength ? uri : "";
			auto detailsStr = std::string("Another [WebcamWriter node](") + finalUri + ") is already active";
			SetNodeStatusMessages({ {{}, "WebcamWriter activation failed", nos::fb::NodeStatusMessageType::FAILURE, detailsStr, 5, true, true} });
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
		if (!IS_SOFTCAM_DRIVER_FOUND)
		{
			SetNodeStatusMessages({ {{}, "Driver not found", nos::fb::NodeStatusMessageType::FAILURE, "Softcam driver is not installed on your system. Install it according to the directives in README.md", 10, true, true}});
			return;
		}
		if (!IsCameraDifferent() && CamHandle)
			return;
		if(CamHandle)
			DestroyCamera();
		if (!Resolution.x() || !Resolution.y() || FrameRate < FLT_MIN || Format == WebcamTextureFormat::NONE) {
			static constexpr auto resError = "Resolution dimension can't be 0";
			static constexpr auto frameRateError = "FrameRate can't be 0";
			static constexpr auto formatError = "Format can't be NONE";
			auto detailsStr = (!Resolution.x() || !Resolution.y()) ? resError : (FrameRate < FLT_MIN) ? frameRateError : formatError;
			SetNodeStatusMessages({ {{}, "Invalid parameter for camera", nos::fb::NodeStatusMessageType::FAILURE, detailsStr, 5, true, true}});
			return;
		}
		if (Format != WebcamTextureFormat::BGR24) {
			SetNodeStatusMessages({ {{}, "Not tested format", nos::fb::NodeStatusMessageType::WARNING, "", 5, true, false}});
		}

		CamHandle = scCreateCamera(Resolution.x(), Resolution.y(), FrameRate, GetSoftcamFormatFromWebcamFormat(Format));
		if (!CamHandle)
		{
			SetNodeStatusMessages({ {{}, "Camera creation failed", nos::fb::NodeStatusMessageType::FAILURE, "Driver failed to create the camera", 5, true, true}});
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
