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
#include <google/protobuf/text_format.h>
#include "agent.hpp"
#include "oefcoreproxy.hpp"
#include <unordered_set>

using namespace fetch::oef;

class MeteoStation : public fetch::oef::Agent
{
private:
  double unitPrice_;
  std::unordered_set<uint32_t> dialogues_;
  
public:
  MeteoStation(const std::string &agentId, asio::io_context &io_context, const std::string &host)
    : fetch::oef::Agent{std::unique_ptr<fetch::oef::OEFCoreInterface>(new fetch::oef::OEFCoreNetworkProxy{agentId, io_context, host})} {
      start();
      static std::vector<VariantType> properties = { VariantType{true}, VariantType{true}, VariantType{true}, VariantType{false}};
      static std::random_device rd;
      static std::mt19937 g(rd());
      static Attribute wind{"wind_speed", Type::Bool, true};
      static Attribute temp{"temperature", Type::Bool, true};
      static Attribute air{"air_pressure", Type::Bool, true};
      static Attribute humidity{"humidity", Type::Bool, true};
      static std::vector<Attribute> attributes{wind,temp,air,humidity};
      static DataModel weather{"weather_data", attributes, "All possible weather data."};
      
      std::shuffle(properties.begin(), properties.end(), g);
      static std::normal_distribution<double> dist{1.0, 0.1}; // mean,stddev
      unitPrice_ = dist(g);
      std::cerr << getPublicKey() << " " << unitPrice_ << std::endl;
      std::unordered_map<std::string,VariantType> props;
      int i = 0;
      for(auto &a : attributes) {
        props[a.name()] = properties[i];
        ++i;
      }
      Instance instance{weather, props};
      registerService(1, instance);
    }
  MeteoStation(const MeteoStation &) = delete;
  MeteoStation operator=(const MeteoStation &) = delete;

  void onOEFError(uint32_t answerId, fetch::oef::pb::Server_AgentMessage_OEFError_Operation operation) override {}
  void onDialogueError(uint32_t answerId, uint32_t dialogueId, const std::string &origin) override {}
  void onSearchResult(uint32_t, const std::vector<std::string> &results) override {}
  void onMessage(uint32_t msgId, uint32_t dialogueId, const std::string &from, const std::string &content) override {}
  void onCFP(uint32_t msgId, uint32_t dialogueId, const std::string &from, uint32_t target, const fetch::oef::CFPType &constraints) override {
    auto price = Attribute{"price", Type::Int, true};

    auto model = DataModel{"weather_data", {price}};
    auto desc = Instance{model, {{"price", VariantType{unitPrice_}}}};
    sendPropose(msgId + 1, dialogueId, from, target + 1, ProposeType{std::vector<Instance>({desc})});
  }
  void onPropose(uint32_t msgId, uint32_t dialogueId, const std::string &from, uint32_t target, const fetch::oef::ProposeType &proposals) override {}
  void onAccept(uint32_t msgId, uint32_t dialogueId, const std::string &from, uint32_t target) override {
    // let's send the data
    std::cerr << "I won!\n";
    std::random_device rd;
    std::mt19937 g(rd());
    std::normal_distribution<double> dist{15.0, 2.0};
    Data temp{"temperature", "double", {std::to_string(dist(g))}};
    sendMessage(1, dialogueId, from, temp.handle().SerializeAsString());
    Data air{"air pressure", "double", {std::to_string(dist(g))}};
    sendMessage(2, dialogueId, from, air.handle().SerializeAsString());
    Data humid{"humidity", "double", {std::to_string(dist(g))}};
    sendMessage(3, dialogueId, from, humid.handle().SerializeAsString());
  }
  void onDecline(uint32_t msgId, uint32_t dialogueId, const std::string &from, uint32_t target) override {
    std::cerr << "I lost\n";
  }
};

int main(int argc, char* argv[])
{
  uint32_t nbStations = 1;
  bool showHelp = false;
  std::string host;

  auto parser = clara::Help(showHelp)
    | clara::Arg(host, "host")("Host address to connect.")
    | clara::Opt(nbStations, "stations")["--stations"]["-n"]("Number of meteo stations to simulate.");

  auto result = parser.parse(clara::Args(argc, argv));
  if(showHelp || argc == 1) {
    std::cout << parser << std::endl;
  } else {
    try {
      IoContextPool pool(2);
      pool.run();
      std::vector<std::unique_ptr<MeteoStation>> stations;
      for(uint32_t i = 1; i <= nbStations; ++i) {
        std::string name = "Station_" + std::to_string(i);
        stations.emplace_back(std::make_unique<MeteoStation>(name, pool.getIoContext(), host));
      }
      pool.join();
    } catch(std::exception &e) {
      std::cerr << "Exception " << e.what() << std::endl;
    }
  }

  return 0;
}
