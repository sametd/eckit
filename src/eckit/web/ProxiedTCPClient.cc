/*
 * (C) Copyright 1996-2017 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */


#include "eckit/config/Resource.h"
#include "eckit/web/ProxiedTCPClient.h"
#include "eckit/web/HttpHeader.h"

//-----------------------------------------------------------------------------

namespace eckit {

//-----------------------------------------------------------------------------

ProxiedTCPClient::ProxiedTCPClient(const std::string& proxyHost, int proxyPort, int port):
    TCPClient(port),
    proxyHost_(proxyHost),
    proxyPort_(proxyPort)
{
}

ProxiedTCPClient::~ProxiedTCPClient()
{
}

TCPSocket& ProxiedTCPClient::connect(const std::string& host, int port, int retries, int timeout) {
    TCPSocket& socket = TCPClient::connect(proxyHost_, proxyPort_, retries, timeout);

    socket.debug(debug_);

    const char * crlf = "\r\n";

    std::ostringstream oss;
    oss << "CONNECT " << host << ":" << port << " HTTP/1.0" << crlf;
    oss << "User-agent: eckit/1.0" << crlf;
    oss << crlf;

    std::string request(oss.str());
    ASSERT(socket.write(&request[0], request.size()) == request.size());


    // Strip http-header
    HttpHeader header(socket);
    std::cout << header << std::endl;
    header.checkForStatus();

    return socket;
}

void ProxiedTCPClient::print(std::ostream& s) const {
    s << "ProxiedTCPClient["
      << "proxyHost=" << proxyHost_
      << "proxyPort=" << proxyPort_
      << ",";
    TCPClient::print(s);
    s << "]";
}

//-----------------------------------------------------------------------------

} // namespace eckit

