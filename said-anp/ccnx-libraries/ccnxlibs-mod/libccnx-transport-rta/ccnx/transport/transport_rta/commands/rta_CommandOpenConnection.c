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

/**
 *
 * Implements the RtaCommandOpenConnection object which signals to RTA Framework to open a new connection
 * with the given configuration.
 */
#include <config.h>

#include <LongBow/runtime.h>

#include <stdio.h>

#include <parc/algol/parc_Memory.h>
#include <parc/algol/parc_Object.h>

#include <ccnx/transport/transport_rta/commands/rta_CommandOpenConnection.h>

struct rta_command_openconnection {
    int stackId;
    int apiNotifierFd;
    int transportNotifierFd;
    PARCJSON *config;
};

// ======= Private API

static void
_rtaCommandOpenConnection_Destroy(RtaCommandOpenConnection **openConnectionPtr)
{
    RtaCommandOpenConnection *openConnection = *openConnectionPtr;
    if (openConnection->config != NULL) {
        parcJSON_Release(&openConnection->config);
    }
}

parcObject_ExtendPARCObject(RtaCommandOpenConnection, _rtaCommandOpenConnection_Destroy,
                            NULL, NULL, NULL, NULL, NULL, NULL);

parcObject_ImplementAcquire(rtaCommandOpenConnection, RtaCommandOpenConnection);

parcObject_ImplementRelease(rtaCommandOpenConnection, RtaCommandOpenConnection);

// ======= Public API

RtaCommandOpenConnection *
rtaCommandOpenConnection_Create(int stackId, int apiNotifierFd, int transportNotifierFd, const PARCJSON *config)
{
    RtaCommandOpenConnection *openConnection = parcObject_CreateInstance(RtaCommandOpenConnection);
    openConnection->stackId = stackId;
    openConnection->apiNotifierFd = apiNotifierFd;
    openConnection->transportNotifierFd = transportNotifierFd;
    openConnection->config = parcJSON_Copy(config);
    return openConnection;
}

int
rtaCommandOpenConnection_GetApiNotifierFd(const RtaCommandOpenConnection *openConnection)
{
    assertNotNull(openConnection, "Parameter openConnection must be non-null");
    return openConnection->apiNotifierFd;
}

int
rtaCommandOpenConnection_GetStackId(const RtaCommandOpenConnection *openConnection)
{
    assertNotNull(openConnection, "Parameter openConnection must be non-null");
    return openConnection->stackId;
}

int
rtaCommandOpenConnection_GetTransportNotifierFd(const RtaCommandOpenConnection *openConnection)
{
    assertNotNull(openConnection, "Parameter openConnection must be non-null");
    return openConnection->transportNotifierFd;
}

PARCJSON *
rtaCommandOpenConnection_GetConfig(const RtaCommandOpenConnection *openConnection)
{
    assertNotNull(openConnection, "Parameter openConnection must be non-null");
    return openConnection->config;
}
