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

#include <stdbool.h>
#include <stdint.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#include <LongBow/runtime.h>

#include <parc/algol/parc_Memory.h>

#include <ccnx/forwarder/metis/config/metisControl_CacheStore.h>

#include <ccnx/api/control/cpi_ManageLinks.h>
#include <ccnx/api/control/cpi_Acks.h>
#include <ccnx/api/control/cpi_Forwarding.h>

static MetisCommandReturn _metisControlCacheStore_Execute(MetisCommandParser *parser, MetisCommandOps *ops, PARCList *args);
static MetisCommandReturn _metisControlCacheStore_HelpExecute(MetisCommandParser *parser, MetisCommandOps *ops, PARCList *args);

static const char *_commandCacheStore = "cache store";
static const char *_commandCacheStoreHelp = "help cache store";

// ====================================================

MetisCommandOps *
metisControlCacheStore_Create(MetisControlState *state)
{
    return metisCommandOps_Create(state, _commandCacheStore, NULL, _metisControlCacheStore_Execute, metisCommandOps_Destroy);
}

MetisCommandOps *
metisControlCacheStore_HelpCreate(MetisControlState *state)
{
    return metisCommandOps_Create(state, _commandCacheStoreHelp, NULL, _metisControlCacheStore_HelpExecute, metisCommandOps_Destroy);
}

// ====================================================

static MetisCommandReturn
_metisControlCacheStore_HelpExecute(MetisCommandParser *parser, MetisCommandOps *ops, PARCList *args)
{
    printf("cache store [on|off]\n");
    printf("\n");

    return MetisCommandReturn_Success;
}

static MetisCommandReturn
_metisControlCacheStore_Execute(MetisCommandParser *parser, MetisCommandOps *ops, PARCList *args)
{
    if (parcList_Size(args) != 3) {
        _metisControlCacheStore_HelpExecute(parser, ops, args);
        return MetisCommandReturn_Failure;
    }

    CCNxControl *cacheRequest;
    if (strcmp(parcList_GetAtIndex(args, 2), "on") == 0) {
        cacheRequest = ccnxControl_CreateCacheStoreRequest(true);
    } else if (strcmp(parcList_GetAtIndex(args, 2), "off") == 0) {
        cacheRequest = ccnxControl_CreateCacheStoreRequest(false);
    } else  {
        _metisControlCacheStore_HelpExecute(parser, ops, args);
        return MetisCommandReturn_Failure;
    }

    MetisControlState *state = ops->closure;
    CCNxMetaMessage *message = ccnxMetaMessage_CreateFromControl(cacheRequest);
    CCNxMetaMessage *rawResponse = metisControlState_WriteRead(state, message);
    ccnxMetaMessage_Release(&message);

    CCNxControl *response = ccnxMetaMessage_GetControl(rawResponse);

    if (metisControlState_GetDebug(state)) {
        char *str = parcJSON_ToString(ccnxControl_GetJson(response));
        printf("reponse:\n%s\n", str);
        parcMemory_Deallocate((void **) &str);
    }

    if (!cpiAcks_IsAck(ccnxControl_GetJson(response))) {
        printf("command failed:\n");
    }

    ccnxMetaMessage_Release(&rawResponse);

    return MetisCommandReturn_Success;
}
