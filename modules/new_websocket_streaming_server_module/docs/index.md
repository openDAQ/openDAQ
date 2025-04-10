# High-Performance WebSocket Streaming Server Module

## Introduction

The High-Performance WebSocket Streaming Server Module is an [openDAQ][openDAQ] module that
streams signal data to clients using the [WebSocket Streaming protocol][streaming-protocol]. This
module performs the same function as the `websocket_streaming_server_module`, but has been
reimplemented in order to achieve higher performance and reliability on resource-limited
Linux-based embedded platforms. This reimplementation has the following design characteristics:

- Limited use of abstraction layers: Boost [Asio][boost-asio] is used at the lowest level for
  nearly-transparent access to underlying socket and event loop functions. On embedded Linux
  platforms this results in almost-direct use of the [Berkeley sockets API][berkeley-sockets]
  API and of [epoll][epoll]. No other abstraction layers exist in the main I/O path. As well as
  improving performance, this leads to simpler code that is easier to statically analyze for
  maximum reliability.

- Synchronous & non-blocking programming model: streamed data is fed directly into the TCP send
  buffer, synchronously, on whatever openDAQ thread sends output signal packets. The TCP send
  buffer size is managed to ensure sufficient ability to tolerate short-duration network issues on
  the order of seconds without blocking the calling openDAQ thread. Clients that are unable to
  read data fast enough are disconnected, and will never block any other client or the calling
  openDAQ thread. Incoming connection acceptance and negotiation is handled in a single dedicated
  worker thread.

- Minimal use of locking: While locks are used in some places to make certain objects and member
  functions thread-safe, there is no cross-object locking. Instead, lock-free standard library
  [`shared_ptr`][shared-ptr] / [`weak_ptr`][weak-ptr] objects are carefully used to manage object
  lifecycles, in particular of client objects. The intent is to minimize the locks that need to be
  taken in the performance-critical signal transmission code path. The locks that do exist are
  designed such that the performance-critical thread "almost never" has to wait for the lock:
  contention is a corner case.

## Class Diagram

![class-diagram]

## Operational Overview

1. The @ref daq::modules::ws_streaming_server::WsStreamingServerModuleImpl
   "WsStreamingServerModuleImpl" class implements an [openDAQ][openDAQ] module that declares a
   single server type, implemented by the @ref
   daq::modules::ws_streaming_server::WsStreamingServerImpl "WsStreamingServerImpl" class.

2. The @ref daq::modules::ws_streaming_server::WsStreamingServerImpl "WsStreamingServerImpl" class
   instantiates and owns a @ref daq::ws_streaming::server object. This object is configured
   according to the openDAQ properties attached to the server (such as TCP port numbers).

3. The @ref daq::ws_streaming::server object enumerates the available public openDAQ signals, and
   creates and attaches a @ref daq::ws_streaming::WebSocketSignalListenerImpl
   "WebSocketSignalListenerImpl" object to each of them.

4. The @ref daq::ws_streaming::server object creates a background thread to handle client
   management. The first job of the management thread is to listen for incoming TCP connections
   and to create @ref daq::ws_streaming::websocket_client_negotiating
   "websocket_client_negotiating" objects to handle them. These objects are stored in the
   server's client list. The server is the primary owner of a client object (although ownership is
   temporarily shared via weak references with an active listener object; see below).

5. When the @ref daq::ws_streaming::websocket_client_negotiating "websocket_client_negotiating"
   has completed WebSocket upgrade negotiation, the client is replaced by a 
   @ref daq::ws_streaming::websocket_client_established "websocket_client_established" object.
   Control client requests are managed directly by the @ref
   daq::ws_streaming::websocket_client_negotiating "websocket_client_negotiating" class.

6. When a client subscribes to a signal, a weak reference to that client's @ref
   daq::ws_streaming::websocket_client_established "websocket_client_established" object is added
   to the corresponding @ref daq::ws_streaming::WebSocketSignalListenerImpl
   "WebSocketSignalListenerImpl" object's client list. The signal description (metadata) and
   possibly initial values are transmitted to the client.

7. When a @ref daq::ws_streaming::WebSocketSignalListenerImpl "WebSocketSignalListenerImpl"
   receives a packet, it transmits the data and/or metadata to all clients in its list. Data
   transmission for various signal types is handled by classes derived from @ref
   daq::ws_streaming::signal_writer "signal_writer".

8. Disconnected clients may, depending on circumstances, either be "noticed" by the @ref
   daq::ws_streaming::server object or by a @ref daq::ws_streaming::WebSocketSignalListenerImpl
   "WebSocketSignalListenerImpl" object. In either case, [shutdown()][shutdown] is called on the
   socket, ensuring that the daq::ws_streaming::server - which is the primary owner of the client
   object - "notices" the disconnected client by virtue of [read()][read] returning zero. The
   client is then removed from the server's client list and (once all temporarily locked weak
   references in listener objects have been released) destroyed.

9. When the server is stopped, the listeners are detached from the corresponding signals, and the
   worker thread is gracefully stopped and joined.

## Known Limitations

- This implementation only supports a subset of openDAQ signal data types and descriptors.
  However, the framework is in place to add additional support for other data types and
  descriptors, and this can be easily extended as needed.

- This implementation is closely coupled with openDAQ. In the future it should be possible to
  encapsulate the link between openDAQ and the server implementation so that the module can also
  be used outside of openDAQ.

- Only a limited set of properties are configurable: specifically, the TCP port numbers.
  Additional configuration properties such as the TCP send buffer size would also be useful.

- Dynamic signals are not supported: only the set of publicly-exposed signals that exist at the
  moment of server creation can be streamed. Additional signals created later will not be
  recognized.

## Known Bugs

- Exposing signals using data types or descriptor features that are not supported by the library
  may result in termination without a useful error message. See "Known Limitations" above. The
  code could more gracefully handle this scenario.

No other bugs are currently known.

[berkeley-sockets]: https://en.wikipedia.org/wiki/Berkeley_sockets
[boost-asio]: https://www.boost.org/doc/libs/1_86_0/doc/html/boost_asio.html
[boost-beast]: https://www.boost.org/doc/libs/1_86_0/libs/beast/doc/html/index.html
[class-diagram]: class-diagram.png "Class diagram"
[epoll]: https://man7.org/linux/man-pages/man7/epoll.7.html
[openDAQ]: https://opendaq.com/
[read]: https://www.man7.org/linux/man-pages/man2/read.2.html
[shared-ptr]: https://en.cppreference.com/w/cpp/memory/shared_ptr
[shutdown]: https://www.man7.org/linux/man-pages/man2/shutdown.2.html
[streaming-protocol]: https://github.com/openDAQ/streaming-protocol-lt/blob/main/docs/openDAQ-streaming.md
[weak-ptr]: https://en.cppreference.com/w/cpp/memory/weak_ptr
