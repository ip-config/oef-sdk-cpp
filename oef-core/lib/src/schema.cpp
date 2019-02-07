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

namespace fetch {
  namespace oef {
    ConstraintExpr::ConstraintExpr(const Or &orp) {
      auto *o = constraint_.mutable_or_();
      o->CopyFrom(orp.handle());
    }

    ConstraintExpr::ConstraintExpr(const And &andp) {
      auto *a = constraint_.mutable_and_();
      a->CopyFrom(andp.handle());
    }

    ConstraintExpr::ConstraintExpr(const Not &notp) {
      auto *a = constraint_.mutable_not_();
      a->CopyFrom(notp.handle());
    }

    ConstraintExpr::ConstraintExpr(const Constraint &constraint) {
      auto *a = constraint_.mutable_constraint();
      a->CopyFrom(constraint.handle());
    }
    
    Constraint::operator ConstraintExpr() const { return ConstraintExpr{*this}; }

    ConstraintExpr operator!(const ConstraintExpr &expr) {
      return Not{expr};
    }

    ConstraintExpr operator&&(const ConstraintExpr &lhs, const ConstraintExpr &rhs) {
      return And{{lhs, rhs}};
    }

    ConstraintExpr operator||(const ConstraintExpr &lhs, const ConstraintExpr &rhs) {
      return Or{{lhs, rhs}};
    }
    
    bool ConstraintExpr::valid(const fetch::oef::pb::Query_ConstraintExpr &constraint, const fetch::oef::pb::Query_DataModel &dm) {
      auto expr_case = constraint.expression_case();
      switch(expr_case) {
      case fetch::oef::pb::Query_ConstraintExpr::kOr:
        return Or::valid(constraint.or_(), dm);
      case fetch::oef::pb::Query_ConstraintExpr::kAnd:
        return And::valid(constraint.and_(), dm);
      case fetch::oef::pb::Query_ConstraintExpr::kNot:
        return Not::valid(constraint.not_(), dm);
      case fetch::oef::pb::Query_ConstraintExpr::kConstraint:
        return Constraint::valid(constraint.constraint(), dm);
      case fetch::oef::pb::Query_ConstraintExpr::EXPRESSION_NOT_SET:
        // should not reach this line
        return false;
      }
      return false;
    }

    bool ConstraintExpr::check(const fetch::oef::pb::Query_ConstraintExpr &constraint, const VariantType &v) {
      auto expr_case = constraint.expression_case();
      switch(expr_case) {
      case fetch::oef::pb::Query_ConstraintExpr::kOr:
        return Or::check(constraint.or_(), v);
      case fetch::oef::pb::Query_ConstraintExpr::kAnd:
        return And::check(constraint.and_(), v);
      case fetch::oef::pb::Query_ConstraintExpr::kNot:
        return Not::check(constraint.not_(), v);
      case fetch::oef::pb::Query_ConstraintExpr::kConstraint:
        return Constraint::check(constraint.constraint(), v);
      case fetch::oef::pb::Query_ConstraintExpr::EXPRESSION_NOT_SET:
        // should not reach this line
        return false;
      }
      return false;
    }
    
    bool ConstraintExpr::check(const fetch::oef::pb::Query_ConstraintExpr &constraint, const Instance &i) {
      auto expr_case = constraint.expression_case();
      switch(expr_case) {
      case fetch::oef::pb::Query_ConstraintExpr::kOr:
        return Or::check(constraint.or_(), i);
      case fetch::oef::pb::Query_ConstraintExpr::kAnd:
        return And::check(constraint.and_(), i);
      case fetch::oef::pb::Query_ConstraintExpr::kNot:
        return Not::check(constraint.not_(), i);
      case fetch::oef::pb::Query_ConstraintExpr::kConstraint:
        return Constraint::check(constraint.constraint(), i);
      case fetch::oef::pb::Query_ConstraintExpr::EXPRESSION_NOT_SET:
        // should not reach this line
        return false;
      }
      return false;
    }
  } // namespace oef
} // namespace fetch


