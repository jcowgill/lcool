/*
 * Copyright (C) 2014 James Cowgill
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

#ifndef LCOOL_LOGGER_HPP
#define LCOOL_LOGGER_HPP

#include <boost/format/format_fwd.hpp>
#include <cstdint>
#include <iosfwd>
#include <string>

#include "smart_ptr.hpp"

namespace lcool
{
	/** A position in a code file (used for logging errors and debugging) */
	class location
	{
	public:
		/** Filename of this position */
		shared_ptr<const std::string> filename;

		/** Start line of the position (first line is 1) */
		std::uint32_t line;

		/** Start column of the position (first column is 1) */
		std::uint32_t column;

		/** Convert this location into a string */
		std::string to_string() const;
	};

	/** Prints a location to an output stream (according to to_string) */
	std::ostream& operator<< (std::ostream& stream, const location& loc);

	/** Class used to log compiler warnings and errors */
	class logger
	{
	public:
		virtual ~logger() { }

		/** Returns true if errors have occured */
		virtual bool has_errors() const = 0;

		/** Prints a warning */
		virtual void warning(const std::string& str) = 0;

		/** Prints a warning which occured at the given location */
		virtual void warning(const location& loc, const std::string& str) = 0;

		/** Prints an error */
		virtual void error(const std::string& str) = 0;

		/** Prints an error which occured at the given location */
		virtual void error(const location& loc, const std::string& str) = 0;

		// Boost format versions
		void warning(const boost::format& format);
		void warning(const location& loc, const boost::format& format);
		void error(const boost::format& format);
		void error(const location& loc, const boost::format& format);
	};

	/** Implementation of logger which prints to an ostream */
	class logger_ostream : public logger
	{
	public:
		/** Constructs a logger using clog as its stream */
		logger_ostream();

		/** Constructs a logger using the given stream */
		explicit logger_ostream(std::ostream& stream);

		virtual bool has_errors() const override;
		virtual void warning(const std::string& str) override;
		virtual void warning(const location& loc, const std::string& str) override;
		virtual void error(const std::string& str) override;
		virtual void error(const location& loc, const std::string& str) override;

	private:
		std::ostream& stream;
		bool errors_printed = false;
	};
}

#endif
