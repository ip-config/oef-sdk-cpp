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

class MeteoClientDialogueAgent : public DialogueAgent {
  std::vector<std::shared_ptr<MeteoDialogue>> meteoDialogues_;
  std::string bestStation_;
  double bestPrice_ = -1.0f;
  size_t nbAnswers_ = 0;
public:
  uint32_t dataReceived_ = 0;
  MeteoClientDialogueAgent(const std::string &agentId, asio::io_context &io_context, const std::string &host)
    : fetch::oef::DialogueAgent{std::unique_ptr<fetch::oef::OEFCoreInterface>(new fetch::oef::OEFCoreNetworkProxy{agentId, io_context, host})} {
    start();
  }
  std::string bestStation() const { return bestStation_; }
  double bestPrice() const { return bestPrice_; }

  void update(const std::string &from, double price) {
    if(bestPrice_ == -1.0 || bestPrice_ > price) {
      bestPrice_ = price;
      bestStation_ = from;
    }
    ++nbAnswers_;
    if(nbAnswers_ >= dialogues_.size()) {
      sendAnswer();
    }
  }
  void sendAnswer();
  void onSearchResult(uint32_t search_id, const std::vector<std::string> &results) override;
  void onNewMessage(const std::string &from, uint32_t dialogueId, const std::string &content) override {}
  void onNewCFP(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target, const CFPType &constraints) override {}
  void onConnectionError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation) override {}
};

class MeteoDialogue : public SingleDialogue {
  MeteoClientDialogueAgent &meteo_agent_;
  
public:
  MeteoDialogue(MeteoClientDialogueAgent &agent, std::string destination)
    : SingleDialogue(agent, destination), meteo_agent_{agent} {
    sendCFP(CFPType{stde::nullopt});
  }
  void onMessage(const std::string &content) override {
    assert(destination_ == meteo_agent_.bestStation());
    Data temp{content};
    std::cerr << "**Received " << temp.name() << " = " << temp.values().front() << std::endl;
    ++meteo_agent_.dataReceived_;
    if(meteo_agent_.dataReceived_ == 3) {
      agent_.stop();
    }
  }
  void onPropose(uint32_t msgId, uint32_t target, const ProposeType &proposals) override {
    stde::optional<std::string> s_price;
    proposals.match(
                    [this](const std::string &s) {
                      assert(false);
                    },
                    [this,&s_price](const std::vector<Instance> &is) {
                      assert(is.size() == 1);
                      s_price = is.front().value("price");
                    });
    assert(s_price);
    double currentPrice = std::stod(s_price.value());
    meteo_agent_.update(destination_, currentPrice);
  }
  void sendAnswer(const std::string &winner) {
    if(destination_ == winner) {
      // msgId and target should be given by protocol
      sendAccept(2, 1);
    } else {
      sendDecline(2, 1);
    }
  }
  void onCFP(uint32_t msgId, uint32_t target, const CFPType &constraints) override {}
  void onAccept(uint32_t msgId, uint32_t target) override {}
  void onDecline(uint32_t msgId, uint32_t target) override {}
};

void MeteoClientDialogueAgent::onSearchResult(uint32_t search_id, const std::vector<std::string> &results) {
  std::cerr << "onSearchResults " << results.size() << std::endl;
  if(results.empty()) {
    std::cerr << "No candidates\n";
  }
  for(auto &c : results) {
    meteoDialogues_.push_back(std::make_shared<MeteoDialogue>(*this, c));
    registerDialogue(meteoDialogues_.back());
  }
}
void MeteoClientDialogueAgent::sendAnswer() {
  for(auto &d : meteoDialogues_) {
    d->sendAnswer(bestStation_);
  }
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
    ConstraintType eqTrue{Relation{Relation::Op::Eq, true}};
    Constraint temperature_c { temperature, eqTrue};
    Constraint air_c         { air,         eqTrue};
    Constraint humidity_c    { humidity,    eqTrue};

    // Construct a Query schema and send it to the Node
    QueryModel q1{{temperature_c,air_c,humidity_c}, weather};
    client.searchServices(1, q1);
    pool.join();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
