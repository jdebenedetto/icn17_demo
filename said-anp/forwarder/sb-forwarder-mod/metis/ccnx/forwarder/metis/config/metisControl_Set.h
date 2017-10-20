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
 * @file metisControl_Set.h
 * @brief Implements the set node of the CLI tree
 *
 * Implements the "set" and "help set" nodes of the command tree
 *
 */
#ifndef Metis_metisControl_Set_h
#define Metis_metisControl_Set_h

#include <ccnx/forwarder/metis/config/metis_ControlState.h>
MetisCommandOps *metisControlSet_Create(MetisControlState *state);
MetisCommandOps *metisControlSet_HelpCreate(MetisControlState *state);
#endif // Metis_metisControl_Set_h
