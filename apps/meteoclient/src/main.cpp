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
#include <unordered_map>

#include "oefcoreproxy.hpp"
#include "agent.hpp"
#include "uuid.hpp"
#include "dialogue.hpp"

using namespace fetch::oef;

class MeteoDialogue;
class MeteoGroupDialogues;

class MeteoClientDialogueAgent : public DialogueAgent {
  std::unique_ptr<MeteoGroupDialogues> group_;
public:
  MeteoClientDialogueAgent(const std::string &agentId, asio::io_context &io_context, const std::string &host)
    : fetch::oef::DialogueAgent{std::unique_ptr<fetch::oef::OEFCoreInterface>(new fetch::oef::OEFCoreNetworkProxy{agentId, io_context, host})} {
    start();
  }
  void onSearchResult(uint32_t search_id, const std::vector<std::string> &results) override;
  void onNewMessage(uint32_t msgId, uint32_t dialogueId, const std::string &from, const std::string &content) override {}
  void onNewCFP(uint32_t msgId, uint32_t dialogueId, const std::string &from, uint32_t target, const CFPType &constraints) override {}
  void onOEFError(uint32_t answerId, fetch::oef::pb::Server_AgentMessage_OEFError_Operation operation) override {}
  void onDialogueError(uint32_t answerId, uint32_t dialogueId, const std::string &origin) override {}
};

class MeteoDialogue : public SingleDialogue {
  MeteoClientDialogueAgent &meteo_agent_;
  std::function<void(const std::string &,double)> notify_;
  uint32_t dataReceived_ = 0;
  
public:
  MeteoDialogue(MeteoClientDialogueAgent &agent, std::string destination, std::function<void(const std::string &,double)> notify)
    : SingleDialogue(agent, destination), meteo_agent_{agent}, notify_{notify} {
    sendCFP(0, 0, CFPType{stde::nullopt});
  }
  void onMessage(uint32_t msgId, const std::string &content) override {
    Data temp{content};
    std::cerr << "**Received " << temp.name() << " = " << temp.values().front() << std::endl;
    ++dataReceived_;
    if(dataReceived_ == 3) {
      agent_.stop();
    }
  }
  void onPropose(uint32_t msgId, uint32_t target, const ProposeType &proposals) override {
    stde::optional<VariantType> s_price;
    proposals.match(
                    [this](const std::string &s) {
                      assert(false);
                    },
                    [this,&s_price](const std::vector<Instance> &is) {
                      assert(is.size() == 1);
                      s_price = is.front().value("price");
                    });
    assert(s_price);
    double currentPrice = s_price.value().get<double>();
    notify_(destination_, currentPrice);
  }
  void sendAnswer(const std::string &winner) {
    if(destination_ == winner) {
      // msgId and target should be given by protocol
      sendAccept(2, 1);
    } else {
      sendDecline(2, 1);
      //agent_.stop();
    }
  }
  void onCFP(uint32_t msgId, uint32_t target, const CFPType &constraints) override {}
  void onAccept(uint32_t msgId, uint32_t target) override {}
  void onDecline(uint32_t msgId, uint32_t target) override {}
  void onDialogueError(uint32_t answerId, uint32_t dialogueId, const std::string &origin) override {}
};

class MeteoGroupDialogues : public GroupDialogues {
public:
  MeteoGroupDialogues(MeteoClientDialogueAgent &meteo_agent, const std::vector<std::string> &agents) : GroupDialogues{meteo_agent} {
    std::vector<std::shared_ptr<SingleDialogue>> dialogues;
    for(auto &c : agents) {
      dialogues.push_back(std::make_shared<MeteoDialogue>(meteo_agent, c, [this](const std::string &from, uint64_t price) {
                                                                            this->update(from, price);
                                                                           }));
    }
    addAgents(dialogues);
  }
  bool better(uint64_t price1, uint64_t price2) const override {
    return price1 < price2;
  }
  void finished() override {
    for(auto &p : dialogues_) {
      MeteoDialogue *d = dynamic_cast<MeteoDialogue*>(p.second.get());
      d->sendAnswer(bestAgent_);
    }
  }
};

void MeteoClientDialogueAgent::onSearchResult(uint32_t search_id, const std::vector<std::string> &results) {
  std::cerr << "onSearchResults " << results.size() << std::endl;
  group_ = std::make_unique<MeteoGroupDialogues>(*this, results);
}

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3) {
      std::cerr << "Usage: meteoclient <agentID> <host>\n";
      return 1;
    }
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%n] [%l] %v");
    spdlog::set_level(spdlog::level::level_enum::trace);
    IoContextPool pool(2);
    pool.run();

    MeteoClientDialogueAgent client(argv[1], pool.getIoContext(), argv[2]);

    // Build up our DataModel (this is identical to the meteostations DataModel, wouldn't work if not)
    Attribute wind        { "wind_speed",   Type::Bool, true};
    Attribute temperature { "temperature",  Type::Bool, true};
    Attribute air         { "air_pressure", Type::Bool, true};
    Attribute humidity    { "humidity",     Type::Bool, true};
    std::vector<Attribute> attributes{wind,temperature,air,humidity};
    DataModel weather{"weather_data", attributes, "All possible weather data."};

    // Create constraints against our Attributes (whether the station CAN provide them)
    Relation eqTrue{Relation::Op::Eq, true};
    Constraint temperature_c { temperature.name(), eqTrue};
    Constraint air_c         { air.name(),         eqTrue};
    Constraint humidity_c    { humidity.name(),    eqTrue};

    // Construct a Query schema and send it to the Node
    QueryModel q1{{ConstraintExpr{temperature_c},ConstraintExpr{air_c},ConstraintExpr{humidity_c}}, weather};
    client.searchServices(1, q1);
    pool.join();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
