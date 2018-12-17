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

#include <hayai.hpp>
#include "agent.pb.h"
#include <iostream>

std::vector<uint8_t> data;

BENCHMARK(Serialization, ID, 10, 1000)
{
  fetch::oef::pb::Agent_Server_ID id;
  id.set_public_key("Agent1");
  std::string data(id.SerializeAsString());
}

BENCHMARK(Serialization, ID_Array, 10, 1000)
{
  fetch::oef::pb::Agent_Server_ID id;
  id.set_public_key("Agent1");
  size_t size = id.ByteSize();
  data.resize(size);
  (void)id.SerializeWithCachedSizesToArray(data.data());
}

BENCHMARK(Serialization, Phrase, 10, 1000)
{
  fetch::oef::pb::Server_Phrase phrase;
  phrase.set_phrase("RandomlyGeneratedString");
  std::string data(phrase.SerializeAsString());
}

BENCHMARK(Serialization, Phrase_Array, 10, 1000)
{
  fetch::oef::pb::Server_Phrase phrase;
  phrase.set_phrase("RandomlyGeneratedString");
  size_t size = phrase.ByteSize();
  data.resize(size);
  (void)phrase.SerializeWithCachedSizesToArray(data.data());
}

BENCHMARK(Serialization, Answer, 10, 1000)
{
  fetch::oef::pb::Agent_Server_Answer answer;
  answer.set_answer("gnirtSdetareneGylmodnaR");
  std::string data(answer.SerializeAsString());
}

BENCHMARK(Serialization, Answer_Array, 10, 1000)
{
  fetch::oef::pb::Agent_Server_Answer answer;
  answer.set_answer("gnirtSdetareneGylmodnaR");
  size_t size = answer.ByteSize();
  data.resize(size);
  answer.SerializeWithCachedSizesToArray(data.data());
}

BENCHMARK(Serialization, Connected, 100, 1000)
{
  fetch::oef::pb::Server_Connected good;
  good.set_status(true);
  std::string data(good.SerializeAsString());
}

BENCHMARK(Serialization, Connected_Array, 1, 1)
{
  fetch::oef::pb::Server_Connected good;
  good.set_status(true);
  size_t size = good.ByteSize();
  data.resize(size);
  (void)good.SerializeWithCachedSizesToArray(data.data());
}

BENCHMARK(Serialization, Connected_Array2, 1, 1)
{
  fetch::oef::pb::Server_Connected good;
  good.set_status(true);
  size_t size = good.ByteSize();
  data.resize(size);
  good.SerializeToArray(data.data(), size);
}

BENCHMARK(DeSerialization, ID, 10, 1000)
{
  fetch::oef::pb::Agent_Server_ID id;
  id.set_public_key("Agent1");
  fetch::oef::pb::Agent_Server_ID id2;
  id2.ParseFromString(id.SerializeAsString());
}

BENCHMARK(DeSerialization, ID_Array, 10, 1000)
{
  fetch::oef::pb::Agent_Server_ID id;
  id.set_public_key("Agent1");
  size_t size = id.ByteSize();
  data.resize(size);
  (void)id.SerializeWithCachedSizesToArray(data.data());
  fetch::oef::pb::Agent_Server_ID id2;
  id2.ParseFromArray(data.data(), size);
}

