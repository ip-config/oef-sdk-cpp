#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "schema.hpp"
#include "agent.pb.h"
#include <experimental/optional>
#include <memory>
#include "mapbox/variant.hpp"

namespace var = mapbox::util; // for the variant
namespace stde = std::experimental;

namespace fetch {
  namespace oef {
    class ConnectionInterface {
    public:
      virtual void onError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation, stde::optional<uint32_t> dialogueId, stde::optional<uint32_t> msgId) = 0;
      virtual void onSearchResult(uint32_t search_id, const std::vector<std::string> &results) = 0;
    };

    class DialogueInterface {
    public:
      virtual void onMessage(const std::string &from, uint32_t dialogueId, const std::string &content) = 0;
      virtual void onCFP(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target, const CFPType &constraints) = 0;
      virtual void onPropose(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target, const ProposeType &proposals) = 0;
      virtual void onAccept(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target) = 0;
      virtual void onDecline(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target) = 0;
    };

    class AgentInterface : public ConnectionInterface, public DialogueInterface {
    };

    class OEFCoreInterface {
    protected:
      const std::string agentPublicKey_;
    public:
      explicit OEFCoreInterface(std::string agentPublicKey) : agentPublicKey_{std::move(agentPublicKey)} {}
      virtual bool handshake() = 0;
      virtual void registerDescription(const Instance &instance) = 0;
      virtual void registerService(const Instance &instance) = 0;
      virtual void searchAgents(uint32_t search_id, const QueryModel &model) = 0;
      virtual void searchServices(uint32_t search_id, const QueryModel &model) = 0;
      virtual void unregisterDescription() = 0;
      virtual void unregisterService(const Instance &instance) = 0;
      virtual void sendMessage(uint32_t dialogueId, const std::string &dest, const std::string &msg) = 0;
      virtual void sendCFP(uint32_t dialogueId, const std::string &dest, const CFPType &constraints, uint32_t msgId = 1, uint32_t target = 0) = 0;
      virtual void sendPropose(uint32_t dialogueId, const std::string &dest, const ProposeType &proposals, uint32_t msgId, uint32_t target) = 0;
      virtual void sendAccept(uint32_t dialogueId, const std::string &dest, uint32_t msgId, uint32_t target) = 0;
      virtual void sendDecline(uint32_t dialogueId, const std::string &dest, uint32_t msgId, uint32_t target) = 0;
      virtual void loop(AgentInterface &agent) = 0;
      virtual void stop() = 0;
      virtual ~OEFCoreInterface() = default;
      std::string getPublicKey() const { return agentPublicKey_; }
    };

    class Agent : public AgentInterface {
    private:
      std::unique_ptr<OEFCoreInterface> oefCore_;
    protected:
      explicit Agent(std::unique_ptr<OEFCoreInterface> oefCore) : oefCore_{std::move(oefCore)} {}
      void start() {
        if(oefCore_->handshake())
          oefCore_->loop(*this);
      }
    public:
      std::string getPublicKey() const { return oefCore_->getPublicKey(); }
      void registerDescription(const Instance &instance) {
        oefCore_->registerDescription(instance);
      }
      void registerService(const Instance &instance) {
        oefCore_->registerService(instance);
      }
      void searchAgents(uint32_t search_id, const QueryModel &model) {
        oefCore_->searchAgents(search_id, model);
      }
      void searchServices(uint32_t search_id, const QueryModel &model) {
        oefCore_->searchServices(search_id, model);
      }
      void unregisterService(const Instance &instance) {
        oefCore_->unregisterService(instance);
      }
      void unregisterDescription() {
        oefCore_->unregisterDescription();
      }
      void sendMessage(uint32_t dialogueId, const std::string &dest, const std::string &msg) {
        oefCore_->sendMessage(dialogueId, dest, msg);
      }
      void sendCFP(uint32_t dialogueId, const std::string &dest, const CFPType &constraints, uint32_t msgId = 1, uint32_t target = 0) {
        oefCore_->sendCFP(dialogueId, dest, constraints, msgId, target);
      }
      void sendPropose(uint32_t dialogueId, const std::string &dest, const ProposeType &proposals, uint32_t msgId, uint32_t target) {
        oefCore_->sendPropose(dialogueId, dest, proposals, msgId, target);
      }
      void sendAccept(uint32_t dialogueId, const std::string &dest, uint32_t msgId, uint32_t target) {
        oefCore_->sendAccept(dialogueId, dest, msgId, target);
      }
      void sendDecline(uint32_t dialogueId, const std::string &dest, uint32_t msgId, uint32_t target) {
        oefCore_->sendDecline(dialogueId, dest, msgId, target);
      }
      void stop() {
        oefCore_->stop();
      }
    };
  };
};
