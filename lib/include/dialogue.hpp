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
  const uint32_t dialogueId;

  DialogueKey(std::string dest, uint32_t id) : destination{std::move(dest)}, dialogueId{id} {}
  size_t hash() const {
    return hash_combine(std::hash<std::string>{}(destination), dialogueId);
  }
  bool operator==(const DialogueKey &key) const {
    return key.destination == destination && key.dialogueId == dialogueId;
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
      virtual void onMessage(uint32_t msgId, const std::string &content) = 0;
      virtual void onCFP(uint32_t msgId, uint32_t target, const CFPType &constraints) = 0;
      virtual void onPropose(uint32_t msgId, uint32_t target, const ProposeType &proposals) = 0;
      virtual void onAccept(uint32_t msgId, uint32_t target) = 0;
      virtual void onDecline(uint32_t msgId, uint32_t target) = 0;
      virtual void onDialogueError(uint32_t answerId, uint32_t dialogueId, const std::string &origin) = 0;
      void sendMessage(uint32_t msgId, const std::string &msg);
      void sendCFP(uint32_t msgId, uint32_t target, const CFPType &constraints);
      void sendPropose(uint32_t msgId, uint32_t target, const ProposeType &proposals);
      void sendAccept(uint32_t msgId, uint32_t target);
      void sendDecline(uint32_t msgId, uint32_t target);
    };

    class DialogueAgent : public Agent {
    protected:
      std::unordered_map<DialogueKey, std::shared_ptr<SingleDialogue>> dialogues_;

      static fetch::oef::Logger logger;

    public:
      explicit DialogueAgent(std::unique_ptr<OEFCoreInterface> oefCore) : Agent{std::move(oefCore)} {}
      virtual ~DialogueAgent() = default;
      virtual void onNewMessage(uint32_t msgId, uint32_t dialogueId, const std::string &from, const std::string &content) = 0;
      virtual void onNewCFP(uint32_t msgId, uint32_t dialogueId, const std::string &from, uint32_t target, const CFPType &constraints) = 0;
      void onDialogueError(uint32_t answerId, uint32_t dialogueId, const std::string &origin) override {
        auto iter = dialogues_.find(DialogueKey{origin, dialogueId});
        if(iter == dialogues_.end()) {
          logger.error("onDialogueError: dialogue {} from {} msg_id {} not found.", dialogueId, origin, answerId);
        } else {
          iter->second->onDialogueError(answerId, dialogueId, origin);
        }
      }
      
      void onMessage(uint32_t msgId, uint32_t dialogueId, const std::string &from, const std::string &content) override {
        auto iter = dialogues_.find(DialogueKey{from, dialogueId});
        if(iter == dialogues_.end()) {
          this->onNewMessage(msgId, dialogueId, from, content);
        } else {
          iter->second->onMessage(msgId, content);
        }
      }
      void onCFP(uint32_t msgId, uint32_t dialogueId, const std::string &from, uint32_t target, const CFPType &constraints) override {
        auto iter = dialogues_.find(DialogueKey{from, dialogueId});
        if(iter == dialogues_.end()) {
          onNewCFP(msgId, dialogueId, from, target, constraints);
        } else {
          iter->second->onCFP(msgId, target, constraints);
        }
      }
      void onPropose(uint32_t msgId, uint32_t dialogueId, const std::string &from, uint32_t target, const ProposeType &proposals) override {
        auto iter = dialogues_.find(DialogueKey{from, dialogueId});
        if(iter == dialogues_.end()) {
          logger.error("onPropose: dialogue {} {} not found.", from, dialogueId);
        } else {
          iter->second->onPropose(msgId, target, proposals);
        }
      }
      void onAccept(uint32_t msgId, uint32_t dialogueId, const std::string &from, uint32_t target) override {
        auto iter = dialogues_.find(DialogueKey{from, dialogueId});
        if(iter == dialogues_.end()) {
          logger.error("onAccept: dialogue {} {} not found.", from, dialogueId);
        } else {
          iter->second->onAccept(msgId, target);
        }
      }
      void onDecline(uint32_t msgId, uint32_t dialogueId, const std::string &from, uint32_t target) override {
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

    class GroupDialogues {
    protected:
      DialogueAgent &agent_;
      std::unordered_map<std::string, std::shared_ptr<SingleDialogue>> dialogues_;
      std::string bestAgent_;
      uint64_t bestPrice_;
      size_t nbAnswers_;
      bool first_ = true;
    public:
      GroupDialogues(DialogueAgent &agent) : agent_{agent} {}
      virtual ~GroupDialogues() {
        for(auto &p : dialogues_) {
          agent_.unregisterDialogue(*p.second);
        }
      }
      void addAgents(const std::vector<std::shared_ptr<SingleDialogue>> &agents) {
        for(const auto &a : agents) {
          dialogues_[a->destination()] = a;
          agent_.registerDialogue(a);
        }
      }
      virtual bool better(uint64_t price1, uint64_t price2) const = 0;
      void update(const std::string &agent, uint64_t price) {
        ++nbAnswers_;
        if(first_) {
          first_ = false;
          bestPrice_ = price;
          bestAgent_ = agent;
        } else {
          if(better(price, bestPrice_)) {
            bestPrice_ = price;
            bestAgent_ = agent;
          }
        }
        if(nbAnswers_ >= dialogues_.size()) {
          finished();
        }
      }
      virtual void finished() = 0;
      std::string bestAgent() const { return bestAgent_; }
      uint64_t bestPrice() const { return bestPrice_; }
    };
  } // namespace oef
} // namespace fetch
