route66 <a href="https://travis-ci.org/r-lyeh/route66"><img src="https://api.travis-ci.org/r-lyeh/route66.svg?branch=master" align="right" /></a>
=======

- Route66 is a lightweight embeddable HTTP server. Written in C++11 (or C++03 w/boost).
- Route66 is handy. Support for [CRUD](http://en.wikipedia.org/wiki/Create,_read,_update_and_delete), [MIME](http://en.wikipedia.org/wiki/Internet_media_type) and wildcard routing.
- Route66 is tiny. One header and one source file.
- Route66 is cross-platform.
- Route66 is BOOST licensed.

### API
take a look into [route66.hpp header](route66.hpp)

### Debugging
- Tweak `R66LOG` environment variable to filter and display any incoming http request in runtime.
- Requests matching wildcard patterns will be printed. Ie, `*/*.html*`, `*POST /form?*`, `*192.168.*`, etc...

### sample
```c++
#include <cassert>
#include <iostream>
#include "route66.hpp"

int main() {
    // create index route service
    assert(
        route66::create(8080, "GET /",
            []( route66::request &request, std::ostream &headers, std::ostream &contents ) {
                headers << route66::mime(".html");
                contents << "<html><body><a href='/hello'>#</a></body></html>";
                return 200;
            } )
    );

    // create hello world route service
    assert(
        route66::create(8080, "GET /hello*",
            []( route66::request &request, std::ostream &headers, std::ostream &contents ) {
                headers << route66::mime(".text");
                contents << "hello world!";
                return 200;
            } )
    );

    // create echo route service
    assert(
        route66::create(8080, "GET /echo*",
            []( route66::request &request, std::ostream &headers, std::ostream &contents ) {
                headers << route66::mime(".text");
                contents << request.url;
                return 200;
            } )
    );

    // do whatever in your app. notice that you could change any route behavior in runtime.
    std::cout << "server ready at localhost:8080 GET(/, /hello*, /echo*)" << std::endl;
    for( char ch ; std::cin >> ch ; )
    {}
}
```
