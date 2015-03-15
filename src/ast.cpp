/*
 * Copyright (C) 2014-2015 James Cowgill
 *
 * LCool is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LCool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LCool.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ast.hpp"

namespace ast = lcool::ast;

void ast::assign         ::accept(ast::expr_visitor& visitor) const { visitor.visit(*this); }
void ast::dispatch       ::accept(ast::expr_visitor& visitor) const { visitor.visit(*this); }
void ast::conditional    ::accept(ast::expr_visitor& visitor) const { visitor.visit(*this); }
void ast::loop           ::accept(ast::expr_visitor& visitor) const { visitor.visit(*this); }
void ast::block          ::accept(ast::expr_visitor& visitor) const { visitor.visit(*this); }
void ast::let            ::accept(ast::expr_visitor& visitor) const { visitor.visit(*this); }
void ast::type_case      ::accept(ast::expr_visitor& visitor) const { visitor.visit(*this); }
void ast::new_object     ::accept(ast::expr_visitor& visitor) const { visitor.visit(*this); }
void ast::constant_bool  ::accept(ast::expr_visitor& visitor) const { visitor.visit(*this); }
void ast::constant_int   ::accept(ast::expr_visitor& visitor) const { visitor.visit(*this); }
void ast::constant_string::accept(ast::expr_visitor& visitor) const { visitor.visit(*this); }
void ast::identifier     ::accept(ast::expr_visitor& visitor) const { visitor.visit(*this); }
void ast::compute_unary  ::accept(ast::expr_visitor& visitor) const { visitor.visit(*this); }
void ast::compute_binary ::accept(ast::expr_visitor& visitor) const { visitor.visit(*this); }
