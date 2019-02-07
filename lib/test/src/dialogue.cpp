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

#include "catch.hpp"
#include "server.hpp"
#include <iostream>
#include <chrono>
#include <future>
#include <cassert>
#include <dialogue.hpp>
#include "uuid.hpp"
#include "oefcoreproxy.hpp"
#include "agent.hpp"

using namespace fetch::oef;

enum class AgentAction {
  NONE,
  ON_OEFERROR,
  ON_DIALOGUE_ERROR,
  ON_SEARCH_RESULT,
  ON_MESSAGE,
  ON_CFP,
  ON_PROPOSE,
  ON_ACCEPT,
  ON_DECLINE
};

class SimpleSingleDialogue;
class SimpleDialogueAgent : public fetch::oef::DialogueAgent {
private:
  std::string from_;
  uint32_t dialogueId_;
  std::string content_;
  AgentAction action_ = AgentAction::NONE;
public:
  SimpleDialogueAgent(std::unique_ptr<OEFCoreInterface> oefCore)
  : fetch::oef::DialogueAgent{std::move(oefCore)}
  {
    start();
  }

  const std::string from() const { return from_; }
  uint32_t dialogueId() const { return dialogueId_; }
  const std::string content() const { return content_; }
  AgentAction action() const { return action_; }
  const long nbDialogues() const { return dialogues_.size(); }
  const std::shared_ptr<SimpleSingleDialogue> getDialogue(std::string &destination, uint32_t dialogueId) const {
    DialogueKey dk{destination, dialogueId};
    auto iter = dialogues_.find(dk);
    if (iter == dialogues_.end()){
      return nullptr;
    }
    return (std::shared_ptr<SimpleSingleDialogue>&) iter->second;
  }


  void onNewMessage(uint32_t msgId, uint32_t dialogueId, const std::string &from, const std::string &content) override;
  void onNewCFP(uint32_t msgId, uint32_t dialogueId, const std::string &from, uint32_t target, const CFPType &constraints) override;
  void onOEFError(uint32_t answerId, fetch::oef::pb::Server_AgentMessage_OEFError_Operation operation) override {
    action_ = AgentAction::ON_OEFERROR;
  }
  void onDialogueError(uint32_t answerId, uint32_t dialogueId, const std::string &origin) override {
    from_ = origin;
    dialogueId_ = dialogueId;
    action_ = AgentAction::ON_DIALOGUE_ERROR;
    fetch::oef::DialogueAgent::onDialogueError(answerId, dialogueId, origin);
  }
  void onSearchResult(uint32_t searchId, const std::vector<std::string> &results) override {
    action_ = AgentAction::ON_SEARCH_RESULT;
  }
  void onMessage(uint32_t msgId, uint32_t dialogueId, const std::string &from, const std::string &content) override {
    from_ = from;
    dialogueId_ = dialogueId;
    content_ = content;
    action_ = AgentAction::ON_MESSAGE;
    fetch::oef::DialogueAgent::onMessage(msgId, dialogueId, from, content);
  }
  void onCFP(uint32_t msgId, uint32_t dialogueId, const std::string &from, uint32_t target, const fetch::oef::CFPType &constraints) override {
    from_ = from;
    dialogueId_ = dialogueId;
    action_ = AgentAction::ON_CFP;
    fetch::oef::DialogueAgent::onCFP(msgId, dialogueId, from, target, constraints);
  }
  void onPropose(uint32_t msgId, uint32_t dialogueId, const std::string &from, uint32_t target, const fetch::oef::ProposeType &proposals) override {
    from_ = from;
    dialogueId_ = dialogueId;
    action_ = AgentAction::ON_PROPOSE;
    fetch::oef::DialogueAgent::onPropose(msgId, dialogueId, from, target, proposals);
  }
  void onAccept(uint32_t msgId, uint32_t dialogueId, const std::string &from, uint32_t target) override {
    from_ = from;
    dialogueId_ = dialogueId;
    action_ = AgentAction::ON_ACCEPT;
    fetch::oef::DialogueAgent::onAccept(msgId, dialogueId, from, target);
  }
  void onDecline(uint32_t msgId, uint32_t dialogueId, const std::string &from, uint32_t target) override {
    from_ = from;
    dialogueId_ = dialogueId;
    action_ = AgentAction::ON_DECLINE;
    fetch::oef::DialogueAgent::onDecline(msgId, dialogueId, from, target);
  }


};

