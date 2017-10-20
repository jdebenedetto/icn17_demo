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
      is_final_block_number_received_(false),
      final_block_number_(std::numeric_limits<uint64_t>::max()),
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
  is_running_ = true;
  is_final_block_number_received_ = false;
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

  socket_->setSocketOption(MAX_WINDOW_SIZE, (int) (200)); //TEST

  sendInterest();

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

// TODO Reuse this function for sending an arbitrary interest
void SaidTransportProtocol::sendInterest() {
  Name prefix;
  socket_->getSocketOption(GeneralTransportOptions::NAME_PREFIX, prefix);

  Name suffix;
  socket_->getSocketOption(GeneralTransportOptions::NAME_SUFFIX, suffix);

  if (!suffix.empty()) {
    prefix.append(suffix);
  }

  prefix.appendSegment(segment_number_);
  //------TEST------------------------------------
  // if(segment_number_ == final_block_number_) {
  //   prefix.appendSegment(segment_number_);
  // }
  // else {
  //   prefix.appendSegment(segment_number_ + (rand() % 10)); //TEST
  // }

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
    // std::cout << "sendInterest: is not running " << std::endl; //DEBUG    
    return;
  }

  interests_in_flight_++;
  interest_retransmissions_[segment_number_ % default_values::default_buffer_size] = 0;
  interest_timepoints_[segment_number_ % default_values::default_buffer_size] = std::chrono::steady_clock::now();

  interest->setFlags(1);
  portal_->sendInterest(*interest,
                        bind(&SaidTransportProtocol::onContentSegment, this, _1, _2),
                        bind(&SaidTransportProtocol::onTimeout, this, _1));
  segment_number_++;
}

void SaidTransportProtocol::stop() {
  is_running_ = false;
  portal_->stopEventsLoop();
}

void SaidTransportProtocol::onContentSegment(const Interest &interest, ContentObject &content_object) {

  if (is_running_ == false /*|| input_buffer_[segment]*/) {
    // std::cout << "onCOntentSegment: is_running==false" << std::endl; //DEBUG
    return;
  }

  interests_in_flight_--;

  uint64_t received_segment = content_object.getName().get(-1).toSegment();
  received_segments_[received_segment] = true;
  
  // std::cout << "Segment received: " << received_segment << std::endl; //DEBUG
  // std::cout << "Segment expeted: " << segment_number_ - interests_in_flight_ - 1 << std::endl; //DEBUG
  // std::cout << "Interests in flight: " << interests_in_flight_ << std::endl; //DEBUG

  if(received_segment > segment_number_ && received_segment < final_block_number_) {
    segment_number_ = received_segment + 1;
  }

  changeInterestLifetime(received_segment);
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

  if(!is_final_block_number_received_) {
    scheduleNextInterests();
  }
  else {
    checkForFastRetransmission(final_block_number_);
  }
  
}

void SaidTransportProtocol::afterContentReception(const Interest &interest, const ContentObject &content_object) {
  increaseWindow();
}

void SaidTransportProtocol::afterDataUnsatisfied(uint64_t segment) {
  decreaseWindow();
}

void SaidTransportProtocol::scheduleNextInterests() {
  if (segment_number_ == 0) {
    current_window_size_ = final_block_number_;

    double maxWindowSize = -1;
    socket_->getSocketOption(MAX_WINDOW_SIZE, maxWindowSize);
    maxWindowSize = 50; //TEST

    if (current_window_size_ > maxWindowSize) {
      current_window_size_ = maxWindowSize;
    }

    while (interests_in_flight_ < current_window_size_) {
      if (segment_number_ <= final_block_number_) {
        sendInterest();
      } else {
        break;
      }
    }
  } else {
    if (is_running_) {
      while (interests_in_flight_ < current_window_size_) {
        if (segment_number_ <= final_block_number_) {
          sendInterest();
        } else {
          // segment_number_ = final_block_number_;
          // sendInterest();
          break;
        }
      }
    }
  }
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
  max_window_size = 50; //TEST
  if (current_window_size_ < max_window_size) {
    current_window_size_++;
    socket_->setSocketOption(CURRENT_WINDOW_SIZE, current_window_size_);
  }
};

void SaidTransportProtocol::changeInterestLifetime(uint64_t segment) {
  std::chrono::steady_clock::duration duration = std::chrono::steady_clock::now() - interest_timepoints_[segment];
  rtt_estimator_.addMeasurement(std::chrono::duration_cast<std::chrono::microseconds>(duration));

  RtoEstimator::Duration rto = rtt_estimator_.computeRto();
  std::chrono::milliseconds lifetime = std::chrono::duration_cast<std::chrono::milliseconds>(rto);

  socket_->setSocketOption(INTEREST_LIFETIME, (int) lifetime.count());
}

void SaidTransportProtocol::onManifest(const Interest &interest, ContentObject &content_object) {
  if (!is_running_) {
    return;
  }

  if (verifyManifest(content_object)) {
    // TODO Retrieve piece of data using manifest
  }
}

