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

#include <boost/format.hpp>
#include <iostream>
#include <string>

#include "logger.hpp"

std::string lcool::location::to_string() const
{
	return boost::str(boost::format("%1%:%2%:%3%") % *filename % line % column);
}

std::ostream& lcool::operator<< (std::ostream& stream, const lcool::location& loc)
{
	return stream << loc.to_string();
}

void lcool::logger::warning(const boost::format& format)
{
	warning(format.str());
}

void lcool::logger::warning(const lcool::location& loc, const boost::format& format)
{
	warning(loc, format.str());
}

void lcool::logger::error(const boost::format& format)
{
	error(format.str());
}

void lcool::logger::error(const lcool::location& loc, const boost::format& format)
{
	error(loc, format.str());
}

lcool::logger_ostream::logger_ostream()
	: stream(std::clog)
{
}

lcool::logger_ostream::logger_ostream(std::ostream& stream)
	: stream(stream)
{
}

bool lcool::logger_ostream::has_errors() const
{
	return errors_printed;
}

void lcool::logger_ostream::warning(const std::string& str)
{
	stream << "warning: " << str << std::endl;
}

void lcool::logger_ostream::warning(const lcool::location& loc, const std::string& str)
{
	stream << loc << ": warning: " << str << std::endl;
}

void lcool::logger_ostream::error(const std::string& str)
{
	stream << "error: " << str << std::endl;
	errors_printed = true;
}

void lcool::logger_ostream::error(const lcool::location& loc, const std::string& str)
{
	stream << loc << ": error: " << str << std::endl;
	errors_printed = true;
}
