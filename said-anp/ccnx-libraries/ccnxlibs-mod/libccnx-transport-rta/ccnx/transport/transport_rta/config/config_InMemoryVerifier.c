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

//
//  config_InMemoryVerifier.c
//  Libccnx
//
//

#include <config.h>
#include <stdio.h>
#include "config_InMemoryVerifier.h"

#include <ccnx/transport/transport_rta/core/components.h>

#include <LongBow/runtime.h>

static const char name[] = "InMemoryVerifier";

/**
 * Generates:
 *
 * { "SIGNER" : "InMemoryVerifier",
 * }
 */
CCNxConnectionConfig *
inMemoryVerifier_ConnectionConfig(CCNxConnectionConfig *connConfig)
{
    PARCJSONValue *value = parcJSONValue_CreateFromNULL();
    CCNxConnectionConfig *result = ccnxConnectionConfig_Add(connConfig, inMemoryVerifier_GetName(), value);
    parcJSONValue_Release(&value);

    return result;
}

const char *
inMemoryVerifier_GetName(void)
{
    return name;
}
