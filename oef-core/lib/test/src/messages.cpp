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
#include "schema.hpp"
#include "agent.pb.h"
#include <google/protobuf/text_format.h>

namespace Test {

  TEST_CASE("message serialization", "[serialization]") {
    fetch::oef::pb::Agent_Server_ID id;
    id.set_public_key("Agent1");
    std::string output;
    REQUIRE(google::protobuf::TextFormat::PrintToString(id, &output));
    std::cout << output;

    fetch::oef::pb::Server_Phrase phrase;
    phrase.set_phrase("RandomlyGeneratedString");
    REQUIRE(google::protobuf::TextFormat::PrintToString(phrase, &output));
    std::cout << output;

    fetch::oef::pb::Agent_Server_Answer answer;
    answer.set_answer("gnirtSdetareneGylmodnaR");
    REQUIRE(google::protobuf::TextFormat::PrintToString(answer, &output));
    std::cout << output;
    
    fetch::oef::pb::Server_Connected good, bad;
    good.set_status(true);
    bad.set_status(false);
    REQUIRE(google::protobuf::TextFormat::PrintToString(good, &output));
    std::cout << output;
    REQUIRE(google::protobuf::TextFormat::PrintToString(bad, &output));
    std::cout << output;
    /*
    Uuid uuid = Uuid::uuid4();
    Delivered goodD(uuid, true), badD(uuid, false);
    std::cout << toJsonString<Delivered>(goodD) << "\n";
    std::cout << toJsonString<Delivered>(badD) << "\n";

    Attribute manufacturer{"manufacturer", Type::String, true};
    Attribute colour{"colour", Type::String, false};
    Attribute luxury{"luxury", Type::Bool, true};
    DataModel car{"car", {manufacturer, colour, luxury}, stde::optional<std::string>{"Car sale."}};
    ConstraintType eqTrue{ConstraintType::ValueType{Relation{Relation::Op::Eq, true}}};
    Constraint luxury_c{luxury, eqTrue};
    QueryModel q1{{luxury_c}, stde::optional<DataModel>{car}};

    auto e1 = Envelope::makeQuery(q1);
    std::cout << toJsonString<Envelope>(e1) << "\n";
    auto e1b = fromJsonString<Envelope>(toJsonString<Envelope>(e1));
    std::cout << toJsonString<Envelope>(e1b) << "\n";
    Instance ferrari{car, {{"manufacturer", "Ferrari"}, {"colour", "Aubergine"}, {"luxury", "true"}}};
    auto e2 = Envelope::makeRegister(ferrari);
    std::cout << toJsonString<Envelope>(e2) << "\n";
    auto e2b = fromJsonString<Envelope>(toJsonString<Envelope>(e2));
    std::cout << toJsonString<Envelope>(e2b) << "\n";
    
    auto e3 = Envelope::makeMessage(uuid, "Agent2", "Hello world");
    std::cout << toJsonString<Envelope>(e3) << "\n";
    Instance lemon{car, {{"manufacturer", "Hyundai"}, {"colour", "Pink"}, {"luxury", "false"}}};
    auto e4 = Envelope::makeUnregister(lemon);
    std::cout << toJsonString<Envelope>(e4) << "\n";
    */
  }
}
