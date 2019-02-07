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

#include "dialogue.hpp"
#include "agent.hpp"

namespace fetch {
  namespace oef {
    fetch::oef::Logger fetch::oef::DialogueAgent::logger = fetch::oef::Logger("dialogue-agent");
    fetch::oef::Logger fetch::oef::SingleDialogue::logger = fetch::oef::Logger("dialogue");
    fetch::oef::Logger fetch::oef::OEFCoreNetworkProxy::logger = fetch::oef::Logger("oefcore-network");
    fetch::oef::Logger fetch::oef::OEFCoreLocalPB::logger = fetch::oef::Logger("oefcore-local-pb");
    fetch::oef::Logger fetch::oef::MessageDecoder::logger = fetch::oef::Logger("oefcore-pb");
    fetch::oef::Logger fetch::oef::SchedulerPB::logger = fetch::oef::Logger("oefcore-scheduler-pb");
    
    SingleDialogue::SingleDialogue(DialogueAgent &agent, std::string destination)
      : agent_{agent}, destination_{std::move(destination)}, dialogueId_{Uuid32::uuid().val()},
        buyer_{true}
    {}
    
    SingleDialogue::SingleDialogue(DialogueAgent &agent, std::string destination, uint32_t dialogueId)
      : agent_{agent}, destination_{std::move(destination)}, dialogueId_{dialogueId},
        buyer_{false}
    {}
    SingleDialogue::~SingleDialogue() {
      //      agent_.unregisterDialogue(*this);
    }
    void SingleDialogue::sendMessage(uint32_t msg_id, const std::string &msg) {
      agent_.sendMessage(msg_id, dialogueId_, destination_, msg);
    }
    void SingleDialogue::sendCFP(uint32_t msgId, uint32_t target, const CFPType &constraints) {
      agent_.sendCFP(msgId,dialogueId_, destination_, target, constraints);
    }
    void SingleDialogue::sendPropose(uint32_t msgId, uint32_t target, const ProposeType &proposals) {
      agent_.sendPropose(msgId, dialogueId_, destination_, target, proposals);
    }
    void SingleDialogue::sendAccept(uint32_t msgId, uint32_t target) {
      agent_.sendAccept(msgId, dialogueId_, destination_, target);
    }
    void SingleDialogue::sendDecline(uint32_t msgId, uint32_t target) {
      agent_.sendDecline(msgId, dialogueId_, destination_, target);
    }
  }
}
