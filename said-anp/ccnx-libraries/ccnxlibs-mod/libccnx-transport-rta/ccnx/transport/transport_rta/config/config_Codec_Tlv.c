/*
 * Copyright (c) 2017 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <config.h>
#include <stdio.h>
#include "config_Codec_Tlv.h"
#include <LongBow/runtime.h>

#include <ccnx/transport/transport_rta/core/components.h>

//static const char param_SCHEMA[]  = "SCHEMA";
//static const char param_CODEC[] = "CODEC";
//static const int default_schema = 0;

/**
 * Generates:
 *
 * { "CODEC_TLV" : { } }
 */
CCNxStackConfig *
tlvCodec_ProtocolStackConfig(CCNxStackConfig *stackConfig)
{
    PARCJSONValue *value = parcJSONValue_CreateFromNULL();
    CCNxStackConfig *result = ccnxStackConfig_Add(stackConfig, tlvCodec_GetName(), value);
    parcJSONValue_Release(&value);

    return result;
}

/**
 * Generates:
 *
 * { "CODEC_TLV" : { } }
 */

CCNxConnectionConfig *
tlvCodec_ConnectionConfig(CCNxConnectionConfig *connectionConfig)
{
    PARCJSONValue *value = parcJSONValue_CreateFromNULL();
    CCNxConnectionConfig *result = ccnxConnectionConfig_Add(connectionConfig, tlvCodec_GetName(), value);
    parcJSONValue_Release(&value);
    return result;
}

const char *
tlvCodec_GetName(void)
{
    return RtaComponentNames[CODEC_TLV];
}
