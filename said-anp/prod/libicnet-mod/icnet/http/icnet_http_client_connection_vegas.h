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

#pragma once

#include "icnet_transport_socket_consumer.h"
#include "icnet_transport_socket_producer.h"
#include "icnet_utils_uri.h"
#include "icnet_http_request.h"
#include "icnet_http_default_values.h"

#include <vector>

#define HTTP_VERSION "1.0"

namespace icnet {

namespace http {

class HTTPClientConnectionVegas {
 public:
  HTTPClientConnectionVegas();

  HTTPClientConnectionVegas &get(std::string &url, HTTPHeaders headers = {}, HTTPPayload payload = {});

  HTTPResponse &&response();

  HTTPClientConnectionVegas &stop();

 private:

  void processPayload(transport::ConsumerSocket &c, std::vector<uint8_t> &&payload);

  bool verifyData(transport::ConsumerSocket &c, const transport::ContentObject &contentObject);

  void processLeavingInterest(transport::ConsumerSocket &c, const transport::Interest &interest, std::string &payload);

  HTTPResponse response_;
  transport::ConsumerSocket consumer_;
};

} // end namespace http

} // end namespace icnet