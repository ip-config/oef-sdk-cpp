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

#include <iostream>
#include "clara.hpp"
#include <future>
#include "agent.hpp"
#include "oefcoreproxy.hpp"

class SimpleAgent : public fetch::oef::Agent {
private:
  std::vector<std::string> results_;
 public:
  const std::vector<std::string> &results() const { return results_; }
  SimpleAgent(const std::string &agentId, asio::io_context &io_context, const std::string &host)
    : fetch::oef::Agent{std::unique_ptr<fetch::oef::OEFCoreInterface>(new fetch::oef::OEFCoreNetworkProxy{agentId, io_context, host})} {
      start();
    }
  void onError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation, stde::optional<uint32_t> dialogueId, stde::optional<uint32_t> msgId) override {}
  void onSearchResult(uint32_t search_id, const std::vector<std::string> &results) override {
    results_ = results;
  }
  void onMessage(const std::string &from, uint32_t dialogueId, const std::string &content) override {}
  void onCFP(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target, const fetch::oef::CFPType &constraints) override {}
  void onPropose(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target, const fetch::oef::ProposeType &proposals) override {}
  void onAccept(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target) override {}
  void onDecline(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target) override {}
  ~SimpleAgent() {}
 };


int main(int argc, char* argv[])
{
  bool showHelp = false;
  std::string host = "127.0.0.1";
  uint32_t nbAgents = 100;
  std::string prefix = "Agent_";
  
  auto parser = clara::Help(showHelp)
    | clara::Opt(nbAgents, "nbAgents")["--nbAgents"]["-n"]("Number of agents. Default 100.")
    | clara::Opt(prefix, "prefix")["--prefix"]["-p"]("Prefix used for all agents name. Default: Agent_")
    | clara::Opt(host, "host")["--host"]["-h"]("Host address to connect. Default: 127.0.0.1");
  auto result = parser.parse(clara::Args(argc, argv));

  if(showHelp || argc == 1) {
    std::cout << parser << std::endl;
  } 
  // need to increase max nb file open
  // > ulimit -n 8000
  // ulimit -n 1048576

  std::vector<std::unique_ptr<SimpleAgent>> agents;
  std::vector<std::future<std::unique_ptr<SimpleAgent>>> futures;
  IoContextPool pool(10);
  pool.run();
  try {
    for(size_t i = 1; i <= nbAgents; ++i) {
      std::string name = prefix;
      name += std::to_string(i);
      futures.push_back(std::async(std::launch::async, [&pool,&host](const std::string &n){
          return std::make_unique<SimpleAgent>(n, pool.getIoContext(), host.c_str());
      }, name));
    }
    std::cerr << "Futures created\n";
    for(auto &fut : futures) {
      agents.emplace_back(fut.get());
    }
    std::cerr << "Futures got\n";
  } catch(std::exception &e) {
    std::cerr << "BUG " << e.what() << "\n";
  }
  return 0;
}
