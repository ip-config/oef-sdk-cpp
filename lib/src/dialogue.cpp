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
      agent_.unregisterDialogue(*this);
    }
    void SingleDialogue::sendMessage(const std::string &msg) {
      agent_.sendMessage(dialogueId_, destination_, msg);
    }
    void SingleDialogue::sendCFP(const CFPType &constraints, uint32_t msgId, uint32_t target) {
      agent_.sendCFP(dialogueId_, destination_, constraints, msgId, target);
    }
    void SingleDialogue::sendPropose(const ProposeType &proposals, uint32_t msgId, uint32_t target) {
      agent_.sendPropose(dialogueId_, destination_, proposals, msgId, target);
    }
    void SingleDialogue::sendAccept(uint32_t msgId, uint32_t target) {
      agent_.sendAccept(dialogueId_, destination_, msgId, target);
    }
    void SingleDialogue::sendDecline(uint32_t msgId, uint32_t target) {
      agent_.sendDecline(dialogueId_, destination_, msgId, target);
    }
  }
}
