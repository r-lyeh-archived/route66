route66
=======

- Route66 is a lightweight embeddable HTTP server. Written in C++11.
- Route66 is tiny. One header and one source file.
- Route66 is cross-platform.
- Route66 is BOOST licensed.

### API
take a look into [route66.hpp header](route66.hpp)

### sample
```c++
#include <cassert>
#include <iostream>
#include "route66.hpp"

int main() {
    // create index route service
    assert(
        route66::create(8080, "/",
            []( route66::request &request, std::ostream &headers, std::ostream &contents ) {
                headers << route66::mime::html();
                contents << "<html><body><a href='/hello'>#</a></body></html>";
                return 200;
            } )
    );

    // create hello world route service
    assert(
        route66::create(8080, "/hello",
            []( route66::request &request, std::ostream &headers, std::ostream &contents ) {
                headers << route66::mime::text();
                contents << "hello world!";
                return 200;
            } )
    );

    // create echo route service
    assert(
        route66::create(8080, "/echo",
            []( route66::request &request, std::ostream &headers, std::ostream &contents ) {
                headers << route66::mime::text();
                contents << request.url;
                return 200;
            } )
    );

    // do whatever in your app. notice that you could change any route behavior in runtime.
    std::cout << "server ready at localhost:8080 (/, /hello, /echo)" << std::endl;
    for( char ch ; std::cin >> ch ; )
    {}
}
```
