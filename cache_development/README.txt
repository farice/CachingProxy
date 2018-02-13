This contains an old caching simulator for ECE550

This is just for reference, we may implement that cache in
a different manner (probably, it needs different fields)


-- HTTP Caching Proxy Implementation (Cache) --

Items stored: HTTP OK responses to HTTP GET requests

Notes:

HTTP headers sent from a web server in the OK response to
a GET request contain a representation of the target resource
and the information needed to control the cache.
- content-type
- length in bytes (Content-length)
- caching directives (max-age field in seconds)
- validation token (used to check if the cached response
  is valid after it has expired)
  
