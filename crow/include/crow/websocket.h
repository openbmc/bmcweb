#pragma once
#include <array>
#include "crow/http_request.h"
#include "crow/socket_adaptors.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/uuid/detail/sha1.hpp>

namespace crow {
namespace websocket {
enum class WebSocketReadState {
  MiniHeader,
  Len16,
  Len64,
  Mask,
  Payload,
};

struct Connection {
 public:
  explicit Connection(const crow::Request& req)
      : req(req), userdataPtr(nullptr){};

  virtual void sendBinary(const std::string& msg) = 0;
  virtual void sendText(const std::string& msg) = 0;
  virtual void close(const std::string& msg = "quit") = 0;
  virtual boost::asio::io_service& getIoService() = 0;
  virtual ~Connection() = default;

  void userdata(void* u) { userdataPtr = u; }
  void* userdata() { return userdataPtr; }

  crow::Request req;

 private:
  void* userdataPtr;
};

template <typename Adaptor>
class ConnectionImpl : public Connection {
 public:
  ConnectionImpl(
      const crow::Request& req, Adaptor&& adaptor,
      std::function<void(Connection&)> open_handler,
      std::function<void(Connection&, const std::string&, bool)>
          message_handler,
      std::function<void(Connection&, const std::string&)> close_handler,
      std::function<void(Connection&)> error_handler)
      : adaptor(std::move(adaptor)),
        Connection(req),
        openHandler(std::move(open_handler)),
        messageHandler(std::move(message_handler)),
        closeHandler(std::move(close_handler)),
        errorHandler(std::move(error_handler)) {
    if (!boost::iequals(req.getHeaderValue("upgrade"), "websocket")) {
      adaptor.close();
      delete this;
      return;
    }
    // Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
    // Sec-WebSocket-Version: 13
    std::string magic(req.getHeaderValue("Sec-WebSocket-Key"));
    magic += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    boost::uuids::detail::sha1 s;
    s.process_bytes(magic.data(), magic.size());

    // sha1 digests are 20 bytes long
    uint32_t digest[20 / sizeof(uint32_t)];
    s.get_digest(digest);
    for (int i = 0; i < 5; i++) {
      digest[i] = htonl(digest[i]);
    }
    start(crow::utility::base64encode(reinterpret_cast<char*>(digest), 20));
  }

  template <typename CompletionHandler>
  void dispatch(CompletionHandler handler) {
    adaptor.getIoService().dispatch(handler);
  }

  template <typename CompletionHandler>
  void post(CompletionHandler handler) {
    adaptor.getIoService().post(handler);
  }

  boost::asio::io_service& getIoService() override {
    return adaptor.getIoService();
  }

  void sendPong(const std::string& msg) {
    dispatch([this, msg] {
      char buf[3] = "\x8A\x00";
      buf[1] += msg.size();
      writeBuffers.emplace_back(buf, buf + 2);
      writeBuffers.emplace_back(msg);
      doWrite();
    });
  }

  void sendBinary(const std::string& msg) override {
    dispatch([this, msg] {
      auto header = buildHeader(2, msg.size());
      writeBuffers.emplace_back(std::move(header));
      writeBuffers.emplace_back(msg);
      doWrite();
    });
  }

  void sendText(const std::string& msg) override {
    dispatch([this, msg] {
      auto header = buildHeader(1, msg.size());
      writeBuffers.emplace_back(std::move(header));
      writeBuffers.emplace_back(msg);
      doWrite();
    });
  }

  void close(const std::string& msg) override {
    dispatch([this, msg] {
      hasSentClose = true;
      if (hasRecvClose && !isCloseHandlerCalled) {
        isCloseHandlerCalled = true;
        if (closeHandler) {
          closeHandler(*this, msg);
        }
      }
      auto header = buildHeader(0x8, msg.size());
      writeBuffers.emplace_back(std::move(header));
      writeBuffers.emplace_back(msg);
      doWrite();
    });
  }

 protected:
  std::string buildHeader(int opcode, uint64_t size) {
    char buf[2 + 8] = "\x80\x00";
    buf[0] += opcode;
    if (size < 126) {
      buf[1] += size;
      return {buf, buf + 2};
    } else if (size < 0x10000) {
      buf[1] += 126;
      *reinterpret_cast<uint16_t*>(buf + 2) =
          htons(static_cast<uint16_t>(size));
      return {buf, buf + 4};
    } else {
      buf[1] += 127;
      *reinterpret_cast<uint64_t*>(buf + 2) =
          ((1 == htonl(1))
               ? size
               : (static_cast<uint64_t>(htonl((size)&0xFFFFFFFF)) << 32) |
                     htonl((size) >> 32));
      return {buf, buf + 10};
    }
  }

