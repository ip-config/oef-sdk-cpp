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

#include "agent.pb.h"
#include <algorithm>
#include <cmath>
#include <experimental/optional>
#include <iostream>
#include <limits>
#include "mapbox/variant.hpp"
#include <mutex>
#include <stdexcept>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace stde = std::experimental;
namespace var = mapbox::util; // for the variant

namespace fetch {
  namespace oef {
    constexpr double degree_to_radian(double angle) {
      return M_PI * angle / 180.0;
    }
    static constexpr double EarthRadiusKm = 6372.8;

    struct Location {
      double lon;
      double lat;
      bool operator==(const Location &other) const {
        return lon == other.lon && lat == other.lat;
      }
      bool operator!=(const Location &other) const {
        return lon != other.lon || lat != other.lat;
      }
      bool operator<(const Location &other) const {
        return lat < other.lat;
      }
      bool operator<=(const Location &other) const {
        return lat <= other.lat;
      }
      bool operator>(const Location &other) const {
        return lat > other.lat;
      }
      bool operator>=(const Location &other) const {
        return lat >= other.lat;
      }
      bool operator==(const fetch::oef::pb::Query_Location &other) const {
        return lat == other.lat() && lon == other.lon();
      }
      double distance(const Location &rhs) const {
        double latRad1 = degree_to_radian(lat);
        double latRad2 = degree_to_radian(rhs.lat);
        double lonRad1 = degree_to_radian(lon);
        double lonRad2 = degree_to_radian(rhs.lon);
        
        double diffLa = latRad2 - latRad1;
        double doffLo = lonRad2 - lonRad1;
        
        double computation = asin(sqrt(sin(diffLa / 2) * sin(diffLa / 2) + cos(latRad1) * cos(latRad2) * sin(doffLo / 2) * sin(doffLo / 2)));
        return 2 * EarthRadiusKm * computation;
      }
    };
    enum class Type {
                     Double = fetch::oef::pb::Query_Attribute_Type_DOUBLE,
                     Int = fetch::oef::pb::Query_Attribute_Type_INT,
                     Bool = fetch::oef::pb::Query_Attribute_Type_BOOL,
                     String = fetch::oef::pb::Query_Attribute_Type_STRING,
                     Location = fetch::oef::pb::Query_Attribute_Type_LOCATION };
    using VariantType = var::variant<int,double,std::string,bool,Location>;
    std::string to_string(const VariantType &v);
    
    class Attribute {
    private:
      fetch::oef::pb::Query_Attribute attribute_;
      static bool validate(const fetch::oef::pb::Query_Attribute &attribute, const VariantType &value) {
        auto type = attribute.type();
        bool res;
        value.match([&res,type](int) { res = type == fetch::oef::pb::Query_Attribute_Type_INT;},
                    [&res,type](double) { res = type == fetch::oef::pb::Query_Attribute_Type_DOUBLE;},
                    [&res,type](const std::string &) { res = type == fetch::oef::pb::Query_Attribute_Type_STRING;},
                    [&res,type](bool) { res = type == fetch::oef::pb::Query_Attribute_Type_BOOL;},
                    [&res,type](const Location &) {res = type == fetch::oef::pb::Query_Attribute_Type_LOCATION;});
        return res;
      }
    public:
      explicit Attribute(const std::string &name, Type type, bool required) {
        attribute_.set_name(name);
        attribute_.set_type(static_cast<fetch::oef::pb::Query_Attribute_Type>(type));
        attribute_.set_required(required);
      }
      explicit Attribute(const std::string &name, Type type, bool required,
                         const std::string &description)
        : Attribute{name, type, required} {
        attribute_.set_description(description);
      }
      const fetch::oef::pb::Query_Attribute &handle() const { return attribute_; }
      const std::string &name() const { return attribute_.name(); }
      fetch::oef::pb::Query_Attribute_Type type() const { return attribute_.type(); }
      bool required() const { return attribute_.required(); }
      static std::pair<std::string,std::string> instantiate(const std::unordered_map<std::string,VariantType> &values,
                                                            const fetch::oef::pb::Query_Attribute &attribute) {
        auto iter = values.find(attribute.name());
        if(iter == values.end()) {
          if(attribute.required()) {
            throw std::invalid_argument(std::string("Missing value: ") + attribute.name());
          }
          return std::make_pair(attribute.name(),"");
        }
        if(validate(attribute, iter->second)) {
          return std::make_pair(attribute.name(), to_string(iter->second));
        }
        throw std::invalid_argument(attribute.name() + std::string(" has a wrong type of value ") + to_string(iter->second));
      }
      std::pair<std::string,std::string> instantiate(const std::unordered_map<std::string,VariantType> &values) const {
        return instantiate(values, attribute_);
      }
    };
    
