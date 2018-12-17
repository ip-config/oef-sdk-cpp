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

#define DEBUG_ON 1
#include "server.hpp"
#include <iostream>
#include <google/protobuf/text_format.h>
#include <sstream>
#include <iomanip>

namespace fetch {
  namespace oef {
    fetch::oef::Logger Server::logger = fetch::oef::Logger("oef-node");
    fetch::oef::Logger AgentDirectory::logger = fetch::oef::Logger("agent-directory");
    
    std::string to_string(const google::protobuf::Message &msg) {
      std::string output;
      google::protobuf::TextFormat::PrintToString(msg, &output);
      return output;
    }
    class AgentSession : public std::enable_shared_from_this<AgentSession>
    {
    private:
      const std::string publicKey_;
      stde::optional<Instance> description_;
      AgentDirectory &agentDirectory_;
      ServiceDirectory &serviceDirectory_;
      tcp::socket socket_;

      static fetch::oef::Logger logger;
      
    public:
      explicit AgentSession(std::string publicKey, AgentDirectory &agentDirectory, ServiceDirectory &serviceDirectory, tcp::socket socket)
        : publicKey_{std::move(publicKey)}, agentDirectory_{agentDirectory}, serviceDirectory_{serviceDirectory}, socket_(std::move(socket)) {}
      virtual ~AgentSession() {
        logger.trace("~AgentSession");
        //socket_.shutdown(asio::socket_base::shutdown_both);
      }
      AgentSession(const AgentSession &) = delete;
      AgentSession operator=(const AgentSession &) = delete;
      void start() {
        read();
      }
      void write(std::shared_ptr<Buffer> buffer) {
        asyncWriteBuffer(socket_, std::move(buffer), 5);
      }
      void send(const fetch::oef::pb::Server_AgentMessage &msg) {
        asyncWriteBuffer(socket_, serialize(msg), 10 /* sec ? */);
      }
      std::string id() const { return publicKey_; }
      bool match(const QueryModel &query) const {
        if(!description_) {
          return false;
        }
        return query.check(*description_);
      }
    private:
      void processRegisterDescription(const fetch::oef::pb::AgentDescription &desc) {
        description_ = Instance(desc.description());
        DEBUG(logger, "AgentSession::processRegisterDescription setting description to agent {} : {}", publicKey_, to_string(desc));
        if(!description_) {
          fetch::oef::pb::Server_AgentMessage answer;
          auto *error = answer.mutable_error();
          error->set_operation(fetch::oef::pb::Server_AgentMessage_Error::REGISTER_DESCRIPTION);
          logger.trace("AgentSession::processRegisterDescription sending error {} to {}", error->operation(), publicKey_);
          send(answer);
        }
      }
      void processUnregisterDescription() {
        description_ = stde::nullopt;
        DEBUG(logger, "AgentSession::processUnregisterDescription setting description to agent {}", publicKey_);
      }
      void processRegisterService(const fetch::oef::pb::AgentDescription &desc) {
        DEBUG(logger, "AgentSession::processRegisterService registering agent {} : {}", publicKey_, to_string(desc));
        bool success = serviceDirectory_.registerAgent(Instance(desc.description()), publicKey_);
        if(!success) {
          fetch::oef::pb::Server_AgentMessage answer;
          auto *error = answer.mutable_error();
          error->set_operation(fetch::oef::pb::Server_AgentMessage_Error::REGISTER_SERVICE);
          logger.trace("AgentSession::processRegisterService sending error {} to {}", error->operation(), publicKey_);
          send(answer);
        }
      }
      void processUnregisterService(const fetch::oef::pb::AgentDescription &desc) {
        DEBUG(logger, "AgentSession::processUnregisterService unregistering agent {} : {}", publicKey_, to_string(desc));
        bool success = serviceDirectory_.unregisterAgent(Instance(desc.description()), publicKey_);
        if(!success) {
          fetch::oef::pb::Server_AgentMessage answer;
          auto *error = answer.mutable_error();
          error->set_operation(fetch::oef::pb::Server_AgentMessage_Error::UNREGISTER_SERVICE);
          logger.trace("AgentSession::processUnregisterService sending error {} to {}", error->operation(), publicKey_);
          send(answer);
        }
      }
      void processSearchAgents(const fetch::oef::pb::AgentSearch &search) {
        QueryModel model{search.query()};
        DEBUG(logger, "AgentSession::processSearchAgents from agent {} : {}", publicKey_, to_string(search));
        auto agents_vec = agentDirectory_.search(model);
        fetch::oef::pb::Server_AgentMessage answer;
        auto agents = answer.mutable_agents();
        agents->set_search_id(search.search_id());
        for(auto &a : agents_vec) {
          agents->add_agents(a);
        }
        logger.trace("AgentSession::processSearchAgents sending {} agents to {}", agents_vec.size(), publicKey_);
        send(answer);
      }
      void processQuery(const fetch::oef::pb::AgentSearch &search) {
        QueryModel model{search.query()};
        DEBUG(logger, "AgentSession::processQuery from agent {} : {}", publicKey_, to_string(search));
        auto agents_vec = serviceDirectory_.query(model);
        fetch::oef::pb::Server_AgentMessage answer;
        auto agents = answer.mutable_agents();
        agents->set_search_id(search.search_id());
        for(auto &a : agents_vec) {
          agents->add_agents(a);
        }
        logger.trace("AgentSession::processQuery sending {} agents to {}", agents_vec.size(), publicKey_);
        send(answer);
      }
      void processMessage(fetch::oef::pb::Agent_Message *msg) {
        auto session = agentDirectory_.session(msg->destination());
        DEBUG(logger, "AgentSession::processMessage from agent {} : {}", publicKey_, to_string(*msg));
        logger.trace("AgentSession::processMessage to {} from {}", msg->destination(), publicKey_);
        if(session) {
          fetch::oef::pb::Server_AgentMessage message;
          auto content = message.mutable_content();
          uint32_t did = msg->dialogue_id();
          content->set_dialogue_id(did);
          content->set_origin(publicKey_);
          if(msg->has_content()) {
            content->set_allocated_content(msg->release_content());
          }
          if(msg->has_fipa()) {
            content->set_allocated_fipa(msg->release_fipa());
          }
          DEBUG(logger, "AgentSession::processMessage to agent {} : {}", msg->destination(), to_string(message));
          auto buffer = serialize(message);
          asyncWriteBuffer(session->socket_, buffer, 5, [this,did](std::error_code ec, std::size_t length) {
              if(ec) {
                fetch::oef::pb::Server_AgentMessage answer;
                auto *error = answer.mutable_error();
                error->set_operation(fetch::oef::pb::Server_AgentMessage_Error::SEND_MESSAGE);
                error->set_dialogue_id(did);
                logger.trace("AgentSession::processMessage sending error {} to {}", error->operation(), publicKey_);
                send(answer);
              }
            });
        }
      }
      void process(const std::shared_ptr<Buffer> &buffer) {
        auto envelope = deserialize<fetch::oef::pb::Envelope>(*buffer);
        auto payload_case = envelope.payload_case();
        switch(payload_case) {
        case fetch::oef::pb::Envelope::kSendMessage:
          processMessage(envelope.release_send_message());
          break;
        case fetch::oef::pb::Envelope::kRegisterService:
          processRegisterService(envelope.register_service());
          break;
        case fetch::oef::pb::Envelope::kUnregisterService:
          processUnregisterService(envelope.unregister_service());
          break;
        case fetch::oef::pb::Envelope::kRegisterDescription:
          processRegisterDescription(envelope.register_description());
          break;
        case fetch::oef::pb::Envelope::kUnregisterDescription:
          processUnregisterDescription();
          break;
        case fetch::oef::pb::Envelope::kSearchAgents:
          processSearchAgents(envelope.search_agents());
          break;
        case fetch::oef::pb::Envelope::kSearchServices:
          processQuery(envelope.search_services());
          break;
        case fetch::oef::pb::Envelope::PAYLOAD_NOT_SET:
          logger.error("AgentSession::process cannot process payload {} from {}", payload_case, publicKey_);
        }
      }
      void read() {
        auto self(shared_from_this());
        asyncReadBuffer(socket_, 5, [this, self](std::error_code ec, std::shared_ptr<Buffer> buffer) {
                                if(ec) {
                                  agentDirectory_.remove(publicKey_);
                                  serviceDirectory_.unregisterAll(publicKey_);
                                  logger.info("AgentSession::read error on id {} ec {}", publicKey_, ec);
                                } else {
                                  process(buffer);
                                  read();
                                }});
      }
      
    };
    fetch::oef::Logger AgentSession::logger = fetch::oef::Logger("oef-node::agent-session");

