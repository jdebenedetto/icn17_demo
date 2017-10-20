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
#ifdef __ANDROID__
#include <android/log.h>
#endif

#include "icnet_ccnx_portal.h"

#define UNSET_CALLBACK 0
#define MAX_ARRAY_SIZE 16000

namespace icnet {

namespace ccnx {

Portal::Portal(std::string forwarder_ip_address, std::string forwarder_port)
    : is_running_(true),
      clear_(false),
      on_interest_callback_(UNSET_CALLBACK),
      connector_(io_service_,
                 forwarder_ip_address,
                 forwarder_port,
                 std::bind(&Portal::processIncomingMessages, this, std::placeholders::_1),
                 served_name_list_) {
  io_service_.reset();
}

Portal::~Portal() {
  connector_.close();
  stopEventsLoop();
  clear();
}

void Portal::sendInterest(const Interest &interest,
                          const OnContentObjectCallback &on_content_object,
                          const OnInterestTimeoutCallback &on_interest_timeout) {
  std::shared_ptr<Interest> _interest = const_cast<Interest &>(interest).shared_from_this();

  // Create new message
  CCNxMetaMessage *message = ccnxMetaMessage_CreateFromInterest(_interest->getWrappedStructure());

  // Send it
  connector_.send(message);
  clear_ = false;
  std::function<void(const boost::system::error_code &)> timer_callback;
  
  if(_interest->hasFlag(1)) {
    //-----------------SAID-MOD-----------------------------------------------------------
    const Name &anpName = _interest->getName().getANPName();
    std::unordered_map<Name, std::unique_ptr<PendingANPInterest>>::iterator it = pending_anp_interest_hash_table_.find(anpName);
    if(it == pending_anp_interest_hash_table_.end()) { //new ANP pending interest
      PendingANPInterest *pend_anp_interest = new PendingANPInterest(_interest, io_service_, on_content_object, on_interest_timeout);
      pending_anp_interest_hash_table_[anpName] = std::unique_ptr<PendingANPInterest>(pend_anp_interest);
      timer_callback = [this, anpName](const boost::system::error_code &ec) {
        if (clear_ || !is_running_) {
          return;
        }
        if (ec.value() != boost::system::errc::operation_canceled) {
          std::unordered_map<Name, std::unique_ptr<PendingANPInterest>>::iterator it = pending_anp_interest_hash_table_.find(anpName);
          if (it != pending_anp_interest_hash_table_.end()) {
            it->second->getOnTimeoutCallback()(*it->second->getInterest());
          }
        }
      };
      pend_anp_interest->incCount();
      pend_anp_interest->startCountdown(timer_callback);
    } else { //existing ANP pending interest
      it->second->incCount();
      // it->second->restartCountdown();
      timer_callback = [this, anpName](const boost::system::error_code &ec) {
        if (clear_ || !is_running_) {
          return;
        }
        if (ec.value() != boost::system::errc::operation_canceled) {
          std::unordered_map<Name, std::unique_ptr<PendingANPInterest>>::iterator it = pending_anp_interest_hash_table_.find(anpName);
          if (it != pending_anp_interest_hash_table_.end()) {
            it->second->getOnTimeoutCallback()(*it->second->getInterest());
          }
        }
      };
      it->second->cancelTimer();
      it->second->startCountdown(timer_callback);
    }
    //-----------------SAID-MOD------------------------------------------------------------------
  } else {
    //------------------ORIGINAL----------------------------------------------------------------------------------------
    PendingInterest *pend_interest = new PendingInterest(_interest, io_service_, on_content_object, on_interest_timeout);
    const Name &name = _interest->getName();
    pending_interest_hash_table_[name] = std::unique_ptr<PendingInterest>(pend_interest);
    timer_callback = [this, name](const boost::system::error_code &ec) {
      if (clear_ || !is_running_) {
        return;
      }

      if (ec.value() != boost::system::errc::operation_canceled) {
        std::unordered_map<Name, std::unique_ptr<PendingInterest>>::iterator it = pending_interest_hash_table_.find(name);
        if (it != pending_interest_hash_table_.end()) {
          it->second->getOnTimeoutCallback()(*it->second->getInterest());
        }
      }
    };

    pend_interest->startCountdown(timer_callback);
    //------------------ORIGINAL----------------------------------------------------------------------------------------
  }

  ccnxMetaMessage_Release(&message);
}

void Portal::bind(Name &name, const OnInterestCallback &on_interest_callback) {
  on_interest_callback_ = on_interest_callback;
  served_name_list_.push_back(name);
  work_ = std::shared_ptr<boost::asio::io_service::work>(new boost::asio::io_service::work(io_service_));
  connector_.bind(name);
}

void Portal::sendContentObject(const ContentObject &content_object) {
  ContentObject &ccnx_data = const_cast<ContentObject &>(content_object);
  CCNxMetaMessageStructure *message = ccnxMetaMessage_CreateFromContentObject(ccnx_data.getWrappedStructure());

  ccnxContentObject_AssertValid(ccnx_data.getWrappedStructure());

  connector_.send(message);

  ccnxMetaMessage_Release(&message);
}

void Portal::runEventsLoop() {
  if (io_service_.stopped()) {
    io_service_.reset(); // ensure that run()/poll() will do some work
  }

  is_running_ = true;
  this->io_service_.run();
}

void Portal::stopEventsLoop() {
  is_running_ = false;
  work_.reset();
  io_service_.stop();
}

void Portal::clear() {
  pending_interest_hash_table_.clear();
  pending_anp_interest_hash_table_.clear(); //SAID
  clear_ = true;
}

void Portal::processInterest(CCNxMetaMessage *response) {
  // Interest for a producer
  CCNxInterest *interest_ptr = ccnxInterest_Acquire(ccnxMetaMessage_GetInterest(response));

  if (on_interest_callback_ != UNSET_CALLBACK) {

    Interest interest(interest_ptr);
    if (on_interest_callback_) {
      on_interest_callback_(interest.getName(), interest);
    }
    ccnxInterest_Release((CCNxInterest **) &interest_ptr);
  }
}

void Portal::processControlMessage(CCNxMetaMessage *response) {
  // Control message as response to the route set by a producer

  CCNxControl *control_message = ccnxMetaMessage_GetControl(response);

  if (ccnxControl_IsACK(control_message)) {
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_DEBUG, "libICNet", "Route set correctly!\n");
#else
    std::cout << "Route set correctly!" << std::endl;
#endif
  } else {
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_DEBUG, "libICNet", "Failed to set the route.\n");
#else
    std::cout << "Failed to set the route." << std::endl;
#endif
  }
}