    class Relation {
    public:
      enum class Op {
                     Eq = fetch::oef::pb::Query_Relation_Operator_EQ,
                     Lt = fetch::oef::pb::Query_Relation_Operator_LT,
                     Gt = fetch::oef::pb::Query_Relation_Operator_GT,
                     LtEq = fetch::oef::pb::Query_Relation_Operator_LTEQ,
                     GtEq = fetch::oef::pb::Query_Relation_Operator_GTEQ,
                     NotEq = fetch::oef::pb::Query_Relation_Operator_NOTEQ};
    private:
      fetch::oef::pb::Query_Relation relation_;
      explicit Relation(Op op) {
        relation_.set_op(static_cast<fetch::oef::pb::Query_Relation_Operator>(op));
      }
    public:
      explicit Relation(Op op, const std::string &s) : Relation(op) {
        auto *val = relation_.mutable_val();
        val->set_s(s);
      }
      explicit Relation(Op op, const char *s) : Relation(op, std::string{s}) {}
      explicit Relation(Op op, int i) : Relation(op) {
        auto *val = relation_.mutable_val();
        val->set_i(i);
      }
      explicit Relation(Op op, bool b) : Relation(op) {
        auto *val = relation_.mutable_val();
        val->set_b(b);
      }
      explicit Relation(Op op, double d) : Relation(op) {
        auto *val = relation_.mutable_val();
        val->set_d(d);
      }
      explicit Relation(Op op, const Location &l) : Relation(op) {
        auto *val = relation_.mutable_val();
        auto *loc = val->mutable_l();
        loc->set_lon(l.lon);
        loc->set_lat(l.lat);
      }
      const fetch::oef::pb::Query_Relation &handle() const { return relation_; }
      template <typename T>
      static T get(const fetch::oef::pb::Query_Relation &rel) {
        const auto &val = rel.val();
        if(typeid(T) == typeid(int)) { // hashcode ?
          int i = val.i();
          return *reinterpret_cast<T*>(&(i));
        }
        if(typeid(T) == typeid(double)) { // hashcode ?
          double d = val.d();
          return *reinterpret_cast<T*>(&(d));
        }
        if(typeid(T) == typeid(std::string)) { // hashcode ?
          std::string s = val.s();
          return *reinterpret_cast<T*>(&(s));
        }
        if(typeid(T) == typeid(bool)) { // hashcode ?
          bool b = val.b();
          return *reinterpret_cast<T*>(&(b));
        }
        if(typeid(T) == typeid(Location)) { // hashcode ?
          const fetch::oef::pb::Query_Location &loc = val.l();
          Location l{loc.lon(), loc.lat()};
          return *reinterpret_cast<T*>(&(l));
        }
        throw std::invalid_argument("Not is not a valid type.");
      }
      template <typename T>
      static bool check_value(const fetch::oef::pb::Query_Relation &rel, const T &v) {
        const auto &s = get<T>(rel);
        switch(rel.op()) {
        case fetch::oef::pb::Query_Relation_Operator_EQ: return s == v;
        case fetch::oef::pb::Query_Relation_Operator_NOTEQ: return s != v;
        case fetch::oef::pb::Query_Relation_Operator_LT: return v < s;
        case fetch::oef::pb::Query_Relation_Operator_LTEQ: return v <= s;
        case fetch::oef::pb::Query_Relation_Operator_GT: return v > s;
        case fetch::oef::pb::Query_Relation_Operator_GTEQ: return v >= s;
        }
        return false;
      }
      static bool valid(const fetch::oef::pb::Query_Relation &rel, const fetch::oef::pb::Query_Attribute_Type &t) {
        auto op = rel.op();
        bool equal = op == fetch::oef::pb::Query_Relation_Operator_EQ || op == fetch::oef::pb::Query_Relation_Operator_NOTEQ;
        switch(rel.val().value_case()) {
        case fetch::oef::pb::Query_Value::kS:
          return t == fetch::oef::pb::Query_Attribute_Type_STRING;
        case fetch::oef::pb::Query_Value::kI:
          return t == fetch::oef::pb::Query_Attribute_Type_INT;
        case fetch::oef::pb::Query_Value::kD:
          return t == fetch::oef::pb::Query_Attribute_Type_DOUBLE;
        case fetch::oef::pb::Query_Value::kL:
          return t == fetch::oef::pb::Query_Attribute_Type_LOCATION && equal;
        case fetch::oef::pb::Query_Value::kB:
          return t == fetch::oef::pb::Query_Attribute_Type_BOOL && equal;
        case fetch::oef::pb::Query_Value::VALUE_NOT_SET:
          return false;
        }
        return false;
      }
      static bool check(const fetch::oef::pb::Query_Relation &rel, const VariantType &v) {
        bool res = false;
        v.match(
                [&rel,&res](int i) {res = check_value(rel, i);},
                [&rel,&res](double d) {res = check_value(rel, d);},
                [&rel,&res](const std::string &s) {res = check_value(rel, s);},
                [&rel,&res](bool b) {res = check_value(rel, b);},
                [&rel,&res](const Location &l) {res = check_value(rel, l);});
        return res;
      }
      bool check(const VariantType &v) const {
        return check(relation_, v);
      }
    };
    
