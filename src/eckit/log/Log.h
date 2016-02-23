/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @file Log.h
/// @author Baudouin Raoult
/// @author Tiago Quintino
/// @date May 1996

#ifndef eckit_log_Log_h
#define eckit_log_Log_h

#include "eckit/log/CodeLocation.h"
#include "eckit/log/Channel.h"
#include "eckit/log/UserChannel.h"

namespace eckit {

//----------------------------------------------------------------------------------------------------------------------

class LogObserver {};

/// Singleton holding global streams for logging
///
/// @warning these streams are thread safe. A lock is
///          set when one of the stream is return, and is reset
///          on end of line. Don't forget to call endl.

class Log {

public:  // types
    /// Output formats
    enum {
        compactFormat = 0,
        normalFormat = 1,
        fullFormat = 2,
        monitorFormat = 3,
        applicationFormat = 4,  // Free to use for applications
    };

public:  // methods

    /// Register channels
    static void regstr(const std::string key, Channel* channel);
    static void remove(const std::string key);
    static Channel& channel(const std::string key);

    /// Channel for debug output
    static Channel& debug(int level = 1);
    static Channel& debug(const CodeLocation& where, int level = 1);

    /// Channel for informative messages
    static Channel& info();
    static Channel& info(const CodeLocation& where);

    /// Channel for warning messages
    static Channel& warning();
    static Channel& warning(const CodeLocation& where);

    /// Channel for error messages
    static Channel& error();
    static Channel& error(const CodeLocation& where);

    /// Channel for panic messages
    static std::ostream& panic();
    static std::ostream& panic(const CodeLocation& where);

    /// Channel accessible through category index
    static Channel& channel(int cat, int level = 1);
    static Channel& channel(int cat, const CodeLocation& where, int level = 1);

    /// characters to identify origin of monitoring messages
    enum { Unix = 'U', App = 'X' };

    /// Channel for application monitor
    static Channel& monitor(char, long);
    /// Channel for status messages to Application Monitor
    static Channel& status();
    /// Channel for status messages to Application Monitor
    static Channel& message();

    /// Get the channel for the user
    static UserChannel& user();
    /// Channel for informative messages tp remote user
    static Channel& userInfo();
    /// Channel for warning messages to remote user
    static Channel& userWarning();
    /// Channel for error messages to remote user
    static Channel& userError();
    /// Send messages to remote user directly -- not using a channel
    static void notifyClient(const std::string&);

    /// manipulator that will print the last error message as in perror(2)
    static std::ostream& syserr(std::ostream&);

    /// @todo find a way to refactor this ...
    // This is for Magics
    static void registerObserver(LogObserver*);
    static void unregisterObserver(LogObserver*);
    static std::ostream& dev() { return std::cout; }
    static void broadcast();
    static std::ostream& null();

    // Per application loggin
    // trace(const T* = 0) can be replace by trace<T>() with c++14
    template<typename T>
    static std::ostream& trace(int level = 0, const T* = 0) {
        if(T::trace(level)) {
            dev() << ">>> TRACE-" << T::name() << " ";
            return dev();
        }
        return null();
    }

    template<typename T>
    static bool tracing(int level = 0, const T* = 0) {
        return T::trace(level);
    }

private:
    Log();   ///< Private, non-instanciatable class
    ~Log();  ///< Private, non-instanciatable class
};

std::ostream& setformat(std::ostream&, int);
int format(std::ostream&);

#define ECKIT_DEBUG_HERE std::cerr << " DEBUG () @ " << Here() << std::endl;
#define ECKIT_DEBUG_VAR(x) std::cerr << " DEBUG (" << #x << ":" << x << ") @ " << Here() << std::endl;

//----------------------------------------------------------------------------------------------------------------------

} // namespace eckit

#endif
