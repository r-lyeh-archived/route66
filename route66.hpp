// lightweight http server
// - rlyeh, mit licensed. based on code by Ivan Tikhonov (zlib licensed)

/* to highlight API, remove this asterisk in a syntax highlighted editor --> */

#pragma once

#include <functional>
#include <vector>
#include <string>
#include <utility>
#include <ostream>

namespace route66 {

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
    };

    /*/ callback: alias
    /*/ using callback = std::function<int(request&,std::ostream&,std::ostream &)>;

    /*/ create(): start a daemon at given port and route path, callback will be invoked; returns false if failed.
    /*/ bool create( unsigned port, const std::string &path, const route66::callback &callback );

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

    /*/ httpcode: listing of HTTP result codes
    /*/
    struct httpcode {
        unsigned code;
        const char *const description;
    };

    /*/ 0XX route66
    /*/ httpcode SHUTDOWN();
    /*/ 1XX informational
    /*/ httpcode CONTINUE(); /*/
    /*/ httpcode SWITCH_PROTOCOLS();
    /*/ 2XX success
    /*/ httpcode OK(); /*/
    /*/ httpcode CREATED(); /*/
    /*/ httpcode ACCEPTED(); /*/
    /*/ httpcode PARTIAL(); /*/
    /*/ httpcode NO_CONTENT(); /*/
    /*/ httpcode RESET_CONTENT(); /*/
    /*/ httpcode PARTIAL_CONTENT(); /*/
    /*/ httpcode WEBDAV_MULTI_STATUS();
    /*/ 3XX redirection
    /*/ httpcode AMBIGUOUS(); /*/
    /*/ httpcode MOVED(); /*/
    /*/ httpcode REDIRECT(); /*/
    /*/ httpcode REDIRECT_METHOD(); /*/
    /*/ httpcode NOT_MODIFIED(); /*/
    /*/ httpcode USE_PROXY(); /*/
    /*/ httpcode REDIRECT_KEEP_VERB();
    /*/ 4XX client's fault
    /*/ httpcode BAD_REQUEST(); /*/
    /*/ httpcode DENIED(); /*/
    /*/ httpcode PAYMENT_REQ(); /*/
    /*/ httpcode FORBIDDEN(); /*/
    /*/ httpcode NOT_FOUND(); /*/
    /*/ httpcode BAD_METHOD(); /*/
    /*/ httpcode NONE_ACCEPTABLE(); /*/
    /*/ httpcode PROXY_AUTH_REQ(); /*/
    /*/ httpcode REQUEST_TIMEOUT(); /*/
    /*/ httpcode CONFLICT(); /*/
    /*/ httpcode GONE(); /*/
    /*/ httpcode LENGTH_REQUIRED(); /*/
    /*/ httpcode PRECOND_FAILED(); /*/
    /*/ httpcode REQUEST_TOO_LARGE(); /*/
    /*/ httpcode URI_TOO_LONG(); /*/
    /*/ httpcode UNSUPPORTED_MEDIA(); /*/
    /*/ httpcode RETRY_WITH();
    /*/ 5XX server's fault
    /*/ httpcode SERVER_ERROR(); /*/
    /*/ httpcode NOT_SUPPORTED(); /*/
    /*/ httpcode BAD_GATEWAY(); /*/
    /*/ httpcode SERVICE_UNAVAIL(); /*/
    /*/ httpcode GATEWAY_TIMEOUT(); /*/
    /*/ httpcode VERSION_NOT_SUP();
}
