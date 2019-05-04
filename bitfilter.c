/*
 * Copyright (C) 2016 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#define BIT_URI "http://gareus.org/oss/lv2/bitfilter"

typedef enum {
	BIT_IN,
	BIT_OUT,
	BIT_MODE,
} PortIndex;

typedef struct {
	float* mode;
	float* bits[32];
	float* input;
	float* output;
} BitFilter;

static void
run(LV2_Handle instance, uint32_t n_samples)
{
	BitFilter* self = (BitFilter*)instance;
	uint32_t mask = 0;
	for (int i = 0; i < 32; ++i) {
		if (*(self->bits[i]) > 0) mask |= 1<<i;
	}

	if (*self->mode > 0.5) {
		mask |= 0x7f800000;
		for (uint32_t i = 0; i < n_samples; ++i) {
			float *sample = self->input + i;
			uint32_t value = *((uint32_t *) sample);
			//uint32_t exp = (value & 0x7f800000) >> 23;
			//int sign = (value & 0x80000000) ? -1 : 1;
			uint32_t newsample = value & mask;
			self->output[i] = *((float *) &newsample);
		}
	} else {
		for (uint32_t i = 0; i < n_samples; ++i) {
			float sample = self->input[i];
			if (sample > 1.f) {
				self->output[i] = 1.0;
			} else if (sample < -1.f) {
				self->output[i] = -1.0;
			}
			int32_t ival = (sample) * 2147483648.f;
			ival &= mask;
			self->output[i] = (ival / 2147483648.f);
		}
	}
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	BitFilter* self = (BitFilter*)calloc(1, sizeof(BitFilter));
	if (!self) return NULL;
	return (LV2_Handle)self;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	BitFilter* self = (BitFilter*)instance;

	switch ((PortIndex)port) {
	case BIT_IN:
		self->input = data;
		break;
	case BIT_OUT:
		self->output = data;
		break;
	case BIT_MODE:
		self->mode = data;
		break;
	default:
		if (port > 2 && port < 35) {
			self->bits[port - 3] = data;
		}
		break;
	}
}

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static const void*
extension_data(const char* uri)
{
	return NULL;
}

static const LV2_Descriptor descriptor = {
	BIT_URI,
	instantiate,
	connect_port,
	NULL,
	run,
	NULL,
	cleanup,
	extension_data
};

#undef LV2_SYMBOL_EXPORT
#ifdef _WIN32
#    define LV2_SYMBOL_EXPORT __declspec(dllexport)
#else
#    define LV2_SYMBOL_EXPORT  __attribute__ ((visibility ("default")))
#endif
LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return NULL;
	}
}
