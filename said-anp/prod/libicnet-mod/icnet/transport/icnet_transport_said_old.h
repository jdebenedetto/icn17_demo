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

#ifndef ICNET_SAID_TRANSPORT_PROTOCOL_H_
#define ICNET_SAID_TRANSPORT_PROTOCOL_H_

#include "icnet_transport_protocol.h"
#include "icnet_transport_vegas_rto_estimator.h"


namespace icnet {

namespace transport {

class SaidTransportProtocol : public TransportProtocol {
 public:

  SaidTransportProtocol(Socket *icnet_socket);

  virtual ~SaidTransportProtocol();

  virtual void start();

  void stop();

 protected:

  void sendInterest(uint64_t segment_number);

  void sendInterest();

  void onContentSegment(const Interest &interest, ContentObject &content_object);

  void onContentObject(const Interest &interest, ContentObject &content_object);

  bool verifyContentObject(const Interest &interest, ContentObject &content_object);

  virtual void onTimeout(const Interest &interest);
  
  virtual void decreaseWindow();

  virtual void increaseWindow();

  virtual void afterContentReception(const Interest &interest, const ContentObject &content_object);

  virtual void afterDataUnsatisfied(uint64_t segment);

  virtual void changeInterestLifetime(uint64_t segment);

  void onManifest(const Interest &interest, ContentObject &content_object);

  void scheduleNextInterests();

  void reassemble();

  virtual void copyContent(ContentObject &content_object);

  std::vector<uint64_t> getMissingSegmentNumbers();

  void removeAllPendingInterests();

 protected:
  // reassembly variables
  bool is_final_block_number_discovered_;
  uint64_t final_block_number_;
  bool is_all_blocks_received_;
  uint64_t last_reassembled_segment_;
  std::vector<uint8_t> content_buffer_;
  size_t content_buffer_size_;

  // transmission variables
  double current_window_size_;
  double pending_window_size_;
  uint64_t interests_in_flight_;
  uint64_t segment_number_;
  std::vector<int> interest_retransmissions_;
  int anp_interest_retransmissions_;
  std::vector<std::chrono::steady_clock::time_point> interest_timepoints_;
  std::chrono::steady_clock::time_point anp_interest_timepoints_;
  RtoEstimator rtt_estimator_;

  // buffers
  std::vector<std::shared_ptr<ContentObject> > receive_buffer_; // verified segments by segment number
  std::vector<std::shared_ptr<ContentObject> > unverified_segments_; // used with embedded manifests
  std::vector<std::shared_ptr<Manifest> > verified_manifests_; // by segment number
  std::map<uint64_t, std::shared_ptr<ContentObject> > received_segments_;
  std::vector<uint64_t> received_segment_numbers_;
};

} // end namespace transport

} // end namespace icnet


#endif // ICNET_SAID_TRANSPORT_PROTOCOL_H_