    class Set {
    public:
      using ValueType = var::variant<std::unordered_set<int>,std::unordered_set<double>,
                                     std::unordered_set<std::string>,std::unordered_set<bool>>;
      enum class Op {
                     In = fetch::oef::pb::Query_Set_Operator_IN,
                     NotIn = fetch::oef::pb::Query_Set_Operator_NOTIN};
    private:
      fetch::oef::pb::Query_Set set_;
      
    public:
      explicit Set(Op op, const ValueType &values) {
        set_.set_op(static_cast<fetch::oef::pb::Query_Set_Operator>(op));
        fetch::oef::pb::Query_Set_Values *vals = set_.mutable_vals();
        values.match(
                     [vals](const std::unordered_set<int> &s) {
                       fetch::oef::pb::Query_Set_Values_Ints *ints = vals->mutable_i();
                       for(auto &v : s) {
                         ints->add_vals(v);
                       }},
                     [vals](const std::unordered_set<double> &s) {
                       fetch::oef::pb::Query_Set_Values_Doubles *doubles = vals->mutable_d();
                       for(auto &v : s) {
                         doubles->add_vals(v);
                       }},
                     [vals](const std::unordered_set<std::string> &s) {
                       fetch::oef::pb::Query_Set_Values_Strings *strings = vals->mutable_s();
                       for(auto &v : s) {
                         strings->add_vals(v);
                       }},
                     [vals](const std::unordered_set<bool> &s) {
                       fetch::oef::pb::Query_Set_Values_Bools *bools = vals->mutable_b();
                       for(auto &v : s) {
                         bools->add_vals(v);
                       }});
      }
      const fetch::oef::pb::Query_Set &handle() const { return set_; }
      static bool valid(const fetch::oef::pb::Query_Set &set, const fetch::oef::pb::Query_Attribute_Type &t) {
        switch(set.vals().values_case()) {
        case fetch::oef::pb::Query_Set_Values::kS:
          return t == fetch::oef::pb::Query_Attribute_Type_STRING;
        case fetch::oef::pb::Query_Set_Values::kI:
          return t == fetch::oef::pb::Query_Attribute_Type_INT;
        case fetch::oef::pb::Query_Set_Values::kD:
          return t == fetch::oef::pb::Query_Attribute_Type_DOUBLE;
        case fetch::oef::pb::Query_Set_Values::kL:
          return t == fetch::oef::pb::Query_Attribute_Type_LOCATION;
        case fetch::oef::pb::Query_Set_Values::kB:
          return t == fetch::oef::pb::Query_Attribute_Type_BOOL;
        case fetch::oef::pb::Query_Set_Values::VALUES_NOT_SET:
          return false;
        }
        return false;
      }
      static bool check(const fetch::oef::pb::Query_Set &set, const VariantType &v) {
        const auto &vals = set.vals();
        bool res = false;
        v.match(
                [&vals,&res](int i) {
                  for(auto &val : vals.i().vals()) {
                    if(val == i) {
                      res = true; 
                      return;
                    }
                  }
                },
                [&vals,&res](double d) {
                  for(auto &val : vals.d().vals()) {
                    if(val == d) {
                      res = true; 
                      return;
                    }
                  }
                },
                [&vals,&res](const std::string &st) {
                  for(auto &val : vals.s().vals()) {
                    if(val == st) {
                      res = true; 
                      return;
                    }
                  }
                },
                [&vals,&res](bool b) {
                  for(auto &val : vals.b().vals()) {
                    if(val == b) {
                      res = true; 
                      return;
                    }
                  }
                },
                [&vals,&res](const Location &l) {
                  for(auto &val : vals.l().vals()) {
                    if(l == val) {
                      res = true; 
                      return;
                    }
                  }
                });
        if(set.op() == fetch::oef::pb::Query_Set_Operator_NOTIN) {
          return !res;
        }
        return res;
      }
      bool check(const VariantType &v) const {
        return check(set_, v);
      }
    };

    class Distance {
    private:
      fetch::oef::pb::Query_Distance distance_;
    public:
      explicit Distance(const Location &center, double distance) {
        auto *location = distance_.mutable_center();
        location->set_lon(center.lon);
        location->set_lat(center.lat);
        distance_.set_distance(distance);
      }
      const fetch::oef::pb::Query_Distance &handle() const { return distance_; }
      static bool valid(const fetch::oef::pb::Query_Distance &dist, const fetch::oef::pb::Query_Attribute_Type &t) {
        return t == fetch::oef::pb::Query_Attribute_Type_LOCATION;
      }
      static bool check(const fetch::oef::pb::Query_Distance &distance, const VariantType &v) {
        bool res = false;
        v.match(
                [](int i) {},
                [](double d) {},
                [](const std::string &s) {},
                [](bool b) {},
                [&res,&distance](const Location &l) {
                  Location loc{distance.center().lon(), distance.center().lat()};
                  res = loc.distance(l) <= distance.distance();
                });
        return res;
      }
      bool check(const VariantType &v) const {
        return check(distance_, v);
      }
    };

