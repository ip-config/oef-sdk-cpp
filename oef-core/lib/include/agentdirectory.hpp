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

#include "logger.hpp"
#include "schema.hpp"
#include <memory>

namespace fetch {
    namespace oef {
        class AgentSession;

        class AgentDirectory {
        private:
            mutable std::mutex lock_;
            std::unordered_map<std::string,std::shared_ptr<AgentSession>> sessions_;

            static fetch::oef::Logger logger;

        public:
            AgentDirectory() = default;
            AgentDirectory(const AgentDirectory &) = delete;
            AgentDirectory operator=(const AgentDirectory &) = delete;
            bool exist(const std::string &id) const {
                return sessions_.find(id) != sessions_.end();
            }
            bool add(const std::string &id, std::shared_ptr<AgentSession> session) {
                std::lock_guard<std::mutex> lock(lock_);
                if(exist(id))
                    return false;
                sessions_[id] = std::move(session);
                return true;
            }
            bool remove(const std::string &id) {
                std::lock_guard<std::mutex> lock(lock_);
                return sessions_.erase(id) == 1;
            }
            void clear() {
                std::lock_guard<std::mutex> lock(lock_);
                sessions_.clear();
            }
            std::shared_ptr<AgentSession> session(const std::string &id) const {
                std::lock_guard<std::mutex> lock(lock_);
                auto iter = sessions_.find(id);
                if(iter != sessions_.end()) {
                    return iter->second;
                }
                return std::shared_ptr<AgentSession>(nullptr);
            }
            size_t size() const {
                std::lock_guard<std::mutex> lock(lock_);
                return sessions_.size();
            }
            const std::vector<std::string> search(const QueryModel &query) const;
        };
    }
}
