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

#include "common.hpp"
#include "uuid.hpp"
#include "agent.pb.h"
#include "logger.hpp"
#include "oefcoreproxy.hpp"
#include "clientmsg.hpp"
#include "queue.hpp"
#include "servicedirectory.hpp"

#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <google/protobuf/text_format.h>

namespace fetch {
  namespace oef {
    template <typename T>
    class Dialogues;

    template <typename T>
    class Dialogue {
    private:
      Uuid32 _uuid;
      std::string _dest;
      uint32_t _msgId = 0;
      T _state;
      Dialogues<T> &_dialogues;
      friend class Dialogues<T>;
      Dialogue(uint32_t uuid, std::string dest, Dialogues<T> &dialogues) :
      _uuid{uuid}, _dest{std::move(dest)}, _dialogues{dialogues} {}
      explicit Dialogue(std::string dest, Dialogues<T> &dialogues) :
      _uuid{Uuid32::uuid()}, _dest{std::move(dest)}, _dialogues{dialogues} {}
    public:
      std::string dest() const { return _dest; }
      uint32_t uuid() const { return _uuid.val(); }
      uint32_t msgId() const { return _msgId; }
      void incrementMsgId() { ++_msgId; }
      void setFinished() {
        _dialogues.erase(uuid());
      }
      const T getState() const { return _state; }
      void setState(const T& t) { _state = t; }
      std::shared_ptr<Buffer> envelope(const std::string &outgoing) {
        fetch::oef::pb::Envelope env;
        auto *message = env.mutable_send_message();
        message->set_dialogue_id(_uuid.val());
        message->set_destination(_dest);
        message->set_content(outgoing);
        return serialize(env);
      }
      std::shared_ptr<Buffer> envelope(const google::protobuf::MessageLite &t) {
        return envelope(t.SerializeAsString());
      }
    };

    template <typename T>
    class Dialogues {
    private:
      std::unordered_map<uint32_t, std::shared_ptr<Dialogue<T>>> _dialogues;

      static fetch::oef::Logger logger;
    public:
      Dialogue<T> &create(const std::string &dest) {
        auto dialogue = std::shared_ptr<Dialogue<T>>(new Dialogue<T>{dest, *this});
        _dialogues[dialogue->uuid()] = dialogue;
        logger.trace("create dest {} id {} size {}", dialogue->dest(), dialogue->uuid(), _dialogues.size());
        return *dialogue;
      }
      Dialogue<T> &get(uint32_t id, const std::string &dest) {
        auto iter = _dialogues.find(id);
        if(iter != _dialogues.end()) {
          logger.trace("get2 exists dest {} id {} size {}", iter->second->dest(), iter->second->uuid(), _dialogues.size());
          return *(iter->second);
        }
        auto dialogue = std::shared_ptr<Dialogue<T>>(new Dialogue<T>{id, dest, *this});
        _dialogues[id] = dialogue;
        logger.trace("get2 new dest {} id {} size {}", dialogue->dest(), dialogue->uuid(), _dialogues.size());
        return *dialogue;
      }
      Dialogue<T> &get(uint32_t id) {
        std::shared_ptr<Dialogue<T>> dialogue = _dialogues[id];
        logger.trace("get1 id {} size {} exists {}", id, _dialogues.size(), bool(dialogue));
        assert(dialogue);
        return *dialogue;
      }
      void erase(uint32_t id) {
        _dialogues.erase(id);
      }
    };

    template <typename T>
    fetch::oef::Logger Dialogues<T>::logger = fetch::oef::Logger("oefcore-dialogues");

    class MessageDecoder {
    private:
      static fetch::oef::Logger logger;