  void start(std::string&& hello) {
    static std::string header =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        //"Sec-WebSocket-Protocol: binary\r\n"  // TODO(ed): this hardcodes
        // binary mode find a better way
        "Sec-WebSocket-Accept: ";
    static std::string crlf = "\r\n";
    writeBuffers.emplace_back(header);
    writeBuffers.emplace_back(std::move(hello));
    writeBuffers.emplace_back(crlf);
    writeBuffers.emplace_back(crlf);
    doWrite();
    if (openHandler) {
      openHandler(*this);
    }
    doRead();
  }

  void doRead() {
    isReading = true;
    switch (state) {
      case WebSocketReadState::MiniHeader: {
        // boost::asio::async_read(adaptor.socket(),
        // boost::asio::buffer(&miniHeader, 1),
        adaptor.socket().async_read_some(
            boost::asio::buffer(&miniHeader, 2),
            [this](const boost::system::error_code& ec,
                   std::size_t bytes_transferred) {
              isReading = false;
              miniHeader = htons(miniHeader);
#ifdef BMCWEB_ENABLE_DEBUG

              if (!ec && bytes_transferred != 2) {
                throw std::runtime_error(
                    "WebSocket:MiniHeader:async_read fail:asio bug?");
              }
#endif

              if (!ec && ((miniHeader & 0x80) == 0x80)) {
                if ((miniHeader & 0x7f) == 127) {
                  state = WebSocketReadState::Len64;
                } else if ((miniHeader & 0x7f) == 126) {
                  state = WebSocketReadState::Len16;
                } else {
                  remainingLength = miniHeader & 0x7f;
                  state = WebSocketReadState::Mask;
                }
                doRead();
              } else {
                closeConnection = true;
                adaptor.close();
                if (errorHandler) {
                  errorHandler(*this);
                }
                checkDestroy();
              }
            });
      } break;
      case WebSocketReadState::Len16: {
        remainingLength = 0;
        boost::asio::async_read(
            adaptor.socket(), boost::asio::buffer(&remainingLength, 2),
            [this](const boost::system::error_code& ec,
                   std::size_t bytes_transferred) {
              isReading = false;
              remainingLength = ntohs(*(uint16_t*)&remainingLength);
#ifdef BMCWEB_ENABLE_DEBUG
              if (!ec && bytes_transferred != 2) {
                throw std::runtime_error(
                    "WebSocket:Len16:async_read fail:asio bug?");
              }
#endif

              if (!ec) {
                state = WebSocketReadState::Mask;
                doRead();
              } else {
                closeConnection = true;
                adaptor.close();
                if (errorHandler) {
                  errorHandler(*this);
                }
                checkDestroy();
              }
            });
      } break;
      case WebSocketReadState::Len64: {
        boost::asio::async_read(
            adaptor.socket(), boost::asio::buffer(&remainingLength, 8),
            [this](const boost::system::error_code& ec,
                   std::size_t bytes_transferred) {
              isReading = false;
              remainingLength =
                  ((1 == ntohl(1))
                       ? (remainingLength)
                       : ((uint64_t)ntohl((remainingLength)&0xFFFFFFFF) << 32) |
                             ntohl((remainingLength) >> 32));
#ifdef BMCWEB_ENABLE_DEBUG
              if (!ec && bytes_transferred != 8) {
                throw std::runtime_error(
                    "WebSocket:Len16:async_read fail:asio bug?");
              }
#endif

              if (!ec) {
                state = WebSocketReadState::Mask;
                doRead();
              } else {
                closeConnection = true;
                adaptor.close();
                if (errorHandler) {
                  errorHandler(*this);
                }
                checkDestroy();
              }
            });
      } break;
      case WebSocketReadState::Mask:
        boost::asio::async_read(
            adaptor.socket(), boost::asio::buffer((char*)&mask, 4),
            [this](const boost::system::error_code& ec,
                   std::size_t bytes_transferred) {
              isReading = false;
#ifdef BMCWEB_ENABLE_DEBUG
              if (!ec && bytes_transferred != 4) {
                throw std::runtime_error(
                    "WebSocket:Mask:async_read fail:asio bug?");
              }
#endif

              if (!ec) {
                state = WebSocketReadState::Payload;
                doRead();
              } else {
                closeConnection = true;
                if (errorHandler) {
                  errorHandler(*this);
                }
                adaptor.close();
              }
            });
        break;
      case WebSocketReadState::Payload: {
        size_t toRead = buffer.size();
        if (remainingLength < toRead) {
          toRead = remainingLength;
        }
        adaptor.socket().async_read_some(
            boost::asio::buffer(buffer, toRead),
            [this](const boost::system::error_code& ec,
                   std::size_t bytes_transferred) {
              isReading = false;

              if (!ec) {
                fragment.insert(fragment.end(), buffer.begin(),
                                buffer.begin() + bytes_transferred);
                remainingLength -= bytes_transferred;
                if (remainingLength == 0) {
                  handleFragment();
                  state = WebSocketReadState::MiniHeader;
                  doRead();
                }
              } else {
                closeConnection = true;
                if (errorHandler) {
                  errorHandler(*this);
                }
                adaptor.close();
              }
            });
      } break;
    }
  }