    class Range {
    public:
      using ValueType = var::variant<std::pair<int,int>,std::pair<double,double>,std::pair<std::string,std::string>>;
    private:
      fetch::oef::pb::Query_Range range_;

      static void min_max(double a, double b, double &min, double &max) {
        if(a < b) {
          min = a;
          max = b;
        } else {
          min = b;
          max = a;
        }
      }
    public:
      explicit Range(const std::pair<int,int> &r) {
        fetch::oef::pb::Query_IntPair *p = range_.mutable_i();
        p->set_first(r.first);
        p->set_second(r.second);
      }
      explicit Range(const std::pair<double,double> &r) {
        fetch::oef::pb::Query_DoublePair *p = range_.mutable_d();
        p->set_first(r.first);
        p->set_second(r.second);
      }
      explicit Range(const std::pair<std::string,std::string> &r) {
        fetch::oef::pb::Query_StringPair *p = range_.mutable_s();
        p->set_first(r.first);
        p->set_second(r.second);
      }
      const fetch::oef::pb::Query_Range &handle() const { return range_; }
      static bool valid(const fetch::oef::pb::Query_Range &range, const fetch::oef::pb::Query_Attribute_Type &t) {
        switch(range.pair_case()) {
        case fetch::oef::pb::Query_Range::kS:
          return t == fetch::oef::pb::Query_Attribute_Type_STRING;
        case fetch::oef::pb::Query_Range::kI:
          return t == fetch::oef::pb::Query_Attribute_Type_INT;
        case fetch::oef::pb::Query_Range::kD:
          return t == fetch::oef::pb::Query_Attribute_Type_DOUBLE;
        case fetch::oef::pb::Query_Range::kL:
          return t == fetch::oef::pb::Query_Attribute_Type_LOCATION;
        case fetch::oef::pb::Query_Range::PAIR_NOT_SET:
          return false;
        }
        return false;
      }
      static bool check(const fetch::oef::pb::Query_Range &range, const VariantType &v) {
        bool res = false;
        v.match(
                [&res,&range](int i) {
                  auto &p = range.i();
                  res = i >= p.first() && i <= p.second();
                },
                [&res,&range](double d) {
                  auto &p = range.d();
                  res = d >= p.first() && d <= p.second();
                },
                [&res,&range](const std::string &s) {
                  auto &p = range.s();
                  res = s >= p.first() && s <= p.second();
                },
                [&res,&range](const Location &s) {
                  auto &p = range.l();
                  double min_lat, max_lat, min_lon, max_lon;
                  min_max(p.first().lat(), p.second().lat(), min_lat, max_lat);
                  min_max(p.first().lon(), p.second().lon(), min_lon, max_lon);
                  res = s.lat >= min_lat && s.lat <= max_lat && s.lon >= min_lon && s.lon <= max_lon;
                },
                [](bool b) {});
        return res;
      }
      bool check(const VariantType &v) const {
        return check(range_, v);
      }
    };
    
    class DataModel {
    private:
      fetch::oef::pb::Query_DataModel model_;
    public:
      explicit DataModel(const std::string &name, const std::vector<Attribute> &attributes) {
        std::unordered_set<std::string> att_set;
        for(auto &a : attributes) {
          auto pair = att_set.insert(a.name());
          if(!pair.second) { // attribute name already existed
            throw std::invalid_argument("Duplicate attribute name");
          }
        }
        model_.set_name(name);
        auto *atts = model_.mutable_attributes();
        for(auto &a : attributes) {
          auto *att = atts->Add();
          att->CopyFrom(a.handle());
        }
      }
      explicit DataModel(const std::string &name, const std::vector<Attribute> &attributes, const std::string &description)
        : DataModel{name, attributes} {
        model_.set_description(description);
      }
      const fetch::oef::pb::Query_DataModel &handle() const { return model_; }
      bool operator==(const DataModel &other) const
      {
        return model_.name() == other.model_.name();
        // TODO: should check more.
      }
      static stde::optional<fetch::oef::pb::Query_Attribute> attribute(const fetch::oef::pb::Query_DataModel &model,
                                                                       const std::string &name) {
        for(auto &a : model.attributes()) {
          if(a.name() == name) {
            return stde::optional<fetch::oef::pb::Query_Attribute>{a};
          }
        }
        return stde::nullopt;
      }
      std::string name() const { return model_.name(); }
      static std::vector<std::pair<std::string,std::string>>
      instantiate(const fetch::oef::pb::Query_DataModel &model, const std::unordered_map<std::string,VariantType> &values) {
        std::vector<std::pair<std::string,std::string>> res;
        for(auto &a : model.attributes()) {
          res.emplace_back(Attribute::instantiate(values, a));
        }
        return res;
      }
    };
    