      static void dispatch(AgentInterface &agent, const fetch::oef::pb::Fipa_Message &fipa,
                    const fetch::oef::pb::Server_AgentMessage_Content &content) {
        logger.trace("dispatch msg {}", fipa.msg_case());
        switch(fipa.msg_case()) {
        case fetch::oef::pb::Fipa_Message::kCfp:
          {
            auto &cfp = fipa.cfp();
            logger.trace("dispatch cfp {}", cfp.payload_case());
            CFPType constraints;
            switch(cfp.payload_case()) {
            case fetch::oef::pb::Fipa_Cfp::kQuery:
              constraints = CFPType{QueryModel{cfp.query()}};
              break;
            case fetch::oef::pb::Fipa_Cfp::kContent:
              constraints = CFPType{cfp.content()};
              break;
            case fetch::oef::pb::Fipa_Cfp::kNothing:
              constraints = CFPType{stde::nullopt};
              break;
            case fetch::oef::pb::Fipa_Cfp::PAYLOAD_NOT_SET:
              break;
            }
            logger.trace("dispatch cfp from {} cid {} msgId {} target {}", content.origin(), content.dialogue_id(),
                         fipa.msg_id(), fipa.target());
            agent.onCFP(content.origin(), content.dialogue_id(), fipa.msg_id(), fipa.target(),
                        constraints);
          }
          break;
        case fetch::oef::pb::Fipa_Message::kPropose:
          {
            auto &propose = fipa.propose();
            ProposeType proposals;
            logger.trace("dispatch propose {}", propose.payload_case());
            switch(propose.payload_case()) {
            case fetch::oef::pb::Fipa_Propose::kProposals:
              {
                std::vector<Instance> instances;
                for(auto &instance : propose.proposals().objects()) {
                  instances.emplace_back(instance);
                }
                proposals = ProposeType{std::move(instances)};
              }
              break;
            case fetch::oef::pb::Fipa_Propose::kContent:
              proposals = ProposeType{propose.content()};
              break;
            case fetch::oef::pb::Fipa_Propose::PAYLOAD_NOT_SET:
              break;
            }
            logger.trace("dispatch propose from {} cid {} msgId {} target {}", content.origin(), content.dialogue_id(),
                         fipa.msg_id(), fipa.target());
            agent.onPropose(content.origin(), content.dialogue_id(), fipa.msg_id(), fipa.target(),
                            proposals);
          }
          break;
        case fetch::oef::pb::Fipa_Message::kAccept:
          logger.trace("dispatch accept from {} cid {} msgId {} target {}", content.origin(), content.dialogue_id(),
                       fipa.msg_id(), fipa.target());
          agent.onAccept(content.origin(), content.dialogue_id(), fipa.msg_id(), fipa.target());
          break;
        case fetch::oef::pb::Fipa_Message::kDecline:
          logger.trace("dispatch decline from {} cid {} msgId {} target {}", content.origin(), content.dialogue_id(),
                       fipa.msg_id(), fipa.target());
          agent.onDecline(content.origin(), content.dialogue_id(), fipa.msg_id(), fipa.target());
          break;
        case fetch::oef::pb::Fipa_Message::MSG_NOT_SET:
          logger.error("MessageDecoder::loop error on fipa {}", static_cast<int>(fipa.msg_case()));
        }
      }
    public:
      static void decode(const std::string &agentPublicKey, const std::shared_ptr<Buffer> &buffer, AgentInterface &agent) {
        try {
          auto msg = deserialize<fetch::oef::pb::Server_AgentMessage>(*buffer);
          switch(msg.payload_case()) {
          case fetch::oef::pb::Server_AgentMessage::kError:
            {
              logger.trace("MessageDecoder::loop error");
              auto &error = msg.error();
              agent.onError(error.operation(), error.has_dialogue_id() ? stde::optional<uint32_t>(error.dialogue_id()) : stde::nullopt,
                            error.has_msg_id() ? stde::optional<uint32_t>(error.msg_id()) : stde::nullopt);
            }
            break;
          case fetch::oef::pb::Server_AgentMessage::kAgents:
            {
              logger.trace("MessageDecoder::loop searchResults");
              std::vector<std::string> searchResults;
              for(auto &s : msg.agents().agents()) {
                searchResults.emplace_back(s);
              }
              agent.onSearchResult(msg.agents().search_id(), searchResults);
            }
            break;
          case fetch::oef::pb::Server_AgentMessage::kContent:
            {
              logger.trace("MessageDecoder::loop content");
              auto &content = msg.content();
              switch(content.payload_case()) {
              case fetch::oef::pb::Server_AgentMessage_Content::kContent:
                {
                  logger.trace("MessageDecoder::loop onMessage {} from {} cid {}", agentPublicKey, content.origin(), content.dialogue_id());
                  agent.onMessage(content.origin(), content.dialogue_id(), content.content());
                }
                break;
              case fetch::oef::pb::Server_AgentMessage_Content::kFipa:
                {
                  logger.trace("MessageDecoder::loop fipa");
                  dispatch(agent, content.fipa(), content);
                }
                break;
              case fetch::oef::pb::Server_AgentMessage_Content::PAYLOAD_NOT_SET:
                logger.error("MessageDecoder::loop error on message {}", static_cast<int>(msg.payload_case()));
              }
            }
            break;
          case fetch::oef::pb::Server_AgentMessage::PAYLOAD_NOT_SET:
            logger.error("MessageDecoder::loop error {}", static_cast<int>(msg.payload_case()));
          }
        } catch(std::exception &e) {
          logger.error("MessageDecoder::loop cannot deserialize AgentMessage {}", e.what());
        }
      }
    };
    