bool SaidTransportProtocol::verifyManifest(ContentObject &content_object) {
  ConsumerContentObjectVerificationCallback on_manifest_to_verify = VOID_HANDLER;
  socket_->getSocketOption(ConsumerCallbacksOptions::CONTENT_OBJECT_TO_VERIFY, on_manifest_to_verify);

  bool is_data_secure = false;

  if (on_manifest_to_verify == VOID_HANDLER) {
    // TODO Perform manifest verification
  } else if (on_manifest_to_verify(*dynamic_cast<ConsumerSocket *>(socket_), content_object)) {
    is_data_secure = true;
  }

  return is_data_secure;
}

bool SaidTransportProtocol::requireInterestWithHash(const Interest &interest,
                                                     const ContentObject &content_object,
                                                     Manifest &manifest) {
  // TODO Require content object with specific hash.
  return true;
}

// TODO Add the name in the digest computation!
void SaidTransportProtocol::onContentObject(const Interest &interest, ContentObject &content_object) {
  if (verifyContentObject(interest, content_object)) {

    uint64_t segment = content_object.getName().get(-1).toSegment();

    // checkForFastRetransmission(segment);

    if (interest_retransmissions_[segment % default_values::default_buffer_size] == 0) {
      afterContentReception(interest, content_object);
    }

    if (content_object.hasFinalChunkNumber()) {
      final_block_number_ = content_object.getFinalChunkNumber();
      // std::cout << "Final block number: " << final_block_number_ << std::endl; //DEBUG
    }

    bool virtualDownload = false;

    socket_->getSocketOption(VIRTUAL_DOWNLOAD, virtualDownload);

    if (!virtualDownload) {
      receive_buffer_[segment % default_values::default_buffer_size] = content_object.shared_from_this();
      reassemble();

      if(segment == final_block_number_) {
        removeAllPendingInterests();
        is_final_block_number_received_ = true;
      }
      
      if (is_final_block_number_received_) {
        //REPAIR after last segment
        std::cout << "REPAIR phase" << std::endl; //DEBUG
        checkForFastRetransmission(final_block_number_);
      }

    } else {
      if (segment == final_block_number_) {
        portal_->stopEventsLoop();
      }
    }
  }
}

bool SaidTransportProtocol::verifyContentObject(const Interest &interest, ContentObject &content_object) {
  // TODO Check content object using manifest
  return true;
}

// TODO move inside manifest
bool SaidTransportProtocol::pointsToManifest(ContentObject &content_object) {
  // TODO Check content objects using manifest
  return true;
}

void SaidTransportProtocol::onTimeout(const Interest &interest) {
  if (!is_running_) {
    // std::cout << "onTimeout1: is not running " << std::endl; //DEBUG    
    return;
  }

  interests_in_flight_--;

  // std::cout << "Timeout on " << interest.getName() << std::endl; //DEBUG

  ConsumerInterestCallback on_interest_timeout = VOID_HANDLER;
  socket_->getSocketOption(INTEREST_EXPIRED, on_interest_timeout);
  if (on_interest_timeout != VOID_HANDLER) {
    on_interest_timeout(*dynamic_cast<ConsumerSocket *>(socket_), const_cast<Interest &>(interest));
  }
 
  uint64_t segment = interest.getName().get(-1).toSegment();
  //  uint64_t segment = segment_number_;
  fast_retransmitted_segments[segment] = false;

  // // Do not retransmit interests asking contents that do not exist.
  // if (segment > final_block_number_) {
  //   segment = final_block_number_;
  // }

  afterDataUnsatisfied(segment);

  int max_retransmissions;
  socket_->getSocketOption(GeneralTransportOptions::MAX_INTEREST_RETX, max_retransmissions);

  // if (interest_retransmissions_[segment % default_values::default_buffer_size] < max_retransmissions) {
  if (true) {//HACK

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
      // std::cout << "onTimeout1: is not running " << std::endl; //DEBUG          
      return;
    }

    //retransmit
    // interests_in_flight_++;
    // interest_retransmissions_[segment % default_values::default_buffer_size]++;
    // std::cout << "Retransmission of segment: " << segment << std::endl; //DEBUG
    // portal_->sendInterest(interest,
    //                       bind(&SaidTransportProtocol::onContentSegment, this, _1, _2),
    //                       bind(&SaidTransportProtocol::onTimeout, this, _1));

    if(segment_number_ > final_block_number_) {
      segment_number_ = final_block_number_;
    }

    if(is_final_block_number_received_) {
      checkForFastRetransmission(final_block_number_);
    }
    else {
      sendInterest();
    }    

  } else {

    std::cout << "Max restransmission exceeded for segment: " << segment << std::endl; //DEBUG

    is_running_ = false;

    bool virtual_download = false;
    socket_->getSocketOption(VIRTUAL_DOWNLOAD, virtual_download);

    if (!virtual_download) {
      reassemble();
    }

    portal_->stopEventsLoop();
  }

}

