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
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include <LongBow/runtime.h>

#include <parc/algol/parc_Memory.h>
#include <parc/algol/parc_HashMap.h>
#include <parc/algol/parc_Object.h>
#include <parc/algol/parc_Unsigned.h>

#include <ccnx/forwarder/metis/strategies/strategy_NexthopState.h>
#include <ccnx/forwarder/metis/strategies/strategy_LoadBalancer.h>



static void             _strategyLoadBalancer_ReceiveObject(MetisStrategyImpl *strategy, const MetisNumberSet *egressId, const MetisMessage *objectMessage, MetisTicks rtt);
static void             _strategyLoadBalancer_OnTimeout(MetisStrategyImpl *strategy, const MetisNumberSet *egressId);
static MetisNumberSet *_strategyLoadBalancer_LookupNexthop(MetisStrategyImpl *strategy, const MetisMessage *interestMessage);
static MetisNumberSet *_strategyLoadBalancer_ReturnNexthops(MetisStrategyImpl *strategy);
static unsigned         _strategyLoadBalancer_CountNexthops(MetisStrategyImpl *strategy);
static void             _strategyLoadBalancer_AddNexthop(MetisStrategyImpl *strategy, CPIRouteEntry *route);
static void             _strategyLoadBalancer_RemoveNexthop(MetisStrategyImpl *strategy, CPIRouteEntry *route);
static void             _strategyLoadBalancer_ImplDestroy(MetisStrategyImpl **strategyPtr);
static const char *_strategyLoadBalancer_GetStrategy(MetisStrategyImpl *strategy);

static MetisStrategyImpl _template = {
    .context        = NULL,
    .receiveObject  = &_strategyLoadBalancer_ReceiveObject,
    .onTimeout      = &_strategyLoadBalancer_OnTimeout,
    .lookupNexthop  = &_strategyLoadBalancer_LookupNexthop,
    .returnNexthops = &_strategyLoadBalancer_ReturnNexthops,
    .countNexthops  = &_strategyLoadBalancer_CountNexthops,
    .addNexthop     = &_strategyLoadBalancer_AddNexthop,
    .removeNexthop  = &_strategyLoadBalancer_RemoveNexthop,
    .destroy        = &_strategyLoadBalancer_ImplDestroy,
    .getStrategy    = &_strategyLoadBalancer_GetStrategy,
};

struct strategy_load_balancer;
typedef struct strategy_load_balancer StrategyLoadBalancer;


struct strategy_load_balancer {
    double weights_sum;
    //hash map from connectionId to StrategyNexthopState
    PARCHashMap *strategy_state;
    MetisNumberSet *nexthops;
};

MetisStrategyImpl *
strategyLoadBalancer_Create()
{
    StrategyLoadBalancer *strategy = parcMemory_AllocateAndClear(sizeof(StrategyLoadBalancer));
    assertNotNull(strategy, "parcMemory_AllocateAndClear(%zu) returned NULL", sizeof(StrategyLoadBalancer));

    strategy->weights_sum = 0.0;
    strategy->strategy_state = parcHashMap_Create();
    strategy->nexthops = metisNumberSet_Create();
    srand(time(NULL));

    MetisStrategyImpl *impl = parcMemory_AllocateAndClear(sizeof(MetisStrategyImpl));
    assertNotNull(impl, "parcMemory_AllocateAndClear(%zu) returned NULL", sizeof(MetisStrategyImpl));
    memcpy(impl, &_template, sizeof(MetisStrategyImpl));
    impl->context = strategy;

    return impl;
}

// =======================================================
// Dispatch API


static const char*
_strategyLoadBalancer_GetStrategy(MetisStrategyImpl *strategy)
{
    return FWD_STRATEGY_LOADBALANCER;
}

static void
_update_Stats(StrategyLoadBalancer *strategy, StrategyNexthopState *state, bool inc)
{
    const double ALPHA = 0.9;
    double w = strategyNexthopState_GetWeight(state);
    strategy->weights_sum -= w;
    w = strategyNexthopState_UpdateState(state, inc, ALPHA);
    strategy->weights_sum += w;
}

static unsigned
_select_Nexthop(StrategyLoadBalancer *strategy)
{
    double rnd = (double) rand() / (double) RAND_MAX;
    double start_range = 0.0;

    PARCIterator *it = parcHashMap_CreateKeyIterator(strategy->strategy_state);

    unsigned nexthop = 100000;
    while (parcIterator_HasNext(it)) {
        PARCUnsigned *cid = parcIterator_Next(it);
        const StrategyNexthopState *elem = parcHashMap_Get(strategy->strategy_state, cid);

        double w = strategyNexthopState_GetWeight(elem);

        double prob = w / strategy->weights_sum;
        if ((rnd >= start_range) && (rnd < (start_range + prob))) {
            nexthop = parcUnsigned_GetUnsigned(cid);
            break;
        } else  {
            start_range += prob;
        }
    }

    parcIterator_Release(&it);

    //if no face is selected by the algorithm (for example because of a wrong round in the weights)
    //we may always select the last face here. Double check this!
    return nexthop;
}


static void
_strategyLoadBalancer_ReceiveObject(MetisStrategyImpl *strategy, const MetisNumberSet *egressId, const MetisMessage *objectMessage, MetisTicks rtt)
{
    _strategyLoadBalancer_OnTimeout(strategy, egressId);
}