    class SchedulerPB {
    private:
      struct LocalAgentSession {
        AgentInterface *_agent;
        stde::optional<Instance> _description;
        
        bool match(const QueryModel &query) const {
          if(!_description)
            return false;
          return query.check(*_description);
        }
      };
      std::unordered_map<std::string, LocalAgentSession> _agents;
      Queue<std::pair<std::string,std::shared_ptr<Buffer>>> _queue;
      std::unique_ptr<std::thread> _thread;
      bool _stopping = false;
      ServiceDirectory _sd;
      
      static fetch::oef::Logger logger;
      
      void process() {
        while(!_stopping) {
          auto p = _queue.pop();
          if(!_stopping) {
            if(_agents.find(p.first) != _agents.end() && _agents[p.first]._agent)
              MessageDecoder::decode(p.first, p.second, *_agents[p.first]._agent);
          }
        }
      }
    public:
      SchedulerPB() {
        _thread = std::unique_ptr<std::thread>(new std::thread([this]() { process(); }));
      }
      ~SchedulerPB() {
        if(!_stopping)
          _thread->join();
      }
      void stop() {
        _stopping = true;
        _queue.push(std::pair<std::string,std::shared_ptr<Buffer>>(std::string{""}, std::make_shared<Buffer>(Buffer{})));
        if(_thread)
          _thread->join();
      }
      size_t nbAgents() const { return _agents.size(); }
      bool connect(const std::string &agentPublicKey) {
        logger.trace("SchedulerPB::connect {} size {}", agentPublicKey, _agents.size());
        if(_agents.find(agentPublicKey) != _agents.end())
          return false;
        _agents[agentPublicKey] = LocalAgentSession{};
        return true;
      }
      void disconnect(const std::string &agentPublicKey) {
        logger.trace("SchedulerPB::disconnect {}", agentPublicKey);
        _agents.erase(agentPublicKey);
      }
      void loop(const std::string &agentPublicKey, AgentInterface &agent) {
        logger.trace("SchedulerPB::loop {}", agentPublicKey);
        _agents[agentPublicKey]._agent = &agent;
      }
      void registerDescription(const std::string &agentPublicKey, const Instance &instance) {
        logger.trace("SchedulerPB::registerDescription {}", agentPublicKey);
        if(_agents.find(agentPublicKey) == _agents.end())
          logger.error("SchedulerPB::registerDescription {} is not registered", agentPublicKey);
        else
          _agents[agentPublicKey]._description = instance;
      }
      void unregisterDescription(const std::string &agentPublicKey) {
        logger.trace("SchedulerPB::unregisterDescription {}", agentPublicKey);
        if(_agents.find(agentPublicKey) == _agents.end())
          logger.error("SchedulerPB::unregisterDescription {} is not registered", agentPublicKey);
        else
          _agents[agentPublicKey]._description = stde::nullopt;
      }
      void registerService(const std::string &agentPublicKey, const Instance &instance) {
        logger.trace("SchedulerPB::registerService {}", agentPublicKey);
        _sd.registerAgent(instance, agentPublicKey);
      }
      void unregisterService(const std::string &agentPublicKey, const Instance &instance) {
        logger.trace("SchedulerPB::unregisterService {}", agentPublicKey);
        _sd.unregisterAgent(instance, agentPublicKey);
      }
      std::vector<std::string> searchAgents(uint32_t, const QueryModel &model) const {
        logger.trace("SchedulerPB::searchServices");
        std::vector<std::string> res;
        for(const auto &s : _agents) {
          if(s.second.match(model))
            res.emplace_back(s.first);
        }
        return res;
      }
      std::vector<std::string> searchServices(uint32_t, const QueryModel &model) const {
        logger.trace("SchedulerPB::searchServices");
        auto res = _sd.query(model);
        logger.trace("SchedulerPB::searchServices size {}", res.size());
        return res;
      }
      void send(const std::string &agentPublicKey, const std::shared_ptr<Buffer> &buffer) {
        logger.trace("SchedulerPB::send {}", agentPublicKey);
        _queue.push(std::pair<std::string,std::shared_ptr<Buffer>>(agentPublicKey, buffer));
      }
      void sendTo(const std::string &agentPublicKey, const std::string &to, const std::shared_ptr<Buffer> &buffer) {
        logger.trace("SchedulerPB::sendTo {} to {}", agentPublicKey, to);
        if(_agents.find(to) == _agents.end())
          logger.error("SchedulerPB::sendTo {} is not connected.", to);
        else
          _queue.push(std::pair<std::string,std::shared_ptr<Buffer>>(to, buffer));
      }
    };

