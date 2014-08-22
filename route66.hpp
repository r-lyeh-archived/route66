// lightweight http server
// - rlyeh, mit licensed. based on code by Ivan Tikhonov (zlib licensed)

/* to highlight API, remove this asterisk in a syntax highlighted editor --> */

#pragma once

#include <functional>
#include <vector>
#include <string>

namespace route66 {

    /*/ mime: types
    /*/
    struct mime {
        /*/ html(): mime type that is equal to "Content-Type: text/html; charset=utf-8\r\n"
        /*/ static std::string html();
        /*/ text(): mime type that is equal to "Content-Type: text/plain; charset=utf-8\r\n"
        /*/ static std::string text();
        /*/ json(): mime type that is equal to "Content-Type: application/json; charset=utf-8\r\n"
        /*/ static std::string json();
    };

    /*/ request: class
    /*/
    struct request {
        /*/ url: raw request ('/search?req1=blabla&req2=whatever')
        /*/ std::string url;
        /*/ path: raw path url ('/search')
        /*/ std::string path;
        /*/ options: raw options url ('?req1=blabla&req2=whatever')
        /*/ std::string options;
        /*/ arguments: decoupled options as arguments ([0]={'req1','blabla'}, [1]={'req2','whatever'})
        /*/ std::vector<std::pair<std::string, std::string>> arguments;
        /*/ answer(): to tell daemon to answer (optionally) a request /*/
        /*/ - httpcode, the HTML status code (200 OK, 404 NOT FOUND, etc)  /*/
        /*/ - headers, usually a mime type as defined in mime struct above /*/
        /*/ - content, answer content itself
        /*/ std::function<bool(unsigned httpcode, const std::string &headers, const std::string &content )> answer;
        /*/ shutdown(): to tell daemon to shutdown
        /*/ std::function<void()> shutdown;
    };

    /*/ create(): start a daemon at given port and route path, callback will be invoked; returns false if failed.
    /*/ bool create( unsigned port, const std::string &path, const std::function<void(request&)> &callback );
}
