/******************************************************************************
 * TelePi - remote streaming for your Raspberry Pi
 ******************************************************************************
 * encode.c
 *
 * Portions of this code are derived from hello_encode.c:
 * Copyright (c) 2012, Broadcom Europe Ltd
 * Copyright (c) 2012, Kalle Vahlman <zuh@iki>
 *                     Tuomas Kulve <tuomas@kulve.fi>
 */

#include <stdio.h>

#include "telepi.h"
#include "ilclient.h"

OMX_PARAM_PORTDEFINITIONTYPE def;
COMPONENT_T *list[5];
ILCLIENT_T *client;

static void
print_def(OMX_PARAM_PORTDEFINITIONTYPE def)
{
   fprintf(stderr, "Port %u: %s %u/%u %u %u %s,%s,%s %ux%u %ux%u @%u %u\n",
	  def.nPortIndex,
	  def.eDir == OMX_DirInput ? "in" : "out",
	  def.nBufferCountActual,
	  def.nBufferCountMin,
	  def.nBufferSize,
	  def.nBufferAlignment,
	  def.bEnabled ? "enabled" : "disabled",
	  def.bPopulated ? "populated" : "not pop.",
	  def.bBuffersContiguous ? "contig." : "not cont.",
	  def.format.video.nFrameWidth,
	  def.format.video.nFrameHeight,
	  def.format.video.nStride,
	  def.format.video.nSliceHeight,
	  def.format.video.xFramerate, def.format.video.eColorFormat);
}