    class OEFCoreLocalPB : public virtual OEFCoreInterface {
    private:
      SchedulerPB &_scheduler;
      
      static fetch::oef::Logger logger;
    public:
      OEFCoreLocalPB(const std::string &agentPublicKey, SchedulerPB &scheduler) : OEFCoreInterface{agentPublicKey},
                                                                                  _scheduler{scheduler} {}
      void stop() override {
        _scheduler.disconnect(agentPublicKey_);
      }
      ~OEFCoreLocalPB() override {
        _scheduler.disconnect(agentPublicKey_);
      }
      bool handshake() override {
        return _scheduler.connect(agentPublicKey_);
      }
      void loop(AgentInterface &agent) override {
        _scheduler.loop(agentPublicKey_, agent);
      }
      void registerDescription(const Instance &instance) override {
        _scheduler.registerDescription(agentPublicKey_, instance);
      }
      void unregisterDescription() override {
        _scheduler.unregisterDescription(agentPublicKey_);
      }
      void registerService(const Instance &instance) override {
        _scheduler.registerService(agentPublicKey_, instance);
      }
      void searchAgents(uint32_t search_id, const QueryModel &model) override {
        auto agents_vec = _scheduler.searchAgents(search_id, model);
        fetch::oef::pb::Server_AgentMessage answer;
        auto agents = answer.mutable_agents();
        agents->set_search_id(search_id);
        for(auto &a : agents_vec) {
          agents->add_agents(a);
        }
        _scheduler.send(agentPublicKey_, serialize(answer));
      }
      void searchServices(uint32_t search_id, const QueryModel &model) override {
        auto agents_vec = _scheduler.searchServices(search_id, model);
        fetch::oef::pb::Server_AgentMessage answer;
        auto agents = answer.mutable_agents();
        agents->set_search_id(search_id);
        for(auto &a : agents_vec) {
          agents->add_agents(a);
        }
        _scheduler.send(agentPublicKey_, serialize(answer));
      }
      void unregisterService(const Instance &instance) override {
        _scheduler.unregisterService(agentPublicKey_, instance);
      }
      void sendMessage(uint32_t dialogueId, const std::string &dest, const std::string &msg) override {
        fetch::oef::pb::Server_AgentMessage message;
        auto content = message.mutable_content();
        content->set_dialogue_id(dialogueId);
        content->set_origin(agentPublicKey_);
        content->set_content(msg);
        _scheduler.sendTo(agentPublicKey_, dest, serialize(message));
      }
      void sendCFP(uint32_t dialogueId, const std::string &dest, const CFPType &constraints, uint32_t msgId = 1, uint32_t target = 0) override {
        CFP cfp{dialogueId, dest, constraints, msgId, target};
        fetch::oef::pb::Server_AgentMessage message;
        auto content = message.mutable_content();
        content->set_dialogue_id(dialogueId);
        content->set_origin(agentPublicKey_);
        content->set_allocated_fipa(cfp.handle().release_send_message()->release_fipa());
        _scheduler.sendTo(agentPublicKey_, dest, serialize(message));
      };
      void sendPropose(uint32_t dialogueId, const std::string &dest, const ProposeType &proposals, uint32_t msgId, uint32_t target) override {
        Propose propose{dialogueId, dest, proposals, msgId, target};
        fetch::oef::pb::Server_AgentMessage message;
        auto content = message.mutable_content();
        content->set_dialogue_id(dialogueId);
        content->set_origin(agentPublicKey_);
        content->set_allocated_fipa(propose.handle().release_send_message()->release_fipa());
        _scheduler.sendTo(agentPublicKey_, dest, serialize(message));
      }
      void sendAccept(uint32_t dialogueId, const std::string &dest, uint32_t msgId, uint32_t target) override {
        Accept accept{dialogueId, dest, msgId, target};
        fetch::oef::pb::Server_AgentMessage message;
        auto content = message.mutable_content();
        content->set_dialogue_id(dialogueId);
        content->set_origin(agentPublicKey_);
        content->set_allocated_fipa(accept.handle().release_send_message()->release_fipa());
        _scheduler.sendTo(agentPublicKey_, dest, serialize(message));
      }
      void sendDecline(uint32_t dialogueId, const std::string &dest, uint32_t msgId, uint32_t target) override {
        Decline decline{dialogueId, dest, msgId, target};
        fetch::oef::pb::Server_AgentMessage message;
        auto content = message.mutable_content();
        content->set_dialogue_id(dialogueId);
        content->set_origin(agentPublicKey_);
        content->set_allocated_fipa(decline.handle().release_send_message()->release_fipa());
        _scheduler.sendTo(agentPublicKey_, dest, serialize(message));
      }
    };
    