    class Instance {
    private:
      fetch::oef::pb::Query_Instance instance_;
      std::unordered_map<std::string,VariantType> values_;
    public:
      explicit Instance(const DataModel &model, const std::unordered_map<std::string,VariantType> &values) : values_{values} {
        if(values.size() > size_t(model.handle().attributes_size())) {
          throw std::invalid_argument("Too many attributes");
        }
        size_t nb_required = 0;
        for(auto &att : model.handle().attributes()) {
          if(att.required())
            ++nb_required;
        }
        if(values.size() < nb_required) {
          throw std::invalid_argument("Not enough attributes");
        }
        auto *mod = instance_.mutable_model();
        mod->CopyFrom(model.handle());
        auto *vals = instance_.mutable_values();
        for(auto &v : values) {
          const auto iter = std::find_if(model.handle().attributes().begin(), model.handle().attributes().end(),
                                         [&v](const fetch::oef::pb::Query_Attribute &a) {
                                           return v.first == a.name();
                                         });
          if(iter == model.handle().attributes().end()) {
            // attribute does not exist in datamodel
            throw std::invalid_argument("Attribute does not exist in data model.");
          }
          auto att_type = iter->type();
          if(iter->required())
            --nb_required;
          auto *val = vals->Add();
          val->set_key(v.first);
          auto *value = val->mutable_value();
          v.second.match(
                         [att_type, value](int i) {
                           if(att_type != fetch::oef::pb::Query_Attribute_Type_INT) {
                             throw std::invalid_argument("Attribute is not an int in data model.");
                           }
                           value->set_i(i);
                         },
                         [att_type, value](double d) {
                           if(att_type != fetch::oef::pb::Query_Attribute_Type_DOUBLE) {
                             throw std::invalid_argument("Attribute is not a double in data model.");
                           }
                           value->set_d(d);
                         },
                         [att_type, value](const std::string &s) {
                           if(att_type != fetch::oef::pb::Query_Attribute_Type_STRING) {
                             throw std::invalid_argument("Attribute is not a string in data model.");
                           }
                           value->set_s(s);
                         },
                         [att_type, value](const Location &l) {
                           if(att_type != fetch::oef::pb::Query_Attribute_Type_LOCATION) {
                             throw std::invalid_argument("Attribute is not a location in data model.");
                           }
                           auto *loc = value->mutable_l();
                           loc->set_lon(l.lon);
                           loc->set_lat(l.lat);
                         },
                         [att_type, value](bool b) {
                           if(att_type != fetch::oef::pb::Query_Attribute_Type_BOOL) {
                             throw std::invalid_argument("Attribute is not a bool in data model.");
                           }
                           value->set_b(b);
                         });
        }
        if(nb_required > 0) {
          throw std::invalid_argument("Not enough attributes.");
        }
      }
      explicit Instance(const fetch::oef::pb::Query_Instance &instance) : instance_{instance}
      {
        const auto &values = instance_.values();
        for(auto &v : values) {
          switch(v.value().value_case()) {
          case fetch::oef::pb::Query_Value::kS:
            values_[v.key()] = VariantType{v.value().s()};
            break;
          case fetch::oef::pb::Query_Value::kD:
            values_[v.key()] = VariantType{v.value().d()};
            break;
          case fetch::oef::pb::Query_Value::kB:
            values_[v.key()] = VariantType{v.value().b()};
            break;
          case fetch::oef::pb::Query_Value::kI:
            values_[v.key()] = VariantType{int(v.value().i())};
            break;
          case fetch::oef::pb::Query_Value::kL:
            values_[v.key()] = VariantType{Location{v.value().l().lon(), v.value().l().lat()}};
            break;
          case fetch::oef::pb::Query_Value::VALUE_NOT_SET:
          default:
            break;
          }
        }
      }
      const fetch::oef::pb::Query_Instance &handle() const { return instance_; }
      bool operator==(const Instance &other) const
      {
        if(!(instance_.model().name() == other.instance_.model().name())) {
          return false;
        }
        for(const auto &p : values_) {
          const auto &iter = other.values_.find(p.first);
          if(iter == other.values_.end()) {
            return false;
          }
          if(iter->second != p.second) {
            return false;
          }
        }
        return true;
      }
      std::size_t hash() const {
        std::size_t h = std::hash<std::string>{}(instance_.model().name());
        for(const auto &p : values_) {
          std::size_t hs = std::hash<std::string>{}(p.first);
          h = hs ^ (h << 1);
          p.second.match([&hs](int i) { hs = std::hash<int>{}(i);},
                         [&hs](double d) { hs = std::hash<double>{}(d);},
                         [&hs](const std::string &s) { hs = std::hash<std::string>{}(s);},
                         [&hs](const Location &l) {
                           std::size_t h1 = std::hash<double>{}(l.lon);
                           hs = h1 ^ (std::hash<double>{}(l.lat) << 1);},
                         [&hs](bool b) { hs = std::hash<bool>{}(b);});
          h = hs ^ (h << 2);
        }
        return h;
      }
      
      std::vector<std::pair<std::string,std::string>>
      instantiate() const {
        return DataModel::instantiate(instance_.model(), values_);
      }
      const fetch::oef::pb::Query_DataModel &model() const {
        return instance_.model();
      }
      stde::optional<VariantType> value(const std::string &name) const {
        auto iter = values_.find(name);
        if(iter == values_.end()) {
          return stde::nullopt;
        }
        return stde::optional<VariantType>{iter->second};
      }
    };

