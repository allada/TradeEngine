#ifndef Client_h
#define Client_h

#include "../Threading/SocketTasker.h"
#include "../Common.h"
#include <unordered_map>
#include <algorithm>
#include <endian.h>
#include <netinet/in.h>

namespace Net {

class Client : public Threading::SocketTasker {
public:
  Client(FileDescriptor sock, unsigned long ip)
    : Threading::SocketTasker(sock)
    , ip_(ip) { }

  ~Client() override
  {
      close();
  }

  void close() override;

  void run() override;

  unsigned long ip() const { return ip_; }

protected:
  struct Package {
  public:
    enum class Action : uint8_t {
      CLOSE  = 0,
      CLOSED = 1,
      PING   = 2,
      PONG   = 3,
      ACK    = 4,
      NACK   = 5,
    };

    Package() { }
    explicit Package(Action action) { data_.action = action; }

    Action action() const { return data_.action; }

    size_t setData(unsigned char* data, size_t size);
    void setAction(Action action) { data_.action = action; }
    void setSeqInc(uint32_t seqInc) { data_.seqInc = htole32(seqInc); }

    const unsigned char* data() const { return reinterpret_cast<const unsigned char*>(&data_); }
    size_t size() const;

  private:
    struct {
      uint8_t protocolVersion = 0;
      Action action;
      uint32_t seqInc;
    } data_;
  };

  void send(const Package& package);

  unsigned long ip_;
  bool closed_ = false;
};

} /* Net */

#endif /* Client_h */
