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

#include "http_client.h"

#include <curl/curl.h>
#include <sstream>
#include <iostream>

using namespace std;

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
  ((ostream*) stream)->write((const char*)ptr, size * nmemb);
  return size * nmemb;
}

HTTPClient::HTTPClient() {
  curl_ = curl_easy_init();
}

HTTPClient::~HTTPClient() {
  curl_easy_cleanup(curl_);
}

bool HTTPClient::download(const std::string& url, std::ostream& out) {
  curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());

  /* example.com is redirected, so we tell libcurl to follow redirection */
  curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1);
  curl_easy_setopt(curl_, CURLOPT_ACCEPT_ENCODING, "deflate");

  curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_data);
  curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &out);

  /* Perform the request, res will get the return code */
  CURLcode res = curl_easy_perform(curl_);

  /* Check for errors */
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
    return false;
  }

  return true;
}