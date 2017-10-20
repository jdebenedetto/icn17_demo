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

#include "icnet_ccnx_pending_anp_interest.h"

namespace icnet {

namespace ccnx {

PendingANPInterest::PendingANPInterest(std::shared_ptr<Interest> &interest,
                                 boost::asio::io_service &portal_io_service,
                                 const OnContentObjectCallback &on_content_object,
                                 const OnInterestTimeoutCallback &on_interest_timeout)
    : interest_(interest),
      count_(0),
      io_service_(portal_io_service),
      timer_(io_service_),
      on_content_object_callback_(on_content_object),
      on_interest_timeout_callback_(on_interest_timeout),
      received_(false),
      valid_(true) {
}

PendingANPInterest::~PendingANPInterest() {
  interest_.reset();
}

void PendingANPInterest::startCountdown(BoostCallback &cb) {
  timer_.expires_from_now(boost::posix_time::milliseconds(interest_->getInterestLifetime()));
  timer_.async_wait(cb);
}

void PendingANPInterest::restartCountdown() {
  timer_.expires_from_now(boost::posix_time::milliseconds(interest_->getInterestLifetime()));
}

void PendingANPInterest::cancelTimer() {
  timer_.cancel();
}

bool PendingANPInterest::isReceived() const {
  return received_;
}

void PendingANPInterest::setReceived() {
  received_ = true;
}

const std::shared_ptr<Interest> &PendingANPInterest::getInterest() const {
  return interest_;
}

void PendingANPInterest::setInterest(const std::shared_ptr<Interest> &interest) {
  PendingANPInterest::interest_ = interest;
}

const OnContentObjectCallback &PendingANPInterest::getOnDataCallback() const {
  return on_content_object_callback_;
}

void PendingANPInterest::setOnDataCallback(const OnContentObjectCallback &on_content_object) {
  PendingANPInterest::on_content_object_callback_ = on_content_object;
}

const OnInterestTimeoutCallback &PendingANPInterest::getOnTimeoutCallback() const {
  return on_interest_timeout_callback_;
}

void PendingANPInterest::setOnTimeoutCallback(const OnInterestTimeoutCallback &on_interest_timeout) {
  PendingANPInterest::on_interest_timeout_callback_ = on_interest_timeout;
}

void PendingANPInterest::setReceived(bool received) {
  PendingANPInterest::received_ = received;
}

bool PendingANPInterest::isValid() const {
  return valid_;
}

void PendingANPInterest::setValid(bool valid) {
  PendingANPInterest::valid_ = valid;
}

void PendingANPInterest::incCount() {
  PendingANPInterest::count_++;
  // std::cout << "INC PRcount: " << count_ << std::endl; //DEBUG
}

bool PendingANPInterest::decCount() {
  PendingANPInterest::count_--;
  // std::cout << "DEC PRcount: " << count_ << std::endl; //DEBUG
  if(count_ == 0) {
    return true;
  }
  return false;
}


} // end namespace ccnx

} // end namespace icnet