    class ConstraintExpr;
    class Constraint {
    public:
      using ValueType = var::variant<Set,Range,Relation,Distance>;
    private:
      std::string attribute_name_;
      fetch::oef::pb::Query_ConstraintExpr_Constraint constraint_;
    public:
      explicit Constraint(std::string attribute_name, const Range &range) : attribute_name_{std::move(attribute_name)} {
        constraint_.set_attribute_name(attribute_name_);
        auto *r = constraint_.mutable_range_();
        r->CopyFrom(range.handle());
      }
      explicit Constraint(std::string attribute_name, const Relation &rel) : attribute_name_{std::move(attribute_name)} {
        constraint_.set_attribute_name(attribute_name_);
        auto *r = constraint_.mutable_relation();
        r->CopyFrom(rel.handle());
      }
      explicit Constraint(std::string attribute_name, const Set &set) : attribute_name_{std::move(attribute_name)} {
        constraint_.set_attribute_name(attribute_name_);
        auto *s = constraint_.mutable_set_();
        s->CopyFrom(set.handle());
      }
      explicit Constraint(std::string attribute_name, const Distance &distance) : attribute_name_{std::move(attribute_name)} {
        constraint_.set_attribute_name(attribute_name_);
        auto *s = constraint_.mutable_set_();
        s->CopyFrom(distance.handle());
      }
      operator ConstraintExpr() const;
      const fetch::oef::pb::Query_ConstraintExpr_Constraint &handle() const { return constraint_; }
      static bool check(const fetch::oef::pb::Query_ConstraintExpr_Constraint &constraint, const VariantType &v) {
        auto constraint_case = constraint.constraint_case();
        switch(constraint_case) {
        case fetch::oef::pb::Query_ConstraintExpr_Constraint::kSet:
          return Set::check(constraint.set_(), v);
        case fetch::oef::pb::Query_ConstraintExpr_Constraint::kRange:
          return Range::check(constraint.range_(), v);
        case fetch::oef::pb::Query_ConstraintExpr_Constraint::kRelation:
          return Relation::check(constraint.relation(), v);
        case fetch::oef::pb::Query_ConstraintExpr_Constraint::kDistance:
          return Distance::check(constraint.distance(), v);
        case fetch::oef::pb::Query_ConstraintExpr_Constraint::CONSTRAINT_NOT_SET:
          return false;
        }
        return false;
      }
      static bool valid(const fetch::oef::pb::Query_ConstraintExpr_Constraint &constraint, const fetch::oef::pb::Query_DataModel &dm) {
        auto constraint_case = constraint.constraint_case();
        const auto &att_name = constraint.attribute_name();
        const auto iter = std::find_if(dm.attributes().begin(), dm.attributes().end(),
                                    [&att_name](const fetch::oef::pb::Query_Attribute &a) {
                                      return att_name == a.name();
                                    });
        if(iter == dm.attributes().end()) {
          // attribute does not exist in datamodel
          return false;
        }
        switch(constraint_case) {
        case fetch::oef::pb::Query_ConstraintExpr_Constraint::kSet:
          return Set::valid(constraint.set_(), iter->type());
        case fetch::oef::pb::Query_ConstraintExpr_Constraint::kRange:
          return Range::valid(constraint.range_(), iter->type());
        case fetch::oef::pb::Query_ConstraintExpr_Constraint::kRelation:
          return Relation::valid(constraint.relation(), iter->type());
        case fetch::oef::pb::Query_ConstraintExpr_Constraint::kDistance:
          return Distance::valid(constraint.distance(), iter->type());
        case fetch::oef::pb::Query_ConstraintExpr_Constraint::CONSTRAINT_NOT_SET:
          return false;
        }
        return false;
      }
      static bool check(const fetch::oef::pb::Query_ConstraintExpr_Constraint &constraint, const Instance &i) {
        auto &attribute_name = constraint.attribute_name();
        auto attr = DataModel::attribute(i.model(), attribute_name);
        if(attr) {
          // Need to check attr->type with the constraint admissible types -> tricky
          // if(attr->type() != attribute.type()) {
          //   return false;
          // }
        }
        auto v = i.value(attribute_name);
        if(!v) {
          // if(attribute.required()) {
          //   std::cerr << "Should not happen!\n"; // Exception ?
          // }
          return false;
        }
        return check(constraint, *v);
      }
      bool check(const VariantType &v) const {
        return check(constraint_, v);
      }
    };
    
    class Or;
    class And;
    class Not;

    class ConstraintExpr {
    public:
      using ValueType = var::variant<var::recursive_wrapper<Or>,var::recursive_wrapper<And>,var::recursive_wrapper<Not>,Constraint>;
    private:
      fetch::oef::pb::Query_ConstraintExpr constraint_;
    public:
      explicit ConstraintExpr(const Or &orp);
      explicit ConstraintExpr(const And &andp);
      explicit ConstraintExpr(const Not &notp);
      explicit ConstraintExpr(const Constraint &constraint);
      const fetch::oef::pb::Query_ConstraintExpr &handle() const { return constraint_; }
      static bool check(const fetch::oef::pb::Query_ConstraintExpr &constraint, const VariantType &v);
      static bool check(const fetch::oef::pb::Query_ConstraintExpr &constraint, const Instance &i);
      static bool valid(const fetch::oef::pb::Query_ConstraintExpr &constraint, const fetch::oef::pb::Query_DataModel &dm);
      bool check(const VariantType &v) const {
        return check(constraint_, v);
      }
    };
    
