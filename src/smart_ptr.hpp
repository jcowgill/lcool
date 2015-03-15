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

#ifndef LCOOL_SMART_PTR_HPP
#define LCOOL_SMART_PTR_HPP

#include <memory>
#include <utility>

namespace lcool
{
	// If only C++11 had make_unique...

	using std::make_shared;
	using std::unique_ptr;
	using std::shared_ptr;

	template<typename T, typename ...Args>
	unique_ptr<T> make_unique(Args&& ...args)
	{
		return unique_ptr<T>(new T(std::forward<Args>(args)...));
	}
}

#endif
