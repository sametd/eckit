/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "eckit/log/FileTarget.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/config/Resource.h"

namespace eckit {

//----------------------------------------------------------------------------------------------------------------------

FileTarget::FileTarget(const PathName& path) : buffer_(4 * 1024), path_(path) {

    size_t logFileBufferSize = eckit::Resource<size_t>("logFileBufferSize", buffer_.size());  // deactivate with 0

    if (logFileBufferSize) {
        buffer_.resize(logFileBufferSize);
        buffer_.zero();
        out_.rdbuf()->pubsetbuf(buffer_, logFileBufferSize);
    }

    out_.open(path_.asString().c_str(), std::ofstream::out | std::ofstream::app);
    if (!out_) {
        throw eckit::CantOpenFile(path_.asString());
    }
}

FileTarget::~FileTarget() {
    // std::cerr << "FileTarget::~FileTarget() -- " << path_ << std::endl;
    // out_.close();
    // if (out_) {
    //     throw eckit::CloseError(path_.asString(), Here());
    // }
}

void FileTarget::write(const char* start, const char* end) {
    out_.write(start, end - start);
}

void FileTarget::flush() {
    out_.flush();
}

void FileTarget::print(std::ostream& s) const {
    s << "FileTarget(path=" << path_ << ")";
}

//----------------------------------------------------------------------------------------------------------------------

}  // namespace eckit
