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
 * @file metis_Listener.h
 * @brief Provides the function abstraction of all Listeners.
 *
 * A listener accepts in coming packets.  A Stream listener will accept the connection
 * then pass it off to the {@link MetisStreamConnection} class.  A datagram listener
 * will have to have its own way to multiplex packets.
 *
 */

#ifndef Metis_metis_Listener_h
#define Metis_metis_Listener_h

#include <ccnx/api/control/cpi_Address.h>

struct metis_listener_ops;
typedef struct metis_listener_ops MetisListenerOps;

typedef enum {
    METIS_ENCAP_TCP,  /**< TCP encapsulation type */
    METIS_ENCAP_UDP,   /**< UDP encapsulation type */
    METIS_ENCAP_ETHER,  /**< Ethernet encapsulation type */
    METIS_ENCAP_LOCAL  /**< A connection to a local protocol stack */
} MetisEncapType;

struct metis_listener_ops {
    /**
     * A user-defined parameter
     */
    void *context;

    /**
     * Called to destroy the Listener.
     *
     * @param [in] listenerOpsPtr Double pointer to this structure
     */
    void (*destroy)(MetisListenerOps **listenerOpsPtr);

    /**
     * Returns the interface index of the listener.
     *
     * @param [in] ops Pointer to this structure
     *
     * @return the interface index of the listener
     */
    unsigned (*getInterfaceIndex)(const MetisListenerOps *ops);

    /**
     * Returns the address pair that defines the listener (local, remote)
     *
     * @param [in] ops Pointer to this structure
     *
     * @return the (local, remote) pair of addresses
     */
    const CPIAddress * (*getListenAddress)(const MetisListenerOps *ops);

    /**
     * Returns the encapsulation type of the listener (e.g. TCP, UDP, Ethernet)
     *
     * @param [in] ops Pointer to this structure
     *
     * @return the listener encapsulation type
     */
    MetisEncapType (*getEncapType)(const MetisListenerOps *ops);

    /**
     * Returns the underlying socket associated with the listener
     *
     * Not all listeners are capable of returning a useful socket.  In those
     * cases, this function pointer is NULL.
     *
     * TCP does not support this operation (function is NULL).  UDP returns its local socket.
     *
     * The caller should never close this socket, the listener will do that when its
     * destroy method is called.
     *
     * @param [in] ops Pointer to this structure
     *
     * @retval integer The socket descriptor
     *
     * Example:
     * @code
     * <#example#>
     * @endcode
     */
    int (*getSocket)(const MetisListenerOps *ops);
};
#endif // Metis_metis_Listener_h