class SimpleSingleDialogue : public fetch::oef::SingleDialogue {
private:
  std::string content_;
  AgentAction action_ = AgentAction::NONE;
public:
  const std::string content() const { return content_; }
  AgentAction action() const { return action_; }
  SimpleSingleDialogue(SimpleDialogueAgent &agent, std::string destination)
  :  fetch::oef::SingleDialogue(agent, std::move(destination), Uuid32::uuid().val()) {};

  SimpleSingleDialogue(SimpleDialogueAgent &agent, std::string destination, uint32_t dialogueId)
  :  fetch::oef::SingleDialogue(agent, std::move(destination), dialogueId) {};;

  void onMessage(uint32_t msgId, const std::string &content) override {
    content_ = content;
    action_ = AgentAction::ON_MESSAGE;
  };
  void onCFP(uint32_t msgId, uint32_t target, const CFPType &constraints) override {
    action_ = AgentAction::ON_CFP;
  };
  void onPropose(uint32_t msgId, uint32_t target, const ProposeType &proposals) override {
    action_ = AgentAction::ON_PROPOSE;
  };
  void onAccept(uint32_t msgId, uint32_t target) override {
    action_ = AgentAction::ON_ACCEPT;
  };
  void onDecline(uint32_t msgId, uint32_t target) override {
    action_ = AgentAction::ON_DECLINE;
  };
  void onDialogueError(uint32_t answerId, uint32_t dialogueId, const std::string &origin) override {
    action_ = AgentAction::ON_DIALOGUE_ERROR;
  };
};


void SimpleDialogueAgent::onNewMessage(
        uint32_t msgId,
        uint32_t dialogueId,
        const std::string &from,
        const std::string &content) {
  if(!registerDialogue(std::make_shared<SimpleSingleDialogue>(*this, from, dialogueId))){
    logger.error("onNewMessage: failed to register dialogue {} {}", from, dialogueId);
  }
//  auto iter = dialogues_.find(DialogueKey{from, dialogueId});
//  iter->second->onMessage(content);
  this->onMessage(msgId, dialogueId, from, content);
}

void SimpleDialogueAgent::onNewCFP(
        uint32_t msgId,
        uint32_t dialogueId,
        const std::string &from,
        uint32_t target,
        const CFPType &constraints){
  if(!registerDialogue(std::make_shared<SimpleSingleDialogue>(*this, from, dialogueId))){
    logger.error("onNewCFP: failed to register dialogue {} {}", from, dialogueId);
  }
//  auto iter = dialogues_.find(DialogueKey{from, dialogueId});
//  iter->second->onCFP(msgId, target, constraints);
  this->onCFP(msgId, dialogueId, from, target, constraints);
}

namespace Test {

  TEST_CASE("transfer between dialogue agents", "[dialogue]"){

    // start the server
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%n] [%l] %v");
    spdlog::set_level(spdlog::level::level_enum::trace);
    Uuid id = Uuid::uuid4();
    std::cerr << id.to_string() << std::endl;
    std::string s = id.to_string();
    Uuid id2{s};
    std::cerr << id2.to_string() << std::endl;
    Server as;
    std::cerr << "Server created\n";
    as.run();
    std::cerr << "Server started\n";

    IoContextPool pool(2);
    pool.run();

    std::string pkAgent1 = "DialogueAgent1";
    std::string pkAgent2 = "DialogueAgent2";
    std::string pkAgent3 = "DialogueAgent3";

    auto proxy1 = std::unique_ptr<fetch::oef::OEFCoreInterface>(new fetch::oef::OEFCoreNetworkProxy{pkAgent1, pool.getIoContext(), "127.0.0.1"});
    auto proxy2 = std::unique_ptr<fetch::oef::OEFCoreInterface>(new fetch::oef::OEFCoreNetworkProxy{pkAgent2, pool.getIoContext(), "127.0.0.1"});
    auto proxy3 = std::unique_ptr<fetch::oef::OEFCoreInterface>(new fetch::oef::OEFCoreNetworkProxy{pkAgent3, pool.getIoContext(), "127.0.0.1"});

