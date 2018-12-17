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
#include <experimental/optional>
#include <iostream>
#include <limits>
#include "mapbox/variant.hpp"
#include <mutex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace stde = std::experimental;
namespace var = mapbox::util; // for the variant

namespace fetch {
  namespace oef {
    enum class Type {
                     Double = fetch::oef::pb::Query_Attribute_Type_DOUBLE,
                     Int = fetch::oef::pb::Query_Attribute_Type_INT,
                     Bool = fetch::oef::pb::Query_Attribute_Type_BOOL,
                     String = fetch::oef::pb::Query_Attribute_Type_STRING };
    using VariantType = var::variant<int,double,std::string,bool>;
    VariantType string_to_value(fetch::oef::pb::Query_Attribute_Type t, const std::string &s);
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
                    [&res,type](bool) { res = type == fetch::oef::pb::Query_Attribute_Type_BOOL;});
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
      static bool check(const fetch::oef::pb::Query_Relation &rel, const VariantType &v) {
        bool res = false;
        v.match(
                [&rel,&res](int i) {res = check_value(rel, i);},
                [&rel,&res](double d) {res = check_value(rel, d);},
                [&rel,&res](const std::string &s) {res = check_value(rel, s);},
                [&rel,&res](bool b) {res = check_value(rel, b);});        
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
    class Range {
    public:
      using ValueType = var::variant<std::pair<int,int>,std::pair<double,double>,std::pair<std::string,std::string>>;
    private:
      fetch::oef::pb::Query_Range range_;
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
                [&res](bool b) {
                  res = false; // doesn't make sense
                });        
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
        auto *mod = instance_.mutable_model();
        mod->CopyFrom(model.handle());
        auto *vals = instance_.mutable_values();
        for(auto &v : values) {
          auto *val = vals->Add();
          val->set_key(v.first);
          auto *value = val->mutable_value();
          v.second.match([value](int i) { value->set_i(i);},
                         [value](double d) { value->set_d(d);},
                         [value](const std::string &s) { value->set_s(s);},
                         [value](bool b) {value->set_b(b);});
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
      stde::optional<std::string> value(const std::string &name) const {
        auto iter = values_.find(name);
        if(iter == values_.end()) {
          return stde::nullopt;
        }
        return stde::optional<std::string>{to_string(iter->second)};
      }
    };
        
    class Or;
    class And;
    
    class ConstraintType {
    public:
      using ValueType = var::variant<var::recursive_wrapper<Or>,var::recursive_wrapper<And>,Range,Relation,Set>;
    private:
      fetch::oef::pb::Query_Constraint_ConstraintType constraint_;
    public:
      explicit ConstraintType(const Or &orp);
      explicit ConstraintType(const And &andp);
      explicit ConstraintType(const Range &range) {
        auto *r = constraint_.mutable_range_();
        r->CopyFrom(range.handle());
      }
      explicit ConstraintType(const Relation &rel) {
        auto *r = constraint_.mutable_relation();
        r->CopyFrom(rel.handle());
      }
      explicit ConstraintType(const Set &set) {
        auto *s = constraint_.mutable_set_();
        s->CopyFrom(set.handle());
      }
      const fetch::oef::pb::Query_Constraint_ConstraintType &handle() const { return constraint_; }
      static bool check(const fetch::oef::pb::Query_Constraint_ConstraintType &constraint, const VariantType &v);
      bool check(const VariantType &v) const {
        return check(constraint_, v);
      }
    };
    
    class Constraint {
    private:
      fetch::oef::pb::Query_Constraint constraint_;
    public:
      explicit Constraint(const Attribute &attribute, const ConstraintType &constraint) {
        auto *c = constraint_.mutable_attribute();
        c->CopyFrom(attribute.handle());
        auto *ct = constraint_.mutable_constraint();
        ct->CopyFrom(constraint.handle());
      }
      const fetch::oef::pb::Query_Constraint &handle() const { return constraint_; }
      static bool check(const fetch::oef::pb::Query_Constraint &constraint, const VariantType &v) {
        return ConstraintType::check(constraint.constraint(), v);
      }
      static bool check(const fetch::oef::pb::Query_Constraint &constraint, const Instance &i) {
        auto &attribute = constraint.attribute();
        auto attr = DataModel::attribute(i.model(), attribute.name());
        if(attr) {
          if(attr->type() != attribute.type()) {
            return false;
          }
        }
        auto v = i.value(attribute.name());
        if(!v) {
          if(attribute.required()) {
            std::cerr << "Should not happen!\n"; // Exception ?
          }
          return false;
        }
        VariantType value{string_to_value(attribute.type(), *v)};
        return check(constraint, value);
      }
      bool check(const VariantType &v) const {
        return check(constraint_, v);
      }
    };
    
    class Or {
      fetch::oef::pb::Query_Constraint_ConstraintType_Or expr_;
    public:
      explicit Or(const std::vector<ConstraintType> &expr) {
        auto *cts = expr_.mutable_expr();
        for(auto &e : expr) {
          auto *ct = cts->Add();
          ct->CopyFrom(e.handle());
        }
      }
      const fetch::oef::pb::Query_Constraint_ConstraintType_Or &handle() const { return expr_; }
      static bool check(const fetch::oef::pb::Query_Constraint_ConstraintType_Or &expr, const VariantType &v) {
        for(auto &c : expr.expr()) {
          if(ConstraintType::check(c, v)) {
            return true;
          }
        }
        return false;
      }
    };
    
    class And {
    private:
      fetch::oef::pb::Query_Constraint_ConstraintType_And expr_;
    public:
      explicit And(const std::vector<ConstraintType> &expr) {
        auto *cts = expr_.mutable_expr();
        for(auto &e : expr) {
          auto *ct = cts->Add();
          ct->CopyFrom(e.handle());
        }
      }
      const fetch::oef::pb::Query_Constraint_ConstraintType_And &handle() const { return expr_; }
      static bool check(const fetch::oef::pb::Query_Constraint_ConstraintType_And &expr, const VariantType &v) {
        for(auto &c : expr.expr()) {
          if(!ConstraintType::check(c, v)) {
            return false;
          }
        }
        return true;
      }
    };
    
    class QueryModel {
    private:
      fetch::oef::pb::Query_Model model_;
    public:
      explicit QueryModel(const std::vector<Constraint> &constraints) {
        auto *cts = model_.mutable_constraints();
        for(auto &c : constraints) {
          auto *ct = cts->Add();
          ct->CopyFrom(c.handle());
        }
      }
      explicit QueryModel(const std::vector<Constraint> &constraints, const DataModel &model) : QueryModel{constraints} {
        auto *m = model_.mutable_model();
        m->CopyFrom(model.handle());
      }
      explicit QueryModel(const fetch::oef::pb::Query_Model &model) : model_{model} {}
      const fetch::oef::pb::Query_Model &handle() const { return model_; }
      template <typename T>
      bool check_value(const T &v) const {
        for(auto &c : model_.constraints()) {
          if(!Constraint::check(c, VariantType{v})) {
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
          if(!Constraint::check(c, i)) {
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
