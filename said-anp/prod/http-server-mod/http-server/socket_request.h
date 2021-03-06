/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2016 Ole Christian Eidheim
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef ICN_WEB_SERVER_SOCKETREQUEST_H_
#define ICN_WEB_SERVER_SOCKETREQUEST_H_

#include "request.h"

namespace icn_httpserver {

class SocketRequest
    : public Request {
 public:
  void read_remote_endpoint_data(socket_type &socket);

 private:
  std::string remote_endpoint_address_;
  unsigned short remote_endpoint_port_;
};

} // end namespace icn_httpserver

#endif // ICN_WEB_SERVER_SOCKETREQUEST_H_