void SaidTransportProtocol::copyContent(ContentObject &content_object) {
  Array a = content_object.getContent();

  content_buffer_.insert(content_buffer_.end(), (uint8_t *) a.data(), (uint8_t *) a.data() + a.size());

  // if ((last_reassembled_segment_ == final_block_number_) || (!is_running_)) {
  if (last_reassembled_segment_ == final_block_number_) {

    // return content to the user
    std::cout << "Return content to the user" << std::endl; //DEBUG        
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

void SaidTransportProtocol::reassemble() {
  uint64_t index = last_reassembled_segment_ % default_values::default_buffer_size;
  // std::cout << "Default buffer size = " <<  default_values::default_buffer_size << std::endl; //DEBUG
  while (receive_buffer_[index % default_values::default_buffer_size]) {
    if (receive_buffer_[index % default_values::default_buffer_size]->getPayloadType() == PayloadType::DATA) {
      copyContent(*receive_buffer_[index % default_values::default_buffer_size]);
    }

    receive_buffer_[index % default_values::default_buffer_size].reset();
    // std::cout << "last_reassembled_segment = " << last_reassembled_segment_ << std::endl; //DEBUG
    last_reassembled_segment_++;
    index = last_reassembled_segment_ % default_values::default_buffer_size;
  }
}

bool SaidTransportProtocol::verifySegmentUsingManifest(Manifest &manifestSegment, ContentObject &content_object) {
  // TODO Content object verification exploiting manifest
  return true;
}

void SaidTransportProtocol::checkForFastRetransmission(uint64_t last_received) {

  // uint64_t possibly_lost_segment = 0;
  uint64_t highest_received_segment = received_segments_.rbegin()->first;
  // std::cout << "final_block_number = " << final_block_number_ << std::endl; //DEBUG
  // std::cout << "highest_received_segment = " << highest_received_segment << std::endl; //DEBUG
  // std::cout << "last_reassembled_segment = " << last_reassembled_segment_ << std::endl; //DEBUG  

  for (uint64_t i = last_reassembled_segment_; i <= highest_received_segment; i++) {
    if (interests_in_flight_ >= 50) { //fixed window size
      break;
    }
    if (received_segments_.find(i) == received_segments_.end()) {
      if (fast_retransmitted_segments.find(i) == fast_retransmitted_segments.end()) {
      // if (true) { //HACK
        fast_retransmitted_segments[i] = true;
        // std::cout << "Repair segment = " << i << std::endl; //DEBUG        
        fastRetransmit(i);
        // break; //TEST only one repair at a time
      }
    }
  }
}

void SaidTransportProtocol::fastRetransmit(uint64_t chunk_number) {
  int max_retransmissions;
  socket_->getSocketOption(GeneralTransportOptions::MAX_INTEREST_RETX, max_retransmissions);

  // if (interest_retransmissions_[chunk_number % default_values::default_buffer_size] < max_retransmissions) {
  if (true) {//HACK
    Name prefix;
    socket_->getSocketOption(GeneralTransportOptions::NAME_PREFIX, prefix);

    Name suffix;
    socket_->getSocketOption(GeneralTransportOptions::NAME_SUFFIX, suffix);

    if (!suffix.empty()) {
      prefix.append(suffix);
    }

    prefix.appendSegment(chunk_number);

    std::shared_ptr<Interest> retx_interest = std::make_shared<Interest>(prefix);

    ConsumerInterestCallback on_interest_retransmission = VOID_HANDLER;
    socket_->getSocketOption(ConsumerCallbacksOptions::INTEREST_RETRANSMISSION, on_interest_retransmission);

    if (on_interest_retransmission != VOID_HANDLER) {
      on_interest_retransmission(*dynamic_cast<ConsumerSocket *>(socket_), *retx_interest);
    }

    ConsumerInterestCallback on_interest_output = VOID_HANDLER;

    socket_->getSocketOption(ConsumerCallbacksOptions::INTEREST_OUTPUT, on_interest_output);
    if (on_interest_output != VOID_HANDLER) {
      on_interest_output(*dynamic_cast<ConsumerSocket *>(socket_), *retx_interest);
    }

    if (!is_running_) {
      // std::cout << "fastRetransmit: is not running " << std::endl; //DEBUG    
      return;
    }

    interests_in_flight_++;
    interest_retransmissions_[chunk_number % default_values::default_buffer_size]++;
    portal_->sendInterest(*retx_interest,
                          bind(&SaidTransportProtocol::onContentSegment, this, _1, _2),
                          bind(&SaidTransportProtocol::onTimeout, this, _1));
    // sendInterest()
    
    // std::cout << "Repair segment: " << chunk_number << std::endl; //DEBUG
  }
}

void SaidTransportProtocol::removeAllPendingInterests() {
  interests_in_flight_ = 0;
  portal_->clear();
}

} // end namespace transport

} // end namespace icnet