    SimpleDialogueAgent c1(std::move(proxy1));
    SimpleDialogueAgent c2(std::move(proxy2));
    SimpleDialogueAgent c3(std::move(proxy3));

    std::cerr << "Clients created\n";
    REQUIRE(as.nbAgents() == 3);

    std::shared_ptr<SimpleSingleDialogue> dialogue;
    {
      //Receiving a simple message for the first time creates a new dialogue.
      auto d = std::shared_ptr<SimpleSingleDialogue>(new SimpleSingleDialogue{c1, pkAgent2});
      c1.registerDialogue(d);
      d->sendMessage(1, "Hello world");
      std::this_thread::sleep_for(std::chrono::milliseconds{250});
      REQUIRE(c2.action() == AgentAction::ON_MESSAGE);
      REQUIRE(c2.from() == pkAgent1);
      REQUIRE(c2.dialogueId() == d->id());

      dialogue = c2.getDialogue(pkAgent1, d->id());
      REQUIRE(dialogue != nullptr);
      REQUIRE(dialogue->id() == d->id());
      REQUIRE(dialogue->action() == AgentAction::ON_MESSAGE);
      dialogue.reset();
      c1.unregisterDialogue(*d);
    }

    //Receiving a CFP for the first time creates a new dialogue.
    //The other negotiation messages are correctly dispatched to the right dialogue's callback.
    {
      auto d = std::shared_ptr<SimpleSingleDialogue>(new SimpleSingleDialogue{c1, pkAgent2});
      c1.registerDialogue(d);
      d->sendCFP(1, 0, stde::nullopt);
      std::this_thread::sleep_for(std::chrono::milliseconds{250});
      REQUIRE(c2.action() == AgentAction::ON_CFP);
      REQUIRE(c2.from() == pkAgent1);
      REQUIRE(c2.dialogueId() == d->id());
      dialogue = c2.getDialogue(pkAgent1, d->id());
      REQUIRE(dialogue != nullptr);
      REQUIRE(dialogue->id() == d->id());
      REQUIRE(dialogue->action() == AgentAction::ON_CFP);

      dialogue->sendPropose(2, 1, "proposal");
      std::this_thread::sleep_for(std::chrono::milliseconds{250});
      REQUIRE(c1.action() == AgentAction::ON_PROPOSE);
      REQUIRE(c1.from() == pkAgent2);
      REQUIRE(c1.dialogueId() == d->id());
      dialogue = c1.getDialogue(pkAgent2, d->id());
      REQUIRE(dialogue != nullptr);
      REQUIRE(dialogue->id() == d->id());
      REQUIRE(dialogue->action() == AgentAction::ON_PROPOSE);

      d->sendAccept(3, 2);
      std::this_thread::sleep_for(std::chrono::milliseconds{250});
      REQUIRE(c2.action() == AgentAction::ON_ACCEPT);
      REQUIRE(c2.from() == pkAgent1);
      REQUIRE(c2.dialogueId() == d->id());
      dialogue = c2.getDialogue(pkAgent1, d->id());
      REQUIRE(dialogue != nullptr);
      REQUIRE(dialogue->id() == d->id());
      REQUIRE(dialogue->action() == AgentAction::ON_ACCEPT);

      dialogue->sendDecline(4, 3);
      std::this_thread::sleep_for(std::chrono::milliseconds{250});
      REQUIRE(c1.action() == AgentAction::ON_DECLINE);
      REQUIRE(c1.from() == pkAgent2);
      REQUIRE(c1.dialogueId() == d->id());
      dialogue = c1.getDialogue(pkAgent2, d->id());
      REQUIRE(dialogue != nullptr);
      REQUIRE(dialogue->id() == d->id());
      REQUIRE(dialogue->action() == AgentAction::ON_DECLINE);

      c1.unregisterDialogue(*d);
      dialogue.reset();
    }

    //Receiving a 'Propose' as a first message in a dialogue should log an error.
    {
      auto d = std::shared_ptr<SimpleSingleDialogue>(new SimpleSingleDialogue{c1, pkAgent2});
      c1.registerDialogue(d);
      d->sendPropose(1, 0, "proposal");
      std::this_thread::sleep_for(std::chrono::milliseconds{250});
      REQUIRE(c2.action() == AgentAction::ON_PROPOSE);
      REQUIRE(c2.from() == pkAgent1);
      REQUIRE(c2.dialogueId() == d->id());

      dialogue = c2.getDialogue(pkAgent1, d->id());
      REQUIRE(dialogue == nullptr);
      c1.unregisterDialogue(*d);
      dialogue.reset();
    }

