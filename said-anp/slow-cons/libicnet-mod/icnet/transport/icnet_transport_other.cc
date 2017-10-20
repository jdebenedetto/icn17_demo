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

#include "icnet_transport_other.h"
#include "icnet_transport_socket_consumer.h"

namespace icnet {

namespace transport {

OtherTransportProtocol::OtherTransportProtocol(Socket *icnet_socket)
    : TransportProtocol(icnet_socket) {
  icnet_socket->getSocketOption(PORTAL, portal_);
}

OtherTransportProtocol::~OtherTransportProtocol() {
  stop();
}

void OtherTransportProtocol::start() {
  
}

void OtherTransportProtocol::stop() {
  is_running_ = false;
  portal_->stopEventsLoop();
}


} // end namespace transport

} // end namespace icnet
