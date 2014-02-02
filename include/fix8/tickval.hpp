//-------------------------------------------------------------------------------------------------
/*

Fix8 is released under the GNU LESSER GENERAL PUBLIC LICENSE Version 3.

Fix8 Open Source FIX Engine.
Copyright (C) 2010-14 David L. Dight <fix@fix8.org>

Fix8 is free software: you can  redistribute it and / or modify  it under the  terms of the
GNU Lesser General  Public License as  published  by the Free  Software Foundation,  either
version 3 of the License, or (at your option) any later version.

Fix8 is distributed in the hope  that it will be useful, but WITHOUT ANY WARRANTY;  without
even the  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

You should  have received a copy of the GNU Lesser General Public  License along with Fix8.
If not, see <http://www.gnu.org/licenses/>.

BECAUSE THE PROGRAM IS  LICENSED FREE OF  CHARGE, THERE IS NO  WARRANTY FOR THE PROGRAM, TO
THE EXTENT  PERMITTED  BY  APPLICABLE  LAW.  EXCEPT WHEN  OTHERWISE  STATED IN  WRITING THE
COPYRIGHT HOLDERS AND/OR OTHER PARTIES  PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY OF ANY
KIND,  EITHER EXPRESSED   OR   IMPLIED,  INCLUDING,  BUT   NOT  LIMITED   TO,  THE  IMPLIED
WARRANTIES  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS TO
THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE,
YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

IN NO EVENT UNLESS REQUIRED  BY APPLICABLE LAW  OR AGREED TO IN  WRITING WILL ANY COPYRIGHT
HOLDER, OR  ANY OTHER PARTY  WHO MAY MODIFY  AND/OR REDISTRIBUTE  THE PROGRAM AS  PERMITTED
ABOVE,  BE  LIABLE  TO  YOU  FOR  DAMAGES,  INCLUDING  ANY  GENERAL, SPECIAL, INCIDENTAL OR
CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT
NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR
THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH
HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

*/
//-------------------------------------------------------------------------------------------------
#ifndef FIX8_TICKVAL_HPP_
#define FIX8_TICKVAL_HPP_

//-------------------------------------------------------------------------------------------------
#ifndef _MSC_VER
#include <sys/time.h>
#include <limits.h>
#endif
#if (THREAD_SYSTEM == THREAD_PTHREAD)
#include<pthread.h>
#elif (THREAD_SYSTEM == THREAD_POCO)
#endif

//-------------------------------------------------------------------------------------------------
namespace FIX8 {

//-------------------------------------------------------------------------------------------------
#ifdef __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

//-------------------------------------------------------------------------------------------------
#ifdef _MSC_VER

inline LARGE_INTEGER getFILETIMEoffset()
{
	SYSTEMTIME s = { 1970, 1, 1, 0, 0, 0, 0 };
	FILETIME f;
	LARGE_INTEGER t;

	SystemTimeToFileTime(&s, &f);
	t.QuadPart = f.dwHighDateTime;
	t.QuadPart <<= 32;
	t.QuadPart |= f.dwLowDateTime;
	return t;
}

// Windows does not have clock_gettime, so we must supply it
inline int clock_gettime(int X, struct timespec *tv)
{
    LARGE_INTEGER t;
    FILETIME f;
    double nanoseconds;
    //static LARGE_INTEGER    offset;
    static double frequencyToNanoseconds;
    static int initialized = 0;
    static BOOL usePerformanceCounter = 0;

    if (!initialized)
	 {
        LARGE_INTEGER performanceFrequency;
        initialized = 1;
        usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
        if (usePerformanceCounter)
		  {
            //QueryPerformanceCounter(&offset);
            frequencyToNanoseconds = (double)performanceFrequency.QuadPart / 1000000000.;
        }
		  else
		  {
            //offset = getFILETIMEoffset();
            frequencyToNanoseconds = 10.;
        }
    }
    if (usePerformanceCounter)
		 QueryPerformanceCounter(&t);
    else
	 {
        GetSystemTimeAsFileTime(&f);
        t.QuadPart = f.dwHighDateTime;
        t.QuadPart <<= 32;
        t.QuadPart |= f.dwLowDateTime;
    }

    //t.QuadPart -= offset.QuadPart;
    nanoseconds = (double)t.QuadPart / frequencyToNanoseconds;
    t.QuadPart = nanoseconds;
    tv->tv_sec = t.QuadPart / 1000000000;
    tv->tv_nsec = t.QuadPart % 1000000000;
    return 0;
}

#define CLOCK_REALTIME 0

#endif // _MSC_VER

//-------------------------------------------------------------------------------------------------
// OS X does not have clock_gettime, use clock_get_time
inline void current_utc_time(struct timespec *ts)
{
#ifdef __APPLE__
	clock_serv_t cclock;
	mach_timespec_t mts;
	host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);
	ts->tv_sec = mts.tv_sec;
	ts->tv_nsec = mts.tv_nsec;
#else
	clock_gettime(CLOCK_REALTIME, ts);
#endif
}

