#ifndef asiopp_H
#define asiopp_H

#include <memory>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/array.hpp>

template<class U>
inline std::string str(const U& u)
{
  std::ostringstream os; 
  os << u;
  return os.str();
}

class UDPSocket
{
  typedef boost::asio::ip::udp::socket socket_type;
  typedef boost::asio::ip::udp::endpoint endpoint_type;
  socket_type   m_Socket;
  endpoint_type m_EndPoint;
  friend class SocketManager;
public:
  UDPSocket(boost::asio::io_service& ios, const std::string& target_ip, int port)
  : m_Socket(ios,boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0))
  {
    m_Socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
    if (target_ip.substr(target_ip.length()-4) == ".255")
    {
      m_Socket.set_option(boost::asio::socket_base::broadcast(true));
      m_EndPoint=boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::broadcast(),port);
    }
    else
    {
      std::string target_port=str(port);
      boost::asio::ip::udp::resolver resolver(ios);
      boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), target_ip, target_port);
      boost::asio::ip::udp::resolver::iterator iter = resolver.resolve(query);
      m_EndPoint=*iter;
    }
  }
  
  UDPSocket(boost::asio::io_service& ios, int port)
  : m_Socket(ios,boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port))
  {
    m_Socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
//    if (broadcast)
//    {
//      m_EndPoint=boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::broadcast(),port);
//      m_Socket.set_option(boost::asio::socket_base::broadcast(true));
//    }
  }
  
  ~UDPSocket()
  {
    m_Socket.close();
  }
  
  void send(const std::string& msg)
  {
    m_Socket.send_to(boost::asio::buffer(msg, msg.size()), m_EndPoint);
  }
  
  std::string receive()
  {
    boost::array<char, 256> recv_buf;
    boost::asio::ip::udp::endpoint remote_endpoint;
    boost::system::error_code error;
    int n=m_Socket.receive_from(boost::asio::buffer(recv_buf),
                                remote_endpoint, 0, error);
    if (error && error != boost::asio::error::message_size)
      return "";
      //throw boost::system::system_error(error);
    recv_buf[n]=0;
    return std::string(&recv_buf[0]);
  }
};

class Socket
{
  typedef boost::asio::ip::tcp::socket socket_type;
  socket_type  m_Socket;
  friend class SocketManager;
public:
  Socket(boost::asio::io_service& ios) : m_Socket(ios) {}
  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;
  
  bool sendln(std::string s)
  {
    return send(s+"\n");
  }
  
  bool send(const std::string& s)
  {
    boost::asio::write(m_Socket, boost::asio::buffer(s.c_str(), s.length()));
    return true;
  }
  
  bool readln(std::string& s)
  {
    if (m_Socket.available()==0) return false;
    boost::asio::streambuf  response;
    boost::asio::read_until(m_Socket, response, "\n");
    std::istream is(&response);
    std::getline(is,s);
    return true;
  }
};

typedef std::shared_ptr<Socket> socket_ptr;
typedef std::shared_ptr<UDPSocket> udp_socket_ptr;

class SocketManager
{
public:
  static SocketManager* instance()
  {
    static std::unique_ptr<SocketManager> ptr(new SocketManager);
    return ptr.get();
  }
  
  udp_socket_ptr create_broadcast(int port)
  {
    return udp_socket_ptr(new UDPSocket(io_service,".255",port));
  }
  
  udp_socket_ptr create_send_udp_socket(const std::string& target_host, int port)
  {
    return udp_socket_ptr(new UDPSocket(io_service,target_host,port));
  }
  
  udp_socket_ptr create_recv_udp_socket(int port)
  {
    return udp_socket_ptr(new UDPSocket(io_service,port));
  }

  socket_ptr connect(const std::string& host, int port)
  {
    try
    {
      std::string port_s=str(port);
      boost::asio::ip::tcp::resolver resolver(io_service);
      boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), host, port_s);
      boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
      socket_ptr s(new Socket(io_service));
      boost::asio::connect(s->m_Socket, iterator);
      return s;
    } catch (const std::exception& e)
    {
    }
    return socket_ptr();
  }
  
  bool broadcast_message(int port, const std::string& msg)
  {
    auto it=m_BroadcastSockets.find(port);
    if (it==m_BroadcastSockets.end())
    {
      udp_socket_ptr p=create_broadcast(port);
      if (!p) return false;
      m_BroadcastSockets[port]=p;
      return broadcast_message(port,msg);
    }
    it->second->send(msg);
    return true;
  }
private:
  friend struct std::default_delete<SocketManager>;
  SocketManager()
  {}
  ~SocketManager() 
  {
  }
  SocketManager(const SocketManager& rhs) {}
  SocketManager& operator= (const SocketManager& rhs) { return *this; }
  
  boost::asio::io_service io_service;
  typedef std::unordered_map<int,udp_socket_ptr> smap;
  smap m_BroadcastSockets;
};

inline bool broadcast_message(int port, const std::string& msg)
{
  return SocketManager::instance()->broadcast_message(port,msg);
}



#endif // asiopp_H


