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

#include <sstream>
#include <random>
#include <iomanip>      // std::setfill, std::setw

class Uuid32 {
 private:
  uint32_t val_;
 public:
  explicit Uuid32(uint32_t ab) : val_{ab}  {}
  static Uuid32 uuid() {
    static std::random_device rd;
    static std::uniform_int_distribution<uint32_t> dist(0, (uint32_t)(~0));

    uint32_t ab = dist(rd);
    return Uuid32{ab};
  }
  uint32_t val() const { return val_; }
};

class Uuid {
 private:
  uint64_t ab_, cd_;
  explicit Uuid(uint64_t ab, uint64_t cd) : ab_{ab}, cd_{cd} {}
 public:
  explicit Uuid(const std::string &s) : ab_{0}, cd_{0} {
    char sep;
    uint64_t a,b,c,d,e;
    auto idx = s.find_first_of('-');
    if(idx != std::string::npos) {
      std::stringstream ss{s};
      if(ss >> std::hex >> a >> sep >> b >> sep >> c >> sep >> d >> sep >> e) {
        if(ss.eof()) {
          ab_ = (a << 32) | (b << 16) | c;
          cd_ = (d << 48) | e;
        }
      }
    }
  }
 public:
  static Uuid uuid4() {
    static std::random_device rd;
    static std::uniform_int_distribution<uint64_t> dist(0, (uint64_t)(~0));

    uint64_t ab = dist(rd);
    uint64_t cd = dist(rd);
    
    ab = (ab & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    cd = (cd & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;
    
    return Uuid{ab, cd};
  }
  /* template <typename Archive> */
	/* explicit Uuid(const Archive &ar) : Uuid{ar.getString("UUID")} {} */
	/* template <typename Archive> */
	/* void serialize(Archive &ar) const { */
  /*   const std::string s = to_string(); */
	/* 	ar.write("UUID", s); */
	/* } */
  size_t hash() const {
    return ab_ ^ cd_;
  }
  bool operator==(const Uuid &other) {
    return ab_ == other.ab_ && cd_ == other.cd_;
  }
  bool operator!=(const Uuid &other) {
    return !operator==(other);
  }
  bool operator<(const Uuid &other) {
    if(ab_ < other.ab_)
      return true;
    if(ab_ > other.ab_)
      return false;
    return cd_ < other.cd_;
  }
  std::string to_string() const {
    std::stringstream ss;
    ss << std::hex << std::nouppercase << std::setfill('0');
    uint32_t a = (ab_ >> 32);
    uint32_t b = (ab_ & 0xFFFFFFFF);
    uint32_t c = (cd_ >> 32);
    uint32_t d = (cd_ & 0xFFFFFFFF);
    ss << std::setw(8) << (a) << '-';
    ss << std::setw(4) << (b >> 16) << '-';
    ss << std::setw(4) << (b & 0xFFFF) << '-';
    ss << std::setw(4) << (c >> 16 ) << '-';
    ss << std::setw(4) << (c & 0xFFFF);
    ss << std::setw(8) << d;
    return ss.str();
  }
};