bool encode_init(COMPONENT_T **video_encode)
{
	OMX_ERRORTYPE omx_return;

	memset(list, 0, sizeof(list));

	if ((client = ilclient_init()) == NULL)
	{
		return false;
	}

	if (OMX_Init() != OMX_ErrorNone)
	{
		ilclient_destroy(client);
		return false;
	}

	// create video_encode
	omx_return = ilclient_create_component(client, video_encode, "video_encode",
				ILCLIENT_DISABLE_ALL_PORTS |
				ILCLIENT_ENABLE_INPUT_BUFFERS |
				ILCLIENT_ENABLE_OUTPUT_BUFFERS);
	if (omx_return != 0)
	{
		fprintf(stderr, "ilclient_create_component() for video_encode failed with %x!\n", omx_return);
		return false;
	}

	list[0] = *video_encode;

	// get current settings of video_encode component from port 200
	memset(&def, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	def.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	def.nVersion.nVersion = OMX_VERSION;
	def.nPortIndex = 200;

	if (OMX_GetParameter(ILC_GET_HANDLE(*video_encode), OMX_IndexParamPortDefinition, &def) != OMX_ErrorNone)
	{
		fprintf(stderr, "%s:%d: OMX_GetParameter() for video_encode port 200 failed!\n", __FUNCTION__, __LINE__);
		return false;
	}

   print_def(def);

   return true;
}

bool encode_config_format(COMPONENT_T* handle, int32_t width, int32_t height, int32_t framerate)
{
	OMX_ERRORTYPE omx_return;

	// Port 200: in 1/1 115200 16 enabled,not pop.,not cont. 320x240 320x240 @1966080 20
	def.format.video.nFrameWidth = width;
	def.format.video.nFrameHeight = height;
	def.format.video.xFramerate = framerate << 16;
	def.format.video.nSliceHeight = def.format.video.nFrameHeight;
	def.format.video.nStride = def.format.video.nFrameWidth;
	def.format.video.eColorFormat = /* OMX_COLOR_FormatYUV420PackedPlanar; */ OMX_COLOR_Format24bitBGR888;

	print_def(def);

	omx_return = OMX_SetParameter(ILC_GET_HANDLE(handle), OMX_IndexParamPortDefinition, &def);
	if (omx_return != OMX_ErrorNone)
	{
		fprintf(stderr, "%s:%d: OMX_SetParameter() for video_encode port 200 failed with %x!\n", __FUNCTION__, __LINE__, omx_return);
		return false;
	}
}

bool encode_config_encoding(COMPONENT_T* handle, int32_t codec)
{
	OMX_ERRORTYPE omx_return;
	OMX_VIDEO_PARAM_PORTFORMATTYPE format;

	memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
	format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
	format.nVersion.nVersion = OMX_VERSION;
	format.nPortIndex = 201;
	format.eCompressionFormat = OMX_VIDEO_CodingAVC;

	fprintf(stderr, "OMX_SetParameter for video_encode:201...\n");
	omx_return = OMX_SetParameter(ILC_GET_HANDLE(handle), OMX_IndexParamVideoPortFormat, &format);
	if (omx_return != OMX_ErrorNone)
	{
		fprintf(stderr, "%s:%d: OMX_SetParameter() for video_encode port 201 failed with %x!\n", __FUNCTION__, __LINE__, omx_return);
		return false;
	}

	return true;
}

bool encode_config_bitrate(COMPONENT_T* handle, uint32_t bitrate)
{
	OMX_ERRORTYPE omx_return;
	OMX_VIDEO_PARAM_BITRATETYPE bitrateType;

	// set current bitrate to 1Mbit
	memset(&bitrateType, 0, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
	bitrateType.nSize = sizeof(OMX_VIDEO_PARAM_BITRATETYPE);
	bitrateType.nVersion.nVersion = OMX_VERSION;
	bitrateType.eControlRate = OMX_Video_ControlRateVariable;
	bitrateType.nTargetBitrate = bitrate;
	bitrateType.nPortIndex = 201;

	omx_return = OMX_SetParameter(ILC_GET_HANDLE(handle), OMX_IndexParamVideoBitrate, &bitrateType);
	if (omx_return != OMX_ErrorNone)
	{
		fprintf(stderr, "%s:%d: OMX_SetParameter() for bitrate for video_encode port 201 failed with %x!\n", __FUNCTION__, __LINE__, omx_return);
		return false;
	}
   
	// get current bitrate
	memset(&bitrateType, 0, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
	bitrateType.nSize = sizeof(OMX_VIDEO_PARAM_BITRATETYPE);
	bitrateType.nVersion.nVersion = OMX_VERSION;
	bitrateType.nPortIndex = 201;

	if (OMX_GetParameter(ILC_GET_HANDLE(handle), OMX_IndexParamVideoBitrate, &bitrateType) != OMX_ErrorNone)
	{
		fprintf(stderr, "%s:%d: OMX_GetParameter() for video_encode for bitrate port 201 failed!\n", __FUNCTION__, __LINE__);
		return false;
	}

	fprintf(stderr, "Current Bitrate=%u\n",bitrateType.nTargetBitrate);

	return true;
}

bool encode_config_low_latency(COMPONENT_T* handle, bool low_latency)
{
	OMX_ERRORTYPE omx_return;
	OMX_CONFIG_BOOLEANTYPE booleanType;

	// set current bitrate to 1Mbit
	memset(&booleanType, 0, sizeof(OMX_CONFIG_BOOLEANTYPE));
	booleanType.nSize = sizeof(OMX_CONFIG_BOOLEANTYPE);
	booleanType.nVersion.nVersion = OMX_VERSION;
	booleanType.bEnabled = OMX_TRUE;
	//booleanType.nPortIndex = 201;

/*	
    if((r = OMX_SetParameter(ctx.camera, OMX_IndexConfigPortCapturing, &capture)) != OMX_ErrorNone) {
        omx_die(r, "Failed to switch on capture on camera video output port 71");
    }
    */

	omx_return = OMX_SetConfig(ILC_GET_HANDLE(handle), OMX_IndexConfigBrcmVideoH264LowLatency, &booleanType);
	if (omx_return != OMX_ErrorNone)
	{
		fprintf(stderr, "%s:%d: OMX_SetConfig() for low latency for video_encode port 201 failed with %x!\n", __FUNCTION__, __LINE__, omx_return);
		return false;
	}
   
	// get current bitrate
	memset(&booleanType, 0, sizeof(OMX_CONFIG_BOOLEANTYPE));
	booleanType.nSize = sizeof(OMX_CONFIG_BOOLEANTYPE);
	booleanType.nVersion.nVersion = OMX_VERSION;
	//booleanType.nPortIndex = 201;

	if (OMX_GetConfig(ILC_GET_HANDLE(handle), OMX_IndexConfigBrcmVideoH264LowLatency, &booleanType) != OMX_ErrorNone)
	{
		fprintf(stderr, "%s:%d: OMX_GetConfig() for video_encode for bitrate port 201 failed!\n", __FUNCTION__, __LINE__);
		return false;
	}

	fprintf(stderr, "Low latency=%u\n", booleanType.bEnabled);

	return true;
}

bool encode_config_activate(COMPONENT_T* handle)
{
	fprintf(stderr, "encode to idle...\n");

	if (ilclient_change_component_state(handle, OMX_StateIdle) == -1)
	{
		fprintf(stderr, "%s:%d: ilclient_change_component_state(video_encode, OMX_StateIdle) failed", __FUNCTION__, __LINE__);
		return false;
	}

	fprintf(stderr, "enabling port buffers for 200...\n");
	if (ilclient_enable_port_buffers(handle, 200, NULL, NULL, NULL) != 0)
	{
		fprintf(stderr, "enabling port buffers for 200 failed!\n");
		return false;
	}

	fprintf(stderr, "enabling port buffers for 201...\n");
	if (ilclient_enable_port_buffers(handle, 201, NULL, NULL, NULL) != 0)
	{
		fprintf(stderr, "enabling port buffers for 201 failed!\n");
		return false;
	}

	fprintf(stderr, "encode to executing...\n");
	ilclient_change_component_state(handle, OMX_StateExecuting);

	return true;
}

void encode_deinit(COMPONENT_T* handle)
{
   fprintf(stderr, "disabling port buffers for 200 and 201...\n");
   ilclient_disable_port_buffers(handle, 200, NULL, NULL, NULL);
   ilclient_disable_port_buffers(handle, 201, NULL, NULL, NULL);

   ilclient_state_transition(list, OMX_StateIdle);
   ilclient_state_transition(list, OMX_StateLoaded);

   ilclient_cleanup_components(list);

   OMX_Deinit();

   ilclient_destroy(client);
}