#ifndef TELEPI_H
#define TELEPI_H

#include <stdbool.h>
#include <stdint.h>

#include "ilclient.h"

bool encode_init(COMPONENT_T **video_encode);
bool encode_config_format(COMPONENT_T* handle, int32_t width, int32_t height, int32_t framerate);
bool encode_config_encoding(COMPONENT_T* handle, int32_t codec);
bool encode_config_bitrate(COMPONENT_T* handle, uint32_t bitrate);
bool encode_config_activate(COMPONENT_T* handle);
void encode_deinit(COMPONENT_T* handle);

bool encode_config_low_latency(COMPONENT_T* handle, bool low_latency);

#endif // TELEPI_H