  bool isFin() { return miniHeader & 0x8000; }

  int opcode() { return (miniHeader & 0x0f00) >> 8; }

  void handleFragment() {
    for (decltype(fragment.length()) i = 0; i < fragment.length(); i++) {
      fragment[i] ^= ((char*)&mask)[i % 4];
    }
    switch (opcode()) {
      case 0:  // Continuation
      {
        message += fragment;
        if (isFin()) {
          if (messageHandler) {
            messageHandler(*this, message, isBinary);
          }
          message.clear();
        }
      }
      case 1:  // Text
      {
        isBinary = false;
        message += fragment;
        if (isFin()) {
          if (messageHandler) {
            messageHandler(*this, message, isBinary);
          }
          message.clear();
        }
      } break;
      case 2:  // Binary
      {
        isBinary = true;
        message += fragment;
        if (isFin()) {
          if (messageHandler) {
            messageHandler(*this, message, isBinary);
          }
          message.clear();
        }
      } break;
      case 0x8:  // Close
      {
        hasRecvClose = true;
        if (!hasSentClose) {
          close(fragment);
        } else {
          adaptor.close();
          closeConnection = true;
          if (!isCloseHandlerCalled) {
            if (closeHandler) {
              closeHandler(*this, fragment);
            }
            isCloseHandlerCalled = true;
          }
          checkDestroy();
        }
      } break;
      case 0x9:  // Ping
      {
        sendPong(fragment);
      } break;
      case 0xA:  // Pong
      {
        pongReceived = true;
      } break;
    }

    fragment.clear();
  }

  void doWrite() {
    if (sendingBuffers.empty()) {
      sendingBuffers.swap(writeBuffers);
      std::vector<boost::asio::const_buffer> buffers;
      buffers.reserve(sendingBuffers.size());
      for (auto& s : sendingBuffers) {
        buffers.emplace_back(boost::asio::buffer(s));
      }
      boost::asio::async_write(adaptor.socket(), buffers,
                               [&](const boost::system::error_code& ec,
                                   std::size_t /*bytes_transferred*/) {
                                 sendingBuffers.clear();
                                 if (!ec && !closeConnection) {
                                   if (!writeBuffers.empty()) {
                                     doWrite();
                                   }
                                   if (hasSentClose) {
                                     closeConnection = true;
                                   }
                                 } else {
                                   closeConnection = true;
                                   checkDestroy();
                                 }
                               });
    }
  }

  void checkDestroy() {
    // if (hasSentClose && hasRecvClose)
    if (!isCloseHandlerCalled) {
      if (closeHandler) {
        closeHandler(*this, "uncleanly");
      }
    }
    if (sendingBuffers.empty() && !isReading) {
      delete this;
    }
  }

 private:
  Adaptor adaptor;

  std::vector<std::string> sendingBuffers;
  std::vector<std::string> writeBuffers;

  std::array<char, 4096> buffer{};
  bool isBinary{};
  std::string message;
  std::string fragment;
  WebSocketReadState state{WebSocketReadState::MiniHeader};
  uint64_t remainingLength{0};
  bool closeConnection{false};
  bool isReading{false};
  uint32_t mask{};
  uint16_t miniHeader{};
  bool hasSentClose{false};
  bool hasRecvClose{false};
  bool errorOccured{false};
  bool pongReceived{false};
  bool isCloseHandlerCalled{false};

  std::function<void(Connection&)> openHandler;
  std::function<void(Connection&, const std::string&, bool)> messageHandler;
  std::function<void(Connection&, const std::string&)> closeHandler;
  std::function<void(Connection&)> errorHandler;
};
}  // namespace websocket
}  // namespace crow