    const std::vector<std::string> AgentDirectory::search(const QueryModel &query) const {
      std::lock_guard<std::mutex> lock(lock_);
      std::vector<std::string> res;
      for(const auto &s : sessions_) {
        if(s.second->match(query)) {
          res.emplace_back(s.first);
        }
      }
      return res;
    }

    void Server::secretHandshake(const std::string &publicKey, const std::shared_ptr<Context> &context) {
      fetch::oef::pb::Server_Phrase phrase;
      phrase.set_phrase("RandomlyGeneratedString");
      auto phrase_buffer = serialize(phrase);
      logger.trace("Server::secretHandshake sending phrase size {}", phrase_buffer->size());
      asyncWriteBuffer(context->socket_, phrase_buffer, 10 /* sec ? */);
      logger.trace("Server::secretHandshake waiting answer");
      asyncReadBuffer(context->socket_, 5,
                      [this,publicKey,context](std::error_code ec, std::shared_ptr<Buffer> buffer) {
                        if(ec) {
                          logger.error("Server::secretHandshake read failure {}", ec.value());
                        } else {
                          try {
                            auto ans = deserialize<fetch::oef::pb::Agent_Server_Answer>(*buffer);
                            logger.trace("Server::secretHandshake secret [{}]", ans.answer());
                            auto session = std::make_shared<AgentSession>(publicKey, agentDirectory_, serviceDirectory_, std::move(context->socket_));
                            if(agentDirectory_.add(publicKey, session)) {
                              session->start();
                              fetch::oef::pb::Server_Connected status;
                              status.set_status(true);
                              session->write(serialize(status));
                            } else {
                              fetch::oef::pb::Server_Connected status;
                              status.set_status(false);
                              logger.info("Server::secretHandshake PublicKey already connected (interleaved) publicKey {}", publicKey);
                              asyncWriteBuffer(context->socket_, serialize(status), 10 /* sec ? */);
                            }
                            // should check the secret with the public key i.e. ID.
                          } catch(std::exception &) {
                            logger.error("Server::secretHandshake error on Answer publicKey {}", publicKey);
                            fetch::oef::pb::Server_Connected status;
                            status.set_status(false);
                            asyncWriteBuffer(context->socket_, serialize(status), 10 /* sec ? */);
                          }
                          // everything is fine -> send connection OK.
                        }
                      });
    }
    void Server::newSession(tcp::socket socket) {
      auto context = std::make_shared<Context>(std::move(socket));
      asyncReadBuffer(context->socket_, 5,
                      [this,context](std::error_code ec, std::shared_ptr<Buffer> buffer) {
                        if(ec) {
                          logger.error("Server::newSession read failure {}", ec.value());
                        } else {
                          try {
                            logger.trace("Server::newSession received {} bytes", buffer->size());
                            auto id = deserialize<fetch::oef::pb::Agent_Server_ID>(*buffer);
                            logger.trace("Debug {}", to_string(id));
                            logger.trace("Server::newSession connection from {}", id.public_key());
                            if(!agentDirectory_.exist(id.public_key())) { // not yet connected
                              secretHandshake(id.public_key(), context);
                            } else {
                              logger.info("Server::newSession ID {} already connected", id.public_key());
                              fetch::oef::pb::Server_Phrase failure;
                              (void)failure.mutable_failure();
                              asyncWriteBuffer(context->socket_, serialize(failure), 10 /* sec ? */);
                            }
                          } catch(std::exception &) {
                            logger.error("Server::newSession error parsing ID");
                            fetch::oef::pb::Server_Phrase failure;
                            (void)failure.mutable_failure();
                            asyncWriteBuffer(context->socket_, serialize(failure), 10 /* sec ? */);
                          }
                        }
                      });
    }
    void Server::do_accept() {
      logger.trace("Server::do_accept");
      acceptor_.async_accept([this](std::error_code ec, tcp::socket socket) {
                               if (!ec) {
                                 logger.trace("Server::do_accept starting new session");
                                 newSession(std::move(socket));
                                 do_accept();
                               } else {
                                 logger.error("Server::do_accept error {}", ec.value());
                               }
                             });
    }
    Server::~Server() {
      logger.trace("~Server stopping");
      stop();
      logger.trace("~Server stopped");
      agentDirectory_.clear();
      //    acceptor_.close();
      logger.trace("~Server waiting for threads");
      for(auto &t : threads_) {
        if(t) {
          t->join();
        }
      }
      logger.trace("~Server threads stopped");
    }
    void Server::run() {
      for(auto &t : threads_) {
        if(!t) {
          t = std::make_unique<std::thread>([this]() {do_accept(); io_context_.run();});
        }
      }
    }
    void Server::run_in_thread() {
      do_accept();
      io_context_.run();
    }
    void Server::stop() {
      std::this_thread::sleep_for(std::chrono::seconds{1});
      io_context_.stop();
    }
  }
}
