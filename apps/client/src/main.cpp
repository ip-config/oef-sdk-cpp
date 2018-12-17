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
#include "agent.hpp"
#include "oefcoreproxy.hpp"

class SimpleAgent : public fetch::oef::Agent {
 public:
  SimpleAgent(const std::string &agentId, asio::io_context &io_context, const std::string &host)
    : fetch::oef::Agent{std::unique_ptr<fetch::oef::OEFCoreInterface>(new fetch::oef::OEFCoreNetworkProxy{agentId, io_context, host})} {
      start();
    }
  void onError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation, stde::optional<uint32_t> dialogueId, stde::optional<uint32_t> msgId) override {}
  void onSearchResult(uint32_t search_id, const std::vector<std::string> &results) override {}
  void onMessage(const std::string &from, uint32_t dialogueId, const std::string &content) override {}
  void onCFP(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target, const fetch::oef::CFPType &constraints) override {}
  void onPropose(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target, const fetch::oef::ProposeType &proposals) override {}
  void onAccept(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target) override {}
  void onDecline(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target) override {}
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: client <agentID> <host>\n";
      return 1;
    }
    IoContextPool pool(2);
    pool.run();
    SimpleAgent client(argv[1], pool.getIoContext(), argv[2]);
    std::cout << "Enter destination: ";
    std::string destId;
    std::getline(std::cin, destId);
    std::cout << "Enter message: ";
    std::string message;
    std::getline(std::cin, message);
    Uuid32 uuid = Uuid32::uuid();
    client.sendMessage(uuid.val(), destId, message);
    std::cout << "Reply is: ";
    //    std::string reply = client.read(message.size());
    //    std::cout << reply << std::endl;
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