    class OEFCoreNetworkProxy : virtual public OEFCoreInterface {
    private:
      asio::io_context &_io_context;
      tcp::socket _socket;
      
      static fetch::oef::Logger logger;

    public:
      OEFCoreNetworkProxy(const std::string &agentPublicKey, asio::io_context &io_context, const std::string &host)
        : OEFCoreInterface{agentPublicKey}, _io_context{io_context}, _socket{_io_context} {
          tcp::resolver resolver(_io_context);
          asio::connect(_socket, resolver.resolve(host, std::to_string(static_cast<int>(Ports::Agents))));
      }
      OEFCoreNetworkProxy(OEFCoreNetworkProxy &&) = default;
      void stop() override {
        _socket.shutdown(asio::socket_base::shutdown_both);
        _socket.close();
      }
      ~OEFCoreNetworkProxy() override {
        if(_socket.is_open())
          stop();
      }
      bool handshake() override {
        bool connected = false;
        bool finished = false;
        std::mutex lock;
        std::condition_variable condVar;
        fetch::oef::pb::Agent_Server_ID id;
        id.set_public_key(agentPublicKey_);
        logger.trace("OEFCoreNetworkProxy::handshake from [{}]", agentPublicKey_);
        asyncWriteBuffer(_socket, serialize(id), 5,
            [this,&connected,&condVar,&finished,&lock](std::error_code ec, std::size_t length){
              if(ec) {
                std::unique_lock<std::mutex> lck(lock);
                finished = true;
                condVar.notify_all();
              } else {
                logger.trace("OEFCoreNetworkProxy::handshake id sent");
                asyncReadBuffer(_socket, 5,
                    [this,&connected,&condVar,&finished,&lock](std::error_code ec,std::shared_ptr<Buffer> buffer) {
                      if(ec) {
                        std::unique_lock<std::mutex> lck(lock);
                        finished = true;
                        condVar.notify_all();
                      } else {
                        auto p = deserialize<fetch::oef::pb::Server_Phrase>(*buffer);
                        if(p.has_failure()) {
                          finished = true;
                          condVar.notify_all();
                        } else {
                          std::string answer_s = p.phrase();
                          logger.trace("OEFCoreNetworkProxy::handshake received phrase: [{}]", answer_s);
                          // normally should encrypt with private key
                          std::reverse(std::begin(answer_s), std::end(answer_s));
                          logger.trace("OEFCoreNetworkProxy::handshake sending back phrase: [{}]", answer_s);
                          fetch::oef::pb::Agent_Server_Answer answer;
                          answer.set_answer(answer_s);
                          asyncWriteBuffer(_socket, serialize(answer), 5,
                              [this,&connected,&condVar,&finished,&lock](std::error_code ec, std::size_t length){
                                if(ec) {
                                  std::unique_lock<std::mutex> lck(lock);
                                  finished = true;
                                  condVar.notify_all();
                                } else {
                                  asyncReadBuffer(_socket, 5,
                                      [&connected,&condVar,&finished,&lock](std::error_code ec,std::shared_ptr<Buffer> buffer) {
                                        auto c = deserialize<fetch::oef::pb::Server_Connected>(*buffer);
                                        logger.info("OEFCoreNetworkProxy::handshake received connected: {}", c.status());
                                        connected = c.status();
                                        std::unique_lock<std::mutex> lck(lock);
                                        finished = true;
                                        condVar.notify_all();
                                      });
                                }
                              });
                        }
                      }
                    });
              }
            });
        std::unique_lock<std::mutex> lck(lock);
        while(!finished) {
          condVar.wait(lck);
        }
        return connected;
      }
      void loop(AgentInterface &agent) override {
        asyncReadBuffer(_socket, 1000, [this,&agent](std::error_code ec, std::shared_ptr<Buffer> buffer) {
            if(ec) {
              logger.error("OEFCoreNetworkProxy::loop failure {}", ec.value());
            } else {
              logger.trace("OEFCoreNetworkProxy::loop");
              MessageDecoder::decode(agentPublicKey_, buffer, agent);
              loop(agent);
            }
          });
      }
      void registerDescription(const Instance &instance) override {
        Description description{instance};
        asyncWriteBuffer(_socket, serialize(description.handle()), 5);
      }
      void registerService(const Instance &instance) override {
        Register service{instance};
        asyncWriteBuffer(_socket, serialize(service.handle()), 5);
      }
      void searchAgents(uint32_t search_id, const QueryModel &model) override {
        SearchAgents searchAgents{search_id, model};
        asyncWriteBuffer(_socket, serialize(searchAgents.handle()), 5);
      }
      void searchServices(uint32_t search_id, const QueryModel &model) override {
        SearchServices searchServices{search_id, model};
        asyncWriteBuffer(_socket, serialize(searchServices.handle()), 5);
      }
      void unregisterService(const Instance &instance) override {
        Unregister service{instance};
        asyncWriteBuffer(_socket, serialize(service.handle()), 5);
      }
      void unregisterDescription() override {
        UnregisterDescription service{};
        asyncWriteBuffer(_socket, serialize(service.handle()), 5);
      }
      void sendMessage(uint32_t dialogueId, const std::string &dest, const std::string &msg) override {
        Message message{dialogueId, dest, msg};
        asyncWriteBuffer(_socket, serialize(message.handle()), 5);
      }
      void sendCFP(uint32_t dialogueId, const std::string &dest, const CFPType &constraints, uint32_t msgId = 1, uint32_t target = 0) override {
        CFP cfp{dialogueId, dest, constraints, msgId, target};
        asyncWriteBuffer(_socket, serialize(cfp.handle()), 5);
      }
      void sendPropose(uint32_t dialogueId, const std::string &dest, const ProposeType &proposals, uint32_t msgId, uint32_t target) override {
        Propose propose{dialogueId, dest, proposals, msgId, target};
        asyncWriteBuffer(_socket, serialize(propose.handle()), 5);
      }
      void sendAccept(uint32_t dialogueId, const std::string &dest, uint32_t msgId, uint32_t target) override {
        Accept accept{dialogueId, dest, msgId, target};
        asyncWriteBuffer(_socket, serialize(accept.handle()), 5);
      }
      void sendDecline(uint32_t dialogueId, const std::string &dest, uint32_t msgId, uint32_t target) override {
        Decline decline{dialogueId, dest, msgId, target};
        asyncWriteBuffer(_socket, serialize(decline.handle()), 5);
      }
    };

  }
}
