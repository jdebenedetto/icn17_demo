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
 * @file metis_TcpListener.h
 * @brief Listens for in coming TCP connections
 *
 * This is the "server socket" of metis for TCP connections.  The actual I/O is
 * handled by {@link MetisStreamConnection}.
 *
 */

#ifndef Metis_metis_TcpListener_h
#define Metis_metis_TcpListener_h

#include <netinet/in.h>
#include <stdlib.h>
#include <ccnx/forwarder/metis/core/metis_Forwarder.h>
#include <ccnx/forwarder/metis/io/metis_Listener.h>

MetisListenerOps *metisTcpListener_CreateInet6(MetisForwarder *metis, struct sockaddr_in6 sin6);
MetisListenerOps *metisTcpListener_CreateInet(MetisForwarder *metis, struct sockaddr_in sin);
#endif // Metis_metis_TcpListener_h