    //Receiving an 'Accept' as a first message in a dialogue should log an error.
    {
      auto d = std::shared_ptr<SimpleSingleDialogue>(new SimpleSingleDialogue{c1, pkAgent2});
      c1.registerDialogue(d);
      d->sendAccept(1, 0);
      std::this_thread::sleep_for(std::chrono::milliseconds{250});
      REQUIRE(c2.action() == AgentAction::ON_ACCEPT);
      REQUIRE(c2.from() == pkAgent1);
      REQUIRE(c2.dialogueId() == d->id());

      dialogue = c2.getDialogue(pkAgent1, d->id());
      REQUIRE(dialogue == nullptr);
      c1.unregisterDialogue(*d);
      dialogue.reset();
    }

    //Receiving a 'Decline' as a first message in a dialogue should log an error.
    {
      auto d = std::shared_ptr<SimpleSingleDialogue>(new SimpleSingleDialogue{c1, pkAgent2});
      c1.registerDialogue(d);
      d->sendDecline(1, 0);
      std::this_thread::sleep_for(std::chrono::milliseconds{250});
      REQUIRE(c2.action() == AgentAction::ON_DECLINE);
      REQUIRE(c2.from() == pkAgent1);
      REQUIRE(c2.dialogueId() == d->id());

      dialogue = c2.getDialogue(pkAgent1, d->id());
      REQUIRE(dialogue == nullptr);
      c1.unregisterDialogue(*d);
      dialogue.reset();
    }

    // registering an existing dialogue gives a negative status code
    {
      bool status;
      dialogue = std::make_shared<SimpleSingleDialogue>(c3, pkAgent3, 0);
      status = c3.registerDialogue(dialogue);
      REQUIRE(status);
      status = c3.registerDialogue(dialogue);
      REQUIRE(!status);
      status = c3.unregisterDialogue(*dialogue);
    }

    // unregistering an unexisting dialogue gives a negative status code
    {
      bool status;
      dialogue = std::make_shared<SimpleSingleDialogue>(c3, pkAgent3, 1);
      status = c3.unregisterDialogue(*dialogue);
      REQUIRE(!status);
    }

    // receiving 'OnDialogueError' for an unexisting dialogue should log an error.
    {
      uint32_t dialogueId = 0;
      std::string fooPublicKey = "non_existing_agent";
      auto d = std::shared_ptr<SimpleSingleDialogue>(new SimpleSingleDialogue{c3, fooPublicKey, dialogueId});
      c3.registerDialogue(d);
      d->sendMessage(0, "Hello world");
      std::this_thread::sleep_for(std::chrono::milliseconds{250});
      REQUIRE(c3.action() == AgentAction::ON_DIALOGUE_ERROR);
      REQUIRE(c3.from() == fooPublicKey);
      REQUIRE(c3.dialogueId() == dialogueId);
      c3.unregisterDialogue(*d);
      dialogue.reset();
    }

    // check that an 'OnDialogueError' message is dispatched to the right dialogue.
    {
      uint32_t dialogueId = 1;
      std::string fooPublicKey = "non_existing_agent";
      auto d = std::shared_ptr<SimpleSingleDialogue>(new SimpleSingleDialogue{c3, fooPublicKey, dialogueId});
      c3.registerDialogue(d);
      d->sendMessage(0, "Hello world");
      std::this_thread::sleep_for(std::chrono::milliseconds{250});
      REQUIRE(c3.action() == AgentAction::ON_DIALOGUE_ERROR);
      REQUIRE(c3.from() == fooPublicKey);
      REQUIRE(c3.dialogueId() == dialogueId);
      dialogue = c3.getDialogue(fooPublicKey, dialogueId);
      REQUIRE(dialogue != nullptr);
      REQUIRE(dialogue->id() == dialogueId);
      REQUIRE(dialogue->action() == AgentAction::ON_DIALOGUE_ERROR);
      c3.unregisterDialogue(*dialogue);
      dialogue.reset();
    }