//---------------------------------------------------------------------------------------------------
/// High resolution time in nanosecond ticks. Thread safe.
class Tickval
{
public:
	typedef unsigned long long ticks; // unsigned ticks
	typedef long long sticks; // signed ticks
	static const ticks noticks = 0ULL;
	static const sticks nosticks = 0LL;
	static const ticks errorticks = ULLONG_MAX;
	static const ticks thousand = 1000ULL;
	static const ticks million = thousand * thousand;
	static const ticks billion = thousand * million;
	static const ticks second = billion;
	static const ticks minute = 60 * second;
	static const ticks hour = 60 * minute;
	static const ticks day = 24 * hour;
	static const ticks week = 7 * day;

private:
	ticks _value; // We're ok till 03:14:08 UTC on 19 January 2038

public:
	/*! Ctor.
	  \param settonow if true, construct with current time */
	explicit Tickval(bool settonow=false) : _value(settonow ? _cvt(get_timespec()) : noticks) {}

	/*! Copy Ctor. */
	Tickval(const Tickval& from) : _value(from._value) {}

	/*! Ctor.
	  \param from construct from raw ticks value (nanoseconds) */
	Tickval(const ticks from) : _value(from) {}

	/*! Ctor.
	  \param secs seconds
	  \param nsecs nanoseconds */
	Tickval(const time_t secs, const long nsecs)
      : _value(billion * static_cast<ticks>(secs) + static_cast<ticks>(nsecs)) {}

	/*! Ctor.
	  \param from construct from timespec object */
	explicit Tickval(const timespec& from) : _value(_cvt(from)) {}

	/*! Ctor.
	  \param from construct from time_t value */
	explicit Tickval(const time_t from) : _value(static_cast<ticks>(from) * billion) {}

	/*! Assignment operator. */
	Tickval& operator=(const Tickval& that)
	{
		if (this != &that)
			_value = that._value;
		return *this;
	}

	/*! Assignment operator from ticks
	  \param that ticks to assign from
	  \return *this */
	Tickval& operator=(const ticks that)
	{
		_value = that;
		return *this;
	}

	/*! Assignment operator from timespec.
	  \param that timespec object to assign from
	  \return *this */
	Tickval& operator=(const timespec& that)
	{
		_value = _cvt(that);
		return *this;
	}

	/*! Get the raw tick value (nanosecs).
	  \return raw ticks */
	ticks get_ticks() const { return _value; }

	/*! Assign the current time to this object.
	  \return current object */
	Tickval& now() { return get_tickval(*this); }

	/*! Get the current number of elapsed seconds
	  \return value in seconds */
	time_t secs() const { return static_cast<time_t>(_value / billion); }

	/*! Get the current number of elapsed milliseconds (excluding secs)
	  \return value in milliseconds */
	unsigned msecs() const { return static_cast<unsigned>((_value % billion) / million); }

	/*! Get the current number of elapsed microseconds (excluding secs)
	  \return value in microseconds */
	unsigned usecs() const { return static_cast<unsigned>((_value % billion) / thousand); }

	/*! Get the current number of elapsed nanoseconds (excluding secs)
	  \return value in nanoseconds */
	unsigned nsecs() const { return static_cast<unsigned>(_value % billion); }

	/*! Get the current tickval as a double.
	  \return value as a double */
	double todouble() const { return static_cast<double>(_value) / billion; }

	/*! See if this Tickval holds an error value
	  \return true if an error value */
	bool is_errorval() const { return _value == errorticks; }

	/*! See if this Tickval is within the range given
	  \param a lower boundary
	  \param b upper boundary; if an error value, ignore upper range
	  \return true if in range */
	bool in_range(const Tickval& a, const Tickval& b) const
	{
		return !b.is_errorval() ? a <= *this && *this <= b : a <= *this;
	}

	/*! Adjust Tickval by given signed value (sticks)
	  \param by amount to adjust; if +ve add, if -ve sub
	  \return adjusted Tickval */
	Tickval& adjust(sticks by)
	{
		if (by > 0LL)
			*this += by;
		else if (by < 0LL)
			*this -= -by;
		return *this;
	}

	/*! Generate a timespec object
	  \return timespec */
	static timespec get_timespec()
	{
		timespec ts;
		current_utc_time(&ts);
		return ts;
	}

	/*! Generate a Tickval object, constructed with the current time.
	  \return Tickval */
	static Tickval get_tickval()
	{
		timespec ts;
		current_utc_time(&ts);
		return Tickval(ts);
	}

	/*! Assign current time to a given Tickval object.
	  \return target Tickval object */
	static Tickval& get_tickval(Tickval& to)
	{
		timespec ts;
		current_utc_time(&ts);
		return to = ts;
	}

	/*! Get tickval as struct timespec
	  \return result */
	struct timespec as_ts() const
	{
		const timespec ts = { secs(), static_cast<long>(nsecs()) };
		return ts;
	}

	/*! Get tickval as struct tm
	  \param result ref to struct tm to fill
	  \return ptr to result */
	struct tm *as_tm(struct tm& result) const
	{
		const time_t insecs(secs());
#ifdef _MSC_VER
		gmtime_s(&result, &insecs);
		return &result;
#else
		return gmtime_r(&insecs, &result);
#endif
	}

