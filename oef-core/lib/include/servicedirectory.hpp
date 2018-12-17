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

#include "schema.hpp"

#include <unordered_map>
#include <set>
#include <unordered_set>
#include <mutex>

namespace fetch {
  namespace oef {
    class Agents {
    private:
      std::unordered_set<std::string> agents_;
    public:
      explicit Agents() {}
      template <typename Archive>
      explicit Agents(const Archive &ar) {
        std::function<void(const Archive &)> f = [this](const Archive &iar) {
                                                   agents_.emplace(iar.getString());
                                                 };      
        ar.parseObjects(f);
      }
      bool insert(const std::string &agent) {
        return agents_.insert(agent).second;
      }
      bool erase(const std::string &agent) {
        return agents_.erase(agent) == 1;
      }
      size_t size() const {
        return agents_.size();
      }
      void copy(std::unordered_set<std::string> &s) const {
        std::copy(agents_.begin(), agents_.end(), std::inserter(s, s.end()));
      }
    };

    class ServiceDirectory {
    private:
      mutable std::mutex lock_;
      std::unordered_map<Instance,Agents> data_;
    public:
      explicit ServiceDirectory() = default;
      bool registerAgent(const Instance &instance, const std::string &agent) {
        std::lock_guard<std::mutex> lock(lock_);
        return data_[instance].insert(agent);
      }
      bool unregisterAgent(const Instance &instance, const std::string &agent) {
        std::lock_guard<std::mutex> lock(lock_);
        auto iter = data_.find(instance);
        if(iter == data_.end())
          return false;
        bool res = iter->second.erase(agent);
        if(iter->second.size() == 0) {
          data_.erase(instance);
        }
        return res;
      }
      void unregisterAll(const std::string &agent) {
        std::lock_guard<std::mutex> lock(lock_);
        for(auto &p : data_) {
          p.second.erase(agent);
        }
      }
      size_t size() const {
        std::lock_guard<std::mutex> lock(lock_);
        return data_.size();
      }
      std::vector<std::string> query(const QueryModel &query) const {
        std::lock_guard<std::mutex> lock(lock_);
        std::unordered_set<std::string> res;
        for(auto &d : data_) {
          if(query.check(d.first)) {
            d.second.copy(res);
          }
        }
        return std::vector<std::string>(res.begin(), res.end());
      }
    };
  };
};