    c1.stop();
    c2.stop();
    c3.stop();
    pool.stop();


    std::cerr << "Waiting\n";
    std::this_thread::sleep_for(std::chrono::seconds{2});
    std::cerr << "NbAgents " << as.nbAgents() << "\n";

    as.stop();
    std::cerr << "Server stopped\n";

  }


  TEST_CASE("local transfer between dialogue agents", "[dialogue]"){

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%n] [%l] %v");
    spdlog::set_level(spdlog::level::level_enum::trace);
    Uuid id = Uuid::uuid4();
    std::cerr << id.to_string() << std::endl;
    std::string s = id.to_string();
    Uuid id2{s};
    std::cerr << id2.to_string() << std::endl;
    fetch::oef::SchedulerPB scheduler;
    std::cerr << "Scheduler created\n";
    {
      IoContextPool pool(2);
      pool.run();

      std::string pkAgent1 = "DialogueAgent1";
      std::string pkAgent2 = "DialogueAgent2";
      std::string pkAgent3 = "DialogueAgent3";
      
      auto proxy1 = std::unique_ptr<fetch::oef::OEFCoreInterface>(new fetch::oef::OEFCoreLocalPB{pkAgent1, scheduler});
      auto proxy2 = std::unique_ptr<fetch::oef::OEFCoreInterface>(new fetch::oef::OEFCoreLocalPB{pkAgent2, scheduler});
      auto proxy3 = std::unique_ptr<fetch::oef::OEFCoreInterface>(new fetch::oef::OEFCoreLocalPB{pkAgent3, scheduler});
      
      SimpleDialogueAgent c1(std::move(proxy1));
      SimpleDialogueAgent c2(std::move(proxy2));
      SimpleDialogueAgent c3(std::move(proxy3));

      std::cerr << "Clients created\n";
      REQUIRE(scheduler.nbAgents() == 3);
      
      std::shared_ptr<SimpleSingleDialogue> dialogue;
      {
        //Receiving a simple message for the first time creates a new dialogue.
        auto d = std::shared_ptr<SimpleSingleDialogue>(new SimpleSingleDialogue{c1, pkAgent2});
        c1.registerDialogue(d);
        d->sendMessage(1, "Hello world");
        std::this_thread::sleep_for(std::chrono::milliseconds{250});
        REQUIRE(c2.action() == AgentAction::ON_MESSAGE);
        REQUIRE(c2.from() == pkAgent1);
        REQUIRE(c2.dialogueId() == d->id());
        
        dialogue = c2.getDialogue(pkAgent1, d->id());
        REQUIRE(dialogue != nullptr);
        REQUIRE(dialogue->id() == d->id());
        REQUIRE(dialogue->action() == AgentAction::ON_MESSAGE);
        dialogue.reset();
        c1.unregisterDialogue(*d);
      }
      //Receiving a CFP for the first time creates a new dialogue.
      //The other negotiation messages are correctly dispatched to the right dialogue's callback.
      {
        auto d = std::shared_ptr<SimpleSingleDialogue>(new SimpleSingleDialogue{c1, pkAgent2});
        c1.registerDialogue(d);
        d->sendCFP(1, 0, stde::nullopt);
        std::this_thread::sleep_for(std::chrono::milliseconds{250});
        REQUIRE(c2.action() == AgentAction::ON_CFP);
        REQUIRE(c2.from() == pkAgent1);
        REQUIRE(c2.dialogueId() == d->id());
        dialogue = c2.getDialogue(pkAgent1, d->id());
        REQUIRE(dialogue != nullptr);
        REQUIRE(dialogue->id() == d->id());
        REQUIRE(dialogue->action() == AgentAction::ON_CFP);

        dialogue->sendPropose(2, 1, "proposal");
        std::this_thread::sleep_for(std::chrono::milliseconds{250});
        REQUIRE(c1.action() == AgentAction::ON_PROPOSE);
        REQUIRE(c1.from() == pkAgent2);
        REQUIRE(c1.dialogueId() == d->id());
        dialogue = c1.getDialogue(pkAgent2, d->id());
        REQUIRE(dialogue != nullptr);
        REQUIRE(dialogue->id() == d->id());
        REQUIRE(dialogue->action() == AgentAction::ON_PROPOSE);

        d->sendAccept(3, 2);
        std::this_thread::sleep_for(std::chrono::milliseconds{250});
        REQUIRE(c2.action() == AgentAction::ON_ACCEPT);
        REQUIRE(c2.from() == pkAgent1);
        REQUIRE(c2.dialogueId() == d->id());
        dialogue = c2.getDialogue(pkAgent1, d->id());
        REQUIRE(dialogue != nullptr);
        REQUIRE(dialogue->id() == d->id());
        REQUIRE(dialogue->action() == AgentAction::ON_ACCEPT);

        dialogue->sendDecline(4, 3);
        std::this_thread::sleep_for(std::chrono::milliseconds{250});
        REQUIRE(c1.action() == AgentAction::ON_DECLINE);
        REQUIRE(c1.from() == pkAgent2);
        REQUIRE(c1.dialogueId() == d->id());
        dialogue = c1.getDialogue(pkAgent2, d->id());
        REQUIRE(dialogue != nullptr);
        REQUIRE(dialogue->id() == d->id());
        REQUIRE(dialogue->action() == AgentAction::ON_DECLINE);

        c1.unregisterDialogue(*d);
        dialogue.reset();
      }
      //Receiving a 'Propose' as a first message in a dialogue should log an error.
      {
        auto d = std::shared_ptr<SimpleSingleDialogue>(new SimpleSingleDialogue{c1, pkAgent2});
        c1.registerDialogue(d);
        d->sendPropose(1, 0, "proposal");
        std::this_thread::sleep_for(std::chrono::milliseconds{250});
        REQUIRE(c2.action() == AgentAction::ON_PROPOSE);
        REQUIRE(c2.from() == pkAgent1);
        REQUIRE(c2.dialogueId() == d->id());
        
        dialogue = c2.getDialogue(pkAgent1, d->id());
        REQUIRE(dialogue == nullptr);
        c1.unregisterDialogue(*d);
        dialogue.reset();
      }

      //Receiving an 'Accept' as a first message in a dialogue should log an error.
      {
        auto d = std::shared_ptr<SimpleSingleDialogue>(new SimpleSingleDialogue{c1, pkAgent2});
        c1.registerDialogue(d);
        d->sendAccept(1, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds{250});
        REQUIRE(c2.action() == AgentAction::ON_ACCEPT);
        REQUIRE(c2.from() == pkAgent1);
        REQUIRE(c2.dialogueId() == d->id());
        
        dialogue = c2.getDialogue(pkAgent1, d->id());
        REQUIRE(dialogue == nullptr);
        c1.unregisterDialogue(*d);
        dialogue.reset();
      }

      //Receiving a 'Decline' as a first message in a dialogue should log an error.
      {
        auto d = std::shared_ptr<SimpleSingleDialogue>(new SimpleSingleDialogue{c1, pkAgent2});
        c1.registerDialogue(d);
        d->sendDecline(1, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds{250});
        REQUIRE(c2.action() == AgentAction::ON_DECLINE);
        REQUIRE(c2.from() == pkAgent1);
        REQUIRE(c2.dialogueId() == d->id());
        
        dialogue = c2.getDialogue(pkAgent1, d->id());
        REQUIRE(dialogue == nullptr);
        c1.unregisterDialogue(*d);
        dialogue.reset();
      }

      // registering an existing dialogue gives a negative status code
      {
        bool status;
        dialogue = std::make_shared<SimpleSingleDialogue>(c3, pkAgent3, 0);
        status = c3.registerDialogue(dialogue);
        REQUIRE(status);
        status = c3.registerDialogue(dialogue);
        REQUIRE(!status);
        status = c3.unregisterDialogue(*dialogue);
      }
      
      // unregistering an unexisting dialogue gives a negative status code
      {
        bool status;
        dialogue = std::make_shared<SimpleSingleDialogue>(c3, pkAgent3, 1);
        status = c3.unregisterDialogue(*dialogue);
        REQUIRE(!status);
      }
      
      c1.stop();
      c2.stop();
      c3.stop();

      pool.stop();
    }

    std::cerr << "Waiting\n";
    std::this_thread::sleep_for(std::chrono::seconds{1});
    std::cerr << "NbAgents " << scheduler.nbAgents() << "\n";

    scheduler.stop();
    std::cerr << "Server stopped\n";

  }

}