	/*! Set from secs/nsecs
	  \param secs seconds
	  \param nsecs nanoseconds */
	void set(const time_t secs, const long nsecs)
      { _value = billion * static_cast<ticks>(secs) + static_cast<ticks>(nsecs); }

	/*! Not operator
	  \return true if no ticks (0) */
	bool operator!() const { return _value == noticks; }

	/*! Not 0 operator
	  \return true if ticks */
	operator void*() { return _value == noticks ? 0 : this; }

	/*! Cast to unsigned long long.
	  \return ticks as unsigned long long */
	operator Tickval::ticks () { return _value; }

	/*! Cast to double.
	  \return ticks as double */
	operator double() { return todouble(); }

	/*! Binary negate operator.
	  \param newtime Tickval to subtract from
	  \param oldtime Tickval to subtract
	  \return Result as Tickval */
	friend Tickval operator-(const Tickval& newtime, const Tickval& oldtime)
		{ return Tickval(newtime._value - oldtime._value); }

	/*! Binary negate operator.
	  \param newtime Tickval to subtract from
	  \param oldtime Tickval to subtract
	  \return resulting oldtime */
	friend Tickval& operator-=(Tickval& oldtime, const Tickval& newtime)
	{
		oldtime._value -= newtime._value;
		return oldtime;
	}

	/*! Binary addition operator.
	  \param newtime Tickval to add to
	  \param oldtime Tickval to add
	  \return Result as Tickval */
	friend Tickval operator+(const Tickval& newtime, const Tickval& oldtime)
		{ return Tickval(newtime._value + oldtime._value); }

	/*! Binary addition operator.
	  \param newtime Tickval to add to
	  \param oldtime Tickval to add
	  \return resulting oldtime */
	friend Tickval& operator+=(Tickval& oldtime, const Tickval& newtime)
	{
		oldtime._value += newtime._value;
		return oldtime;
	}

	/*! Equivalence operator.
	  \param a lhs Tickval
	  \param b rhs Tickval
	  \return true if equal */
	friend bool operator==(const Tickval& a, const Tickval& b)
		{ return a._value == b._value; }

	/*! Inequivalence operator.
	  \param a lhs Tickval
	  \param b rhs Tickval
	  \return true if not equal */
	friend bool operator!=(const Tickval& a, const Tickval& b)
		{ return a._value != b._value; }

	/*! Greater than operator.
	  \param a lhs Tickval
	  \param b rhs Tickval
	  \return true if a is greater than b */
	friend bool operator>(const Tickval& a, const Tickval& b)
		{ return a._value > b._value; }

	/*! Less than operator.
	  \param a lhs Tickval
	  \param b rhs Tickval
	  \return true if a is less than b */
	friend bool operator<(const Tickval& a, const Tickval& b)
		{ return a._value < b._value; }

	/*! Greater than or equal to operator.
	  \param a lhs Tickval
	  \param b rhs Tickval
	  \return true if a is greater than or equal to b */
	friend bool operator>=(const Tickval& a, const Tickval& b)
		{ return a._value >= b._value; }

	/*! Less than or equal to operator.
	  \param a lhs Tickval
	  \param b rhs Tickval
	  \return true if a is less than or equal to b */
	friend bool operator<=(const Tickval& a, const Tickval& b)
		{ return a._value <= b._value; }

	/*! Binary addition operator.
	  \param oldtime Tickval to add to
	  \param ns ticks to add
	  \return oldtime as Tickval */
	friend Tickval& operator+=(Tickval& oldtime, const ticks& ns)
	{
		oldtime._value += ns;
		return oldtime;
	}

	/*! Binary negate operator.
	  \param oldtime Tickval to subtract from
	  \param ns Tickval to subtract
	  \return oldtime as Tickval */
	friend Tickval& operator-=(Tickval& oldtime, const ticks& ns)
	{
		oldtime._value -= ns;
		return oldtime;
	}

	/*! ostream inserter friend; Tickval printer, localtime
	  \param os stream to insert into
	  \param what Tickval to insert
	  \return ostream object */
	friend std::ostream& operator<<(std::ostream& os, const Tickval& what)
	{
		std::string result;
		return os << GetTimeAsStringMS(result, &what, 9);
	}

	/*! ostream inserter friend; Tickval printer, gmtime
	  \param os stream to insert into
	  \param what Tickval to insert
	  \return ostream object */
	friend std::ostream& operator>>(std::ostream& os, const Tickval& what)
	{
		std::string result;
		return os << GetTimeAsStringMS(result, &what, 9, true);
	}

private:
	/*! Convert timespec to ticks.
	  \param from timespec to convert
	  \return resulting ticks */
	static ticks _cvt(const timespec& from)
	{
		return billion * static_cast<ticks>(from.tv_sec) + static_cast<ticks>(from.tv_nsec);
	}
};

} // FIX8

#endif // _FIX8_TICKVAL_HPP_
