/*
 * (C) Copyright 1996-2012 ECMWF.
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 * In applying this licence, ECMWF does not waive the privileges and immunities 
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

// Baudouin Raoult - ECMWF Sep 96

#ifndef eckit_DateTime_h
#define eckit_DateTime_h


#include "eckit/types/Date.h"
#include "eckit/types/Time.h"

//-----------------------------------------------------------------------------

namespace eckit {

//-----------------------------------------------------------------------------

class DateTime {
public:

// -- Contructors

	DateTime(time_t = ::time(0));
	DateTime(const Date&, const Time&);
	DateTime(const string&);

#include "eckit/types/DateTime.b"

// -- Destructor

	~DateTime() {}

// -- Operators

	bool operator<(const DateTime& other) const
		{ return (date_ == other.date_)
			?(time_ < other.time_)
			:(date_ < other.date_); }

	bool operator==(const DateTime& other) const
		{ return (date_ == other.date_) && (time_ == other.time_); }

	bool operator!=(const DateTime& other) const
		{ return (date_ != other.date_) || (time_ != other.time_); }

	bool operator>(const DateTime& other) const
		{ return (date_ == other.date_)
			?(time_ > other.time_)
			:(date_ > other.date_); }

	bool operator>=(const DateTime& other) const
		{ return !(*this < other); }

	bool operator<=(const DateTime& other) const
		{ return !(*this > other); }

	DateTime& operator=(const DateTime&);

	Second operator-(const DateTime&) const;
	DateTime operator+(const Second&) const;

	operator string() const;

// -- Methods

	const Date& date() const { return date_; }
	const Time& time() const { return time_; }

	DateTime round(const Second& seconds) const;

	void dump(DumpLoad&) const;
	void load(DumpLoad&);

	// -- Class methods
protected:

// -- Members

	Date date_;
	Time time_;

// -- Methods
	// None

// -- Overridden methods
	// None

// -- Class members
	// None

// -- Class methods
	// None

private:

// -- Members
	// None

// -- Methods

	void print(ostream&) const;

// -- Class methods

// -- Friends

	friend ostream& operator<<(ostream& s,const DateTime& p)
		{ p.print(s); return s; }
};


//-----------------------------------------------------------------------------

} // namespace eckit

#endif