    class Or {
      fetch::oef::pb::Query_ConstraintExpr_Or expr_;
    public:
      explicit Or(const std::vector<ConstraintExpr> &expr) {
        if(expr.size() < 2) {
          throw std::invalid_argument("Not enough parameters.");
        }
        auto *cts = expr_.mutable_expr();
        for(auto &e : expr) {
          auto *ct = cts->Add();
          ct->CopyFrom(e.handle());
        }
      }
      operator ConstraintExpr() const { return ConstraintExpr{*this}; }
      const fetch::oef::pb::Query_ConstraintExpr_Or &handle() const { return expr_; }
      static bool check(const fetch::oef::pb::Query_ConstraintExpr_Or &expr, const VariantType &v) {
        for(auto &c : expr.expr()) {
          if(ConstraintExpr::check(c, v)) {
            return true;
          }
        }
        return false;
      }
      static bool check(const fetch::oef::pb::Query_ConstraintExpr_Or &expr, const Instance &i) {
        for(auto &c : expr.expr()) {
          if(ConstraintExpr::check(c, i)) {
            return true;
          }
        }
        return false;
      }
      static bool valid(const fetch::oef::pb::Query_ConstraintExpr_Or &expr, const fetch::oef::pb::Query_DataModel &dm) {
        for(auto &c : expr.expr()) {
          if(!ConstraintExpr::valid(c, dm)) {
            return false;
          }
        }
        return expr.expr_size() > 1;
      }
    };
    
    class And {
    private:
      fetch::oef::pb::Query_ConstraintExpr_And expr_;
    public:
      explicit And(const std::vector<ConstraintExpr> &expr) {
        if(expr.size() < 2) {
          throw std::invalid_argument("Not enough parameters.");
        }
        auto *cts = expr_.mutable_expr();
        for(auto &e : expr) {
          auto *ct = cts->Add();
          ct->CopyFrom(e.handle());
        }
      }
      operator ConstraintExpr() const { return ConstraintExpr{*this}; }
      const fetch::oef::pb::Query_ConstraintExpr_And &handle() const { return expr_; }
      static bool check(const fetch::oef::pb::Query_ConstraintExpr_And &expr, const VariantType &v) {
        for(auto &c : expr.expr()) {
          if(!ConstraintExpr::check(c, v)) {
            return false;
          }
        }
        return true;
      }
      static bool check(const fetch::oef::pb::Query_ConstraintExpr_And &expr, const Instance &i) {
        for(auto &c : expr.expr()) {
          if(!ConstraintExpr::check(c, i)) {
            return false;
          }
        }
        return true;
      }
      static bool valid(const fetch::oef::pb::Query_ConstraintExpr_And &expr, const fetch::oef::pb::Query_DataModel &dm) {
        for(auto &c : expr.expr()) {
          if(!ConstraintExpr::valid(c, dm)) {
            return false;
          }
        }
        return expr.expr_size() > 1;
      }
    };

    class Not {
    private:
      fetch::oef::pb::Query_ConstraintExpr_Not expr_;
    public:
      explicit Not(const ConstraintExpr &expr) {
        auto *cts = expr_.mutable_expr();
        cts->CopyFrom(expr.handle());
      }
      operator ConstraintExpr() const { return ConstraintExpr{*this}; }
      const fetch::oef::pb::Query_ConstraintExpr_Not &handle() const { return expr_; }
      static bool check(const fetch::oef::pb::Query_ConstraintExpr_Not &expr, const VariantType &v) {
        return !ConstraintExpr::check(expr.expr(), v);
      }
      static bool check(const fetch::oef::pb::Query_ConstraintExpr_Not &expr, const Instance &i) {
        return !ConstraintExpr::check(expr.expr(), i);
      }
      static bool valid(const fetch::oef::pb::Query_ConstraintExpr_Not &expr, const fetch::oef::pb::Query_DataModel &dm) {
        return ConstraintExpr::valid(expr.expr(), dm);
      }
    };
    
    ConstraintExpr operator!(const ConstraintExpr &expr);
    ConstraintExpr operator&&(const ConstraintExpr &lhs, const ConstraintExpr &rhs);
    ConstraintExpr operator||(const ConstraintExpr &lhs, const ConstraintExpr &rhs);

