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

#include "agent.pb.h"
#include "schema.hpp"

namespace fetch {
  namespace oef {

    class Register {
    private:
      fetch::oef::pb::Envelope envelope_;
    public:
      explicit Register(const Instance &instance) {
        auto *reg = envelope_.mutable_register_service();
        auto *inst = reg->mutable_description();
        inst->CopyFrom(instance.handle());
      }
      const fetch::oef::pb::Envelope &handle() const { return envelope_; }
    };
    
    class Unregister {
    private:
      fetch::oef::pb::Envelope envelope_;
    public:
      explicit Unregister(const Instance &instance) {
        auto *reg = envelope_.mutable_unregister_service();
        auto *inst = reg->mutable_description();
        inst->CopyFrom(instance.handle());
      }
      const fetch::oef::pb::Envelope &handle() const { return envelope_; }
    };
    
    class UnregisterDescription {
    private:
      fetch::oef::pb::Envelope envelope_;
    public:
      explicit UnregisterDescription() {
        (void) envelope_.mutable_unregister_description();
      }
      const fetch::oef::pb::Envelope &handle() const { return envelope_; }
    };
    
    class SearchServices {
    private:
      fetch::oef::pb::Envelope envelope_;
    public:
      explicit SearchServices(uint32_t search_id, const QueryModel &model) {
        auto *desc = envelope_.mutable_search_services();
        desc->set_search_id(search_id);
        auto *mod = desc->mutable_query();
        mod->CopyFrom(model.handle());
      }
      const fetch::oef::pb::Envelope &handle() const { return envelope_; }
    };
    
    class Message {
    private:
      fetch::oef::pb::Envelope envelope_;
    public:
      explicit Message(uint32_t dialogueId, const std::string &dest, const std::string &msg) {
        auto *message = envelope_.mutable_send_message();
        message->set_dialogue_id(dialogueId);
        message->set_destination(dest);
        message->set_content(msg);
      }
      const fetch::oef::pb::Envelope &handle() const { return envelope_; }
    };
    
    class CFP {
    private:
      fetch::oef::pb::Envelope envelope_;
    public:
      explicit CFP(uint32_t dialogueId, const std::string &dest, const fetch::oef::CFPType &query, uint32_t msgId = 1, uint32_t target = 0) {
        auto *message = envelope_.mutable_send_message();
        message->set_dialogue_id(dialogueId);
        message->set_destination(dest);
        auto *fipa_msg = message->mutable_fipa();
        fipa_msg->set_msg_id(msgId);
        fipa_msg->set_target(target);
        auto *cfp = fipa_msg->mutable_cfp();
        query.match(
                    [cfp](const std::string &content) { cfp->set_content(content); },
                    [cfp](const QueryModel &query) { auto *q = cfp->mutable_query(); q->CopyFrom(query.handle());},
                    [cfp](stde::nullopt_t) { (void)cfp->mutable_nothing(); } );
      }
      fetch::oef::pb::Envelope &handle() { return envelope_; }
    };
    
    class Propose {
    private:
      fetch::oef::pb::Envelope envelope_;
    public:
      explicit Propose(uint32_t dialogueId, const std::string &dest, const fetch::oef::ProposeType &proposals, uint32_t msgId, uint32_t target) {
        auto *message = envelope_.mutable_send_message();
        message->set_dialogue_id(dialogueId);
        message->set_destination(dest);
        auto *fipa_msg = message->mutable_fipa();
        fipa_msg->set_msg_id(msgId);
        fipa_msg->set_target(target);
        auto *props = fipa_msg->mutable_propose();
        proposals.match(
                        [props](const std::string &content) { props->set_content(content); },
                        [props](const std::vector<Instance> &instances) {
                          auto *p = props->mutable_proposals();
                          auto *objs = p->mutable_objects();
                          objs->Reserve(instances.size());
                          for(auto &instance: instances) {
                            auto *inst = objs->Add();
                            inst->CopyFrom(instance.handle());
                          }
                        });
      }
      fetch::oef::pb::Envelope &handle() { return envelope_; }
    };
    
    class Accept {
    private:
      fetch::oef::pb::Envelope envelope_;
    public:
      explicit Accept(uint32_t dialogueId, const std::string &dest, uint32_t msgId, uint32_t target) {
        auto *message = envelope_.mutable_send_message();
        message->set_dialogue_id(dialogueId);
        message->set_destination(dest);
        auto *fipa_msg = message->mutable_fipa();
        fipa_msg->set_msg_id(msgId);
        fipa_msg->set_target(target);
        (void) fipa_msg->mutable_accept();
      }
      fetch::oef::pb::Envelope &handle() { return envelope_; }
    };
    
    class Decline {
    private:
      fetch::oef::pb::Envelope envelope_;
    public:
      explicit Decline(uint32_t dialogueId, const std::string &dest, uint32_t msgId, uint32_t target) {
        auto *message = envelope_.mutable_send_message();
        message->set_dialogue_id(dialogueId);
        message->set_destination(dest);
        auto *fipa_msg = message->mutable_fipa();
        fipa_msg->set_msg_id(msgId);
        fipa_msg->set_target(target);
        (void) fipa_msg->mutable_decline();
      }
      fetch::oef::pb::Envelope &handle() { return envelope_; }
    };
    
    
    class SearchAgents {
    private:
      fetch::oef::pb::Envelope envelope_;
    public:
      explicit SearchAgents(uint32_t search_id, const QueryModel &model) {
        auto *desc = envelope_.mutable_search_agents();
        desc->set_search_id(search_id);
        auto *mod = desc->mutable_query();
        mod->CopyFrom(model.handle());
      }
      const fetch::oef::pb::Envelope &handle() const { return envelope_; }
    };
    
    class Description {
    private:
      fetch::oef::pb::Envelope envelope_;
    public:
      explicit Description(const Instance &instance) {
        auto *desc = envelope_.mutable_register_description();
        auto *inst = desc->mutable_description();
        inst->CopyFrom(instance.handle());
      }
      const fetch::oef::pb::Envelope &handle() const { return envelope_; }
    };
  };
};