void Portal::processContentObject(CCNxMetaMessage *response) {
  // Content object for a consumer

  CCNxContentObject *content_object = ccnxContentObject_Acquire(ccnxMetaMessage_GetContentObject(response));
  CCNxName *n = ccnxContentObject_GetName(content_object);

  PendingInterestHashTable::iterator it = pending_interest_hash_table_.find(Name(n));

  if (it != pending_interest_hash_table_.end()) {

    std::unique_ptr<PendingInterest> &interest_ptr = it->second;

    std::cout << "ContentObject: " << ccnxName_ToString(n) << std::endl; //DEBUG
    std::cout << "for Interest: " << interest_ptr->getInterest()->getName() << std::endl; //DEBUG

    interest_ptr->cancelTimer();
    std::shared_ptr<ContentObject> data_ptr = std::make_shared<ContentObject>(content_object);

    if (!interest_ptr->isReceived()) {
      interest_ptr->setReceived();
      interest_ptr->getOnDataCallback()(*interest_ptr->getInterest(), *data_ptr);
      pending_interest_hash_table_.erase(interest_ptr->getInterest()->getName());
    }
  }
  //-----------------SAID-MOD----------------------------------------------------
  else {
    PendingANPInterestHashTable::iterator it = pending_anp_interest_hash_table_.find(Name(n).getANPName());
    if (it != pending_anp_interest_hash_table_.end()) {
      
      std::unique_ptr<PendingANPInterest> &interest_ptr = it->second;

      std::cout << "ContentObject: " << ccnxName_ToString(n) << std::endl; //DEBUG
      std::cout << "for ANP Interest: " << interest_ptr->getInterest()->getName() << std::endl; //DEBUG

      // interest_ptr->cancelTimer();

      std::shared_ptr<ContentObject> data_ptr = std::make_shared<ContentObject>(content_object);

      if (!interest_ptr->isReceived()) {
        // std::cout << "interest_ptr is not received " << ccnxName_ToString(n) << std::endl; //DEBUG
        interest_ptr->getOnDataCallback()(*interest_ptr->getInterest(), *data_ptr);
        if(interest_ptr->decCount()) {
          interest_ptr->setReceived();
          interest_ptr->cancelTimer();
          pending_anp_interest_hash_table_.erase(Name(n).getANPName());
        }
      }
    }
  }
  //-----------------SAID-MOD----------------------------------------------------

  ccnxContentObject_Release((CCNxContentObject **) &content_object);
}

void Portal::processIncomingMessages(CCNxMetaMessage *response) {
  if (clear_ || !is_running_) {
    return;
  }

  if (response) {
    if (ccnxMetaMessage_IsContentObject(response)) {
      processContentObject(response);
    } else if (ccnxMetaMessage_IsInterest(response)) {
      processInterest(response);
    } else if (ccnxMetaMessage_IsControl(response)) {
      processControlMessage(response);
    }
    ccnxMetaMessage_Release(&response);
  }

}

boost::asio::io_service &Portal::getIoService() {
  return io_service_;
}

} // end namespace ccnx

} // end namespace icnet