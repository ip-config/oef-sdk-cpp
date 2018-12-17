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

#include "oefcoreproxy.hpp"
#include "uuid.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include "logger.hpp"

constexpr size_t hash_combine(size_t lhs, size_t rhs ) {
  lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
  return lhs;
}

struct DialogueKey {
  const std::string destination;
  const uint32_t dialogue_id;

  DialogueKey(std::string dest, uint32_t id) : destination{std::move(dest)}, dialogue_id{id} {}
  size_t hash() const {
    return hash_combine(std::hash<std::string>{}(destination), dialogue_id);
  }
  bool operator==(const DialogueKey &key) const {
    return key.destination == destination && key.dialogue_id == dialogue_id;
  }
};

namespace std
{  
  template<> struct hash<DialogueKey>  {
    size_t operator()(DialogueKey const& s) const noexcept
    {
      return s.hash();
    }
  };
} // namespace std

namespace fetch {
  namespace oef {

    class DialogueAgent;
    // TODO protocol class: msgId ...
    class SingleDialogue {
    protected:
      DialogueAgent &agent_;
      const std::string destination_;
      const uint32_t dialogueId_;
      bool buyer_;
      static fetch::oef::Logger logger;
    public:
      SingleDialogue(DialogueAgent &agent, std::string destination);
      SingleDialogue(DialogueAgent &agent, std::string destination, uint32_t dialogueId);
      virtual ~SingleDialogue();
      std::string destination() const { return destination_; }
      uint32_t id() const { return dialogueId_; } 
      virtual void onMessage(const std::string &content) = 0;
      virtual void onCFP(uint32_t msgId, uint32_t target, const CFPType &constraints) = 0;
      virtual void onPropose(uint32_t msgId, uint32_t target, const ProposeType &proposals) = 0;
      virtual void onAccept(uint32_t msgId, uint32_t target) = 0;
      virtual void onDecline(uint32_t msgId, uint32_t target) = 0;
      void sendMessage(const std::string &msg);
      void sendCFP(const CFPType &constraints, uint32_t msgId = 1, uint32_t target = 0);
      void sendPropose(const ProposeType &proposals, uint32_t msgId, uint32_t target);
      void sendAccept(uint32_t msgId, uint32_t target);
      void sendDecline(uint32_t msgId, uint32_t target);
    };

    class DialogueAgent : public Agent {
    protected:
      std::unordered_map<DialogueKey, std::shared_ptr<SingleDialogue>> dialogues_;

      static fetch::oef::Logger logger;

    public:
      explicit DialogueAgent(std::unique_ptr<OEFCoreInterface> oefCore) : Agent{std::move(oefCore)} {}
      virtual void onNewMessage(const std::string &from, uint32_t dialogueId, const std::string &content) = 0;
      virtual void onNewCFP(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target, const CFPType &constraints) = 0;
      virtual void onConnectionError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation) = 0;
      
      void onError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation, stde::optional<uint32_t> dialogueId, stde::optional<uint32_t> msgId) override {
        assert(false); // protocol buffer is wrong !!! I need the origin
        // if(dialogue_id) { // it concerns a SingleDialogue
        //   auto iter = dialogues_.find(DialogueKey{from, dialogue_id.});
        // if(iter == dialogues_.end()) {
        //   logger.error("onMessage: dialogue {} {} not found.", from, dialogue_id);
        // } else {
        //   iter->second->onMessage(content);
        // }
      }
      
      void onMessage(const std::string &from, uint32_t dialogueId, const std::string &content) override {
        auto iter = dialogues_.find(DialogueKey{from, dialogueId});
        if(iter == dialogues_.end()) {
          logger.error("onMessage: dialogue {} {} not found.", from, dialogueId);
        } else {
          iter->second->onMessage(content);
        }
      }
      void onCFP(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target, const CFPType &constraints) override {
        auto iter = dialogues_.find(DialogueKey{from, dialogueId});
        if(iter == dialogues_.end()) {
          logger.error("onCFP: dialogue {} {} not found.", from, dialogueId);
        } else {
          iter->second->onCFP(msgId, target, constraints);
        }
      }
      void onPropose(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target, const ProposeType &proposals) override {
        auto iter = dialogues_.find(DialogueKey{from, dialogueId});
        if(iter == dialogues_.end()) {
          logger.error("onPropose: dialogue {} {} not found.", from, dialogueId);
        } else {
          iter->second->onPropose(msgId, target, proposals);
        }
      }
      void onAccept(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target) override {
        auto iter = dialogues_.find(DialogueKey{from, dialogueId});
        if(iter == dialogues_.end()) {
          logger.error("onAccept: dialogue {} {} not found.", from, dialogueId);
        } else {
          iter->second->onAccept(msgId, target);
        }
      }
      void onDecline(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target) override {
        auto iter = dialogues_.find(DialogueKey{from, dialogueId});
        if(iter == dialogues_.end()) {
          logger.error("onDecline: dialogue {} {} not found.", from, dialogueId);
        } else {
          iter->second->onDecline(msgId, target);
        }
      }
      bool registerDialogue(std::shared_ptr<SingleDialogue> dialogue) {
        // should be thread safe
        DialogueKey key{dialogue->destination(), dialogue->id()};
        auto iter = dialogues_.find(key);
        if(iter == dialogues_.end()) {
          dialogues_.insert({key, dialogue});
          return true;
        }
        return false;
      }
      bool unregisterDialogue(SingleDialogue &dialogue) {
        // should be thread safe
        DialogueKey key{dialogue.destination(), dialogue.id()};
        auto iter = dialogues_.find(key);
        if(iter == dialogues_.end()) {
          return false;
        }
        dialogues_.erase(iter);
        return true;
      }
    };
  } // namespace oef
} // namespace fetch
