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

#include "icnet_transport_said.h"
#include "icnet_transport_socket_consumer.h"

namespace icnet {

namespace transport {

SaidTransportProtocol::SaidTransportProtocol(Socket *icnet_socket)
    : TransportProtocol(icnet_socket),
      is_final_block_number_discovered_(false),
      final_block_number_(std::numeric_limits<uint64_t>::max()),
      is_all_blocks_received_(false),
      last_reassembled_segment_(0),
      content_buffer_size_(0),
      current_window_size_(default_values::min_window_size),
      interests_in_flight_(0),
      segment_number_(0),
      interest_retransmissions_(default_values::default_buffer_size),
      interest_timepoints_(default_values::default_buffer_size),
      receive_buffer_(default_values::default_buffer_size),
      unverified_segments_(default_values::default_buffer_size),
      verified_manifests_(default_values::default_buffer_size) {
  icnet_socket->getSocketOption(PORTAL, portal_);
}

SaidTransportProtocol::~SaidTransportProtocol() {
  stop();
}

void SaidTransportProtocol::start() {
  is_running_ = true; //inherited from TransportProtocol
  
  is_all_blocks_received_ = false;
  is_final_block_number_discovered_ = false;
  final_block_number_ = std::numeric_limits<uint64_t>::max();
  segment_number_ = 0;
  interests_in_flight_ = 0;
  last_reassembled_segment_ = 0;
  content_buffer_size_ = 0;
  content_buffer_.clear();
  interest_retransmissions_.clear();
  receive_buffer_.clear();
  unverified_segments_.clear();
  verified_manifests_.clear();

  //sendInterest();
  sendInterest(true); //bool anp = true

  bool isAsync = false;
  socket_->getSocketOption(ASYNC_MODE, isAsync);

  bool isContextRunning = false;
  socket_->getSocketOption(RUNNING, isContextRunning);

  if (!isAsync && !isContextRunning) {
    socket_->setSocketOption(RUNNING, true);
    portal_->runEventsLoop();

    // If portal returns, the download (maybe) is finished, so we can remove the pending interests

    removeAllPendingInterests();
  }

}

void SaidTransportProtocol::stop() {
  is_running_ = false;

  portal_->stopEventsLoop();
}

void SaidTransportProtocol::sendInterest() {
  Name prefix;
  socket_->getSocketOption(GeneralTransportOptions::NAME_PREFIX, prefix);

  Name suffix;
  socket_->getSocketOption(GeneralTransportOptions::NAME_SUFFIX, suffix);

  if (!suffix.empty()) {
    prefix.append(suffix);
  }

  // prefix.appendANPSegment(segment_number_);
  prefix.appendSegment(segment_number_);
  //TO-DO: add ANP flag to interest

  std::shared_ptr<Interest> interest = std::make_shared<Interest>(std::move(prefix));

  int interestLifetime = default_values::interest_lifetime;
  socket_->getSocketOption(GeneralTransportOptions::INTEREST_LIFETIME, interestLifetime);
  interest->setInterestLifetime(uint32_t(interestLifetime));

  ConsumerInterestCallback on_interest_output = VOID_HANDLER;

  socket_->getSocketOption(ConsumerCallbacksOptions::INTEREST_OUTPUT, on_interest_output);
  if (on_interest_output != VOID_HANDLER) {
    on_interest_output(*dynamic_cast<ConsumerSocket *>(socket_), *interest);
  }

  if (!is_running_) {
    return;
  }

  interests_in_flight_++;
  
  anp_interest_retransmissions_ = 0;
  anp_interest_timepoints_ = std::chrono::steady_clock::now();

  portal_->sendInterest(*interest,
                        bind(&SaidTransportProtocol::onContentSegment, this, _1, _2),
                        bind(&SaidTransportProtocol::onTimeout, this, _1));
  // the reception of ANP data can be not sequential, but the segments is expected to be sent sequentially from the producer
  // so it's good to mantain a trace of the expected segment number to receive (to be updated if reiceve a greater segment
  // than the one expected and easily find missing packets in the reception)
  segment_number_++;
}

void SaidTransportProtocol::onContentObject(const Interest &interest, ContentObject &content_object) {
  if (verifyContentObject(interest, content_object)) {
    // checkForFastRetransmission(interest);

    uint64_t segment = interest.getName().get(-1).toSegment();

    if (interest_retransmissions_[segment % default_values::default_buffer_size] == 0) {
      afterContentReception(interest, content_object);
    }

    if (content_object.hasFinalChunkNumber()) {
      is_final_block_number_discovered_ = true;
      final_block_number_ = content_object.getFinalChunkNumber();
    }

    bool virtualDownload = false;

    socket_->getSocketOption(VIRTUAL_DOWNLOAD, virtualDownload);

    if (!virtualDownload) {
      //receive_buffer_[segment % default_values::default_buffer_size] = content_object.shared_from_this();
      received_segments_[segment] = content_object.shared_from_this();
      received_segment_numbers_.push_back(segment);
      if(segment == last_reassembled_segment_) {
        reassemble();
      }
    } else {
      if (segment == final_block_number_) {
        portal_->stopEventsLoop();
      }
    }
  }
}

void SaidTransportProtocol::onContentSegment(const Interest &interest, ContentObject &content_object) {
  
  anp_interest_retransmissions_ = 0;

  //uint64_t segment = interest.getName().get(-1).toSegment();

  if (is_running_ == false /*|| input_buffer_[segment]*/) {
    return;
  }

  interests_in_flight_--;

  //changeAnpInterestLifetime();
  ConsumerContentObjectCallback on_data_input = VOID_HANDLER;
  socket_->getSocketOption(ConsumerCallbacksOptions::CONTENT_OBJECT_INPUT, on_data_input);
  if (on_data_input != VOID_HANDLER) {
    on_data_input(*dynamic_cast<ConsumerSocket *>(socket_), content_object);
  }

  ConsumerInterestCallback on_interest_satisfied = VOID_HANDLER;
  socket_->getSocketOption(INTEREST_SATISFIED, on_interest_satisfied);
  if (on_interest_satisfied != VOID_HANDLER) {
    on_interest_satisfied(*dynamic_cast<ConsumerSocket *>(socket_), const_cast<Interest &>(interest));
  }

  if (content_object.getPayloadType() == PayloadType::MANIFEST) {
    onManifest(interest, content_object);
  } else if (content_object.getPayloadType() == PayloadType::DATA) {
    onContentObject(interest, content_object);
  } // TODO InterestReturn

  scheduleNextInterests();
}

void SaidTransportProtocol::onTimeout(const Interest &interest) {
  if (!is_running_) {
    return;
  }

  interests_in_flight_--;

  std::cerr << "Timeout on " << interest.getName() << std::endl;

  ConsumerInterestCallback on_interest_timeout = VOID_HANDLER;
  socket_->getSocketOption(INTEREST_EXPIRED, on_interest_timeout);
  if (on_interest_timeout != VOID_HANDLER) {
    on_interest_timeout(*dynamic_cast<ConsumerSocket *>(socket_), const_cast<Interest &>(interest));
  }

  uint64_t segment = interest.getName().get(-1).toSegment();

  // Do not retransmit interests asking contents that do not exist.
  if (is_final_block_number_discovered_) {
    if (segment > final_block_number_) {
      return;
    }
  }

  afterDataUnsatisfied(segment);

  int max_retransmissions;
  socket_->getSocketOption(GeneralTransportOptions::MAX_INTEREST_RETX, max_retransmissions);

  if (anp_interest_retransmissions_ < max_retransmissions) {

    ConsumerInterestCallback on_interest_retransmission = VOID_HANDLER;
    socket_->getSocketOption(ConsumerCallbacksOptions::INTEREST_RETRANSMISSION, on_interest_retransmission);

    if (on_interest_retransmission != VOID_HANDLER) {
      on_interest_retransmission(*dynamic_cast<ConsumerSocket *>(socket_), interest);
    }

    ConsumerInterestCallback on_interest_output = VOID_HANDLER;

    socket_->getSocketOption(ConsumerCallbacksOptions::INTEREST_OUTPUT, on_interest_output);
    if (on_interest_output != VOID_HANDLER) {
      on_interest_output(*dynamic_cast<ConsumerSocket *>(socket_), interest);
    }

    if (!is_running_) {
      return;
    }

    //retransmit
    interests_in_flight_++;
    anp_interest_retransmissions_++;

    portal_->sendInterest(interest,
                          bind(&SaidTransportProtocol::onContentSegment, this, _1, _2),
                          bind(&SaidTransportProtocol::onTimeout, this, _1));
  } else {
    is_running_ = false;

    bool virtual_download = false;
    socket_->getSocketOption(VIRTUAL_DOWNLOAD, virtual_download);

    if (!virtual_download) {
      reassemble();
    }

    portal_->stopEventsLoop();
  }
}

void SaidTransportProtocol::afterContentReception(const Interest &interest, const ContentObject &content_object) {
  increaseWindow();
}

void SaidTransportProtocol::afterDataUnsatisfied(uint64_t segment) {
  decreaseWindow();
}

void SaidTransportProtocol::changeInterestLifetime(uint64_t segment) {
  std::chrono::steady_clock::duration duration = std::chrono::steady_clock::now() - interest_timepoints_[segment];
  rtt_estimator_.addMeasurement(std::chrono::duration_cast<std::chrono::microseconds>(duration));

  RtoEstimator::Duration rto = rtt_estimator_.computeRto();
  std::chrono::milliseconds lifetime = std::chrono::duration_cast<std::chrono::milliseconds>(rto);

  socket_->setSocketOption(INTEREST_LIFETIME, (int) lifetime.count());
}

bool SaidTransportProtocol::verifyContentObject(const Interest &interest, ContentObject &content_object) {
  // TODO Check content object using manifest
  return true;
}

void SaidTransportProtocol::reassemble() {

  //when received the correct ordered segments, get them out of the buffer
  while(received_segments_.begin()->first == last_reassembled_segment_) {
    if (received_segments_[last_reassembled_segment_]->getPayloadType() == PayloadType::DATA) {
      copyContent(*received_segments_[last_reassembled_segment_]);
    }
    received_segments_[last_reassembled_segment_].reset();
    received_segments_.erase(last_reassembled_segment_);
    last_reassembled_segment_++;
  }

}

void SaidTransportProtocol::copyContent(ContentObject &content_object) {
  Array a = content_object.getContent();

  content_buffer_.insert(content_buffer_.end(), (uint8_t *) a.data(), (uint8_t *) a.data() + a.size());

  //if ((content_object.getName().get(-1).toSegment() == final_block_number_) || (!is_running_)) {
  if (is_all_blocks_received_ || (!is_running_)) {
    // return content to the user
    ConsumerContentCallback on_payload = VOID_HANDLER;
    socket_->getSocketOption(CONTENT_RETRIEVED, on_payload);
    if (on_payload != VOID_HANDLER) {
      on_payload(*dynamic_cast<ConsumerSocket *>(socket_),
                 std::move(content_buffer_));
    }

    //reduce window size to prevent its speculative growth in case when consume() is called in loop
    int current_window_size = -1;
    socket_->getSocketOption(CURRENT_WINDOW_SIZE, current_window_size);
    if ((uint64_t) current_window_size > final_block_number_) {
      socket_->setSocketOption(CURRENT_WINDOW_SIZE, (int) (final_block_number_));
    }

    is_running_ = false;
    portal_->stopEventsLoop();
  }
}

std::vector<uint64_t> SaidTransportProtocol::getMissingSegmentNumbers() {

    std::sort(received_segment_numbers_.begin(), received_segment_numbers_.end());
    std::vector<uint64_t> missing_segment_numbers;
    uint64_t next = 1;
    for (std::vector<uint64_t>::iterator it = received_segment_numbers_.begin(); it != received_segment_numbers_.end(); ++it) {
        if (*it != next) {
          missing_segment_numbers.push_back(next);
        }
       ++next;
    }
    return missing_segment_numbers;
}

void SaidTransportProtocol::decreaseWindow() {
  double min_window_size = -1;
  socket_->getSocketOption(MIN_WINDOW_SIZE, min_window_size);
  if (current_window_size_ > min_window_size) {
    current_window_size_ = std::ceil(current_window_size_ / 2);

    socket_->setSocketOption(CURRENT_WINDOW_SIZE, current_window_size_);
  }
}

void SaidTransportProtocol::increaseWindow() {
  double max_window_size = -1;
  socket_->getSocketOption(MAX_WINDOW_SIZE, max_window_size);
  if (current_window_size_ < max_window_size) {
    current_window_size_++;
    socket_->setSocketOption(CURRENT_WINDOW_SIZE, current_window_size_);
  }
};

void SaidTransportProtocol::removeAllPendingInterests() {
  portal_->clear();
}

} // end namespace transport

} // end namespace icnet