static void
_strategyLoadBalancer_OnTimeout(MetisStrategyImpl *strategy, const MetisNumberSet *egressId)
{
    StrategyLoadBalancer *lb = (StrategyLoadBalancer *) strategy->context;

    for (unsigned i = 0; i < metisNumberSet_Length(egressId); i++) {
        unsigned outId = metisNumberSet_GetItem(egressId, i);
        PARCUnsigned *cid = parcUnsigned_Create(outId);

        const StrategyNexthopState *state = parcHashMap_Get(lb->strategy_state, cid);
        if (state != NULL) {
            _update_Stats(lb, (StrategyNexthopState *) state, false);
        } else  {
            //this may happen if we remove a face/route while downloading a file
            //we should ignore this timeout
        }
        parcUnsigned_Release(&cid);
    }
}


//ATTENTION!! This interface force us to create a MetisNumberSet which need to be delited somewhere
//The specification in the interface requires that this function never returns NULL. in case we have no output face we need to return an empty MetisNumberSet
static MetisNumberSet *
_strategyLoadBalancer_LookupNexthop(MetisStrategyImpl *strategy, const MetisMessage *interestMessage)
{
    StrategyLoadBalancer *lb = (StrategyLoadBalancer *) strategy->context;

    unsigned in_connection = metisMessage_GetIngressConnectionId(interestMessage);
    PARCUnsigned *in = parcUnsigned_Create(in_connection);

    unsigned mapSize = parcHashMap_Size(lb->strategy_state);
    MetisNumberSet *outList = metisNumberSet_Create();

    if ((mapSize == 0) || ((mapSize == 1) && parcHashMap_Contains(lb->strategy_state, in))) {
        //there are no output faces or the input face is also the only output face. return null to avoid loops
        parcUnsigned_Release(&in);
        return outList;
    }

    unsigned out_connection;
    do {
        out_connection = _select_Nexthop(lb);
    } while (out_connection == in_connection);

    PARCUnsigned *out = parcUnsigned_Create(out_connection);

    const StrategyNexthopState *state = parcHashMap_Get(lb->strategy_state, out);
    if (state == NULL) {
        //this is an error and should not happen!
        trapNotImplemented("Try to send an interest on a face that does not exists");
    }

    _update_Stats(lb, (StrategyNexthopState *) state, true);

    parcUnsigned_Release(&in);
    parcUnsigned_Release(&out);

    metisNumberSet_Add(outList, out_connection);
    return outList;
}

static MetisNumberSet *
_strategyLoadBalancer_ReturnNexthops(MetisStrategyImpl *strategy)
{
    StrategyLoadBalancer *lb = (StrategyLoadBalancer *) strategy->context;
    return lb->nexthops;
}

unsigned
_strategyLoadBalancer_CountNexthops(MetisStrategyImpl *strategy)
{
    StrategyLoadBalancer *lb = (StrategyLoadBalancer *) strategy->context;
    return metisNumberSet_Length(lb->nexthops);
}

static void
_strategyLoadBalancer_resetState(MetisStrategyImpl *strategy)
{
    StrategyLoadBalancer *lb = (StrategyLoadBalancer *) strategy->context;
    lb->weights_sum = 0.0;
    PARCIterator *it = parcHashMap_CreateKeyIterator(lb->strategy_state);

    while (parcIterator_HasNext(it)) {
        PARCUnsigned *cid = parcIterator_Next(it);
        StrategyNexthopState *elem = (StrategyNexthopState *) parcHashMap_Get(lb->strategy_state, cid);

        strategyNexthopState_Reset(elem);
        lb->weights_sum += strategyNexthopState_GetWeight(elem);
    }

    parcIterator_Release(&it);
}

static void
_strategyLoadBalancer_AddNexthop(MetisStrategyImpl *strategy, CPIRouteEntry *route)
{
    unsigned connectionId = cpiRouteEntry_GetInterfaceIndex(route); //this returns what in all the rest of the ccnx code is called connection id!


    StrategyNexthopState *state = strategyNexthopState_Create();

    PARCUnsigned *cid = parcUnsigned_Create(connectionId);

    StrategyLoadBalancer *lb = (StrategyLoadBalancer *) strategy->context;

    if (!parcHashMap_Contains(lb->strategy_state, cid)) {
        parcHashMap_Put(lb->strategy_state, cid, state);
        metisNumberSet_Add(lb->nexthops, connectionId);
        _strategyLoadBalancer_resetState(strategy);
    }
}

static void
_strategyLoadBalancer_RemoveNexthop(MetisStrategyImpl *strategy, CPIRouteEntry *route)
{
    unsigned connectionId = cpiRouteEntry_GetInterfaceIndex(route);
    StrategyLoadBalancer *lb = (StrategyLoadBalancer *) strategy->context;

    PARCUnsigned *cid = parcUnsigned_Create(connectionId);

    if (parcHashMap_Contains(lb->strategy_state, cid)) {
        parcHashMap_Remove(lb->strategy_state, cid);
        metisNumberSet_Remove(lb->nexthops, connectionId);
        _strategyLoadBalancer_resetState(strategy);
    }

    parcUnsigned_Release(&cid);
}

static void
_strategyLoadBalancer_ImplDestroy(MetisStrategyImpl **strategyPtr)
{
    assertNotNull(strategyPtr, "Parameter must be non-null double pointer");
    assertNotNull(*strategyPtr, "Parameter must dereference to non-null pointer");

    MetisStrategyImpl *impl = *strategyPtr;
    StrategyLoadBalancer *strategy = (StrategyLoadBalancer *) impl->context;

    parcHashMap_Release(&(strategy->strategy_state));
    metisNumberSet_Release(&(strategy->nexthops));

    parcMemory_Deallocate((void **) &strategy);
    parcMemory_Deallocate((void **) &impl);
    *strategyPtr = NULL;
}