    class QueryModel {
    private:
      fetch::oef::pb::Query_Model model_;
    public:
      explicit QueryModel(const std::vector<ConstraintExpr> &constraints) {
        if(constraints.size() < 1) {
          throw std::invalid_argument("Not enough parameters.");
        }
        auto *cts = model_.mutable_constraints();
        for(auto &c : constraints) {
          auto *ct = cts->Add();
          ct->CopyFrom(c.handle());
        }
      }
      explicit QueryModel(const std::vector<ConstraintExpr> &constraints, const DataModel &model) : QueryModel{constraints} {
        auto *m = model_.mutable_model();
        m->CopyFrom(model.handle());
        for(auto &c : model_.constraints()) {
          if(!ConstraintExpr::valid(c, model_.model())) {
            throw std::invalid_argument("Mismatch between constraints in data model.");
          }
        }
      }
      explicit QueryModel(const fetch::oef::pb::Query_Model &model) : model_{model} {}
      const fetch::oef::pb::Query_Model &handle() const { return model_; }
      template <typename T>
      bool check_value(const T &v) const {
        for(auto &c : model_.constraints()) {
          if(!ConstraintExpr::check(c, VariantType{v})) {
            return false;
          }
        }
        return true;
      }
      bool valid() const {
        // Empty expression is not valid
        if(model_.constraints_size() < 1) {
          return false;
        }
        if(!model_.has_model()) { // no model, so we cannot check.
          return true;
        }
        for(auto &c : model_.constraints()) {
          if(!ConstraintExpr::valid(c, model_.model())) {
            return false;
          }
        }
        return true;
      }
      bool check(const Instance &i) const {
        if(model_.has_model()) {
          if(model_.model().name() != i.model().name()) {
            return false;
          }
          // TODO: more to compare ?
        }
        for(auto &c : model_.constraints()) {
          if(!ConstraintExpr::check(c, i)) {
            return false;
          }
        }
        return true;
      }
    };
    
    class SchemaRef {
    private:
      std::string name_; // unique
      uint32_t version_;
    public:
      explicit SchemaRef(std::string name, uint32_t version) : name_{std::move(name)}, version_{version} {}
      std::string name() const { return name_; }
      uint32_t version() const { return version_; }
    };
    
    class Schema {
    private:
      uint32_t version_;
      DataModel schema_;
    public:
      explicit Schema(uint32_t version, const DataModel &schema) : version_{version}, schema_{schema} {}
      uint32_t version() const { return version_; }
      DataModel schema() const { return schema_; }
    };
    
    class Schemas {
    private:
      mutable std::mutex lock_;
      std::vector<Schema> schemas_;
    public:
      explicit Schemas() = default;
      uint32_t add(uint32_t version, const DataModel &schema) {
        std::lock_guard<std::mutex> lock(lock_);
        if(version == std::numeric_limits<uint32_t>::max()) {
          version = schemas_.size() + 1;
        }
        schemas_.emplace_back(Schema(version, schema));
        return version;
      }
      stde::optional<Schema> get(uint32_t version) const {
        std::lock_guard<std::mutex> lock(lock_);
        if(schemas_.empty()) {
          return stde::nullopt;
        }
        if(version == std::numeric_limits<uint32_t>::max()) {
          return schemas_.back();
        }
        for(auto &p : schemas_) { // TODO: binary search
          if(p.version() >= version) {
            return p;
          }
        }
        return schemas_.back();
      }
    };
    
    class SchemaDirectory {
    private:
      std::unordered_map<std::string, Schemas> schemas_;
    public:
      explicit SchemaDirectory() = default;
      stde::optional<Schema> get(const std::string &key, uint32_t version = std::numeric_limits<uint32_t>::max()) const {
        const auto &iter = schemas_.find(key);
        if(iter != schemas_.end()) {
          return iter->second.get(version);
        }
        return stde::nullopt;
      }
      uint32_t add(const std::string &key, const DataModel &schema, uint32_t version = std::numeric_limits<uint32_t>::max()) {
        return schemas_[key].add(version, schema);
      }
    };
    
    class Data {
    private:
      std::string name_;
      std::string type_;
      std::vector<std::string> values_;
      fetch::oef::pb::Data data_;
    public:
      explicit Data(std::string name, std::string type, const std::vector<std::string> &values)
        : name_{std::move(name)}, type_{std::move(type)}, values_{values} {
          data_.set_name(name);
          data_.set_type(type);
          for(auto &s : values) {
            data_.add_values(s);
          }
        }
      explicit Data(const std::string &buffer) {
        data_.ParseFromString(buffer);
        name_ = data_.name();
        type_ = data_.type();
        for(auto &s : data_.values()) {
          values_.emplace_back(s);
        }
      }
      const fetch::oef::pb::Data &handle() { return data_; }
      std::string name() const { return name_; }
      std::string type() const { return type_; }
      std::vector<std::string> values() const { return values_; }
      
    };
    using CFPType = var::variant<std::string, QueryModel, stde::nullopt_t>;
    using ProposeType = var::variant<std::string, std::vector<Instance>>;
  } // namespace oef
} // namespace fetch

namespace std
{  
  template<> struct hash<fetch::oef::Instance>  {
    size_t operator()(fetch::oef::Instance const& s) const noexcept
    {
      return s.hash();
    }
  };
} // namespace std
