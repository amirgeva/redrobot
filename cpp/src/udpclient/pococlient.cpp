#include <iostream>
#include <sstream>
#include <random>
#include <thread>
#include <chrono>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/SocketAddress.h>
#include <rclient.h>

static void Sleep(unsigned ms)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}



class PocoUDPRobotClient : public RClient
{
  std::string ip;
  int         port;
  bool        done;
  bool        send_drive;
  std::string drive_cmd;
  std::thread th;
  std::vector<double> last_values;
public:
  PocoUDPRobotClient(const char* robot_ip, int server_port)
  : ip(robot_ip)
  , port(server_port)
  , done(false)
  , send_drive(false)
  , last_values(7,0.0)
  , th([&](){main_loop();})
  {}
  
  ~PocoUDPRobotClient()
  {
    done=true;
    th.join();
  }
  
  void main_loop()
  {
    Poco::Net::SocketAddress robot_addr(ip, port);
    Poco::Net::SocketAddress host_addr(Poco::Net::IPAddress(), 9209);
    Poco::Net::DatagramSocket send_sock;
    send_sock.connect(robot_addr);
    Poco::Net::DatagramSocket recv_sock(host_addr,true);
    while (!done)
    {
      if (send_drive)
      {
        send_sock.sendBytes(drive_cmd.c_str(),drive_cmd.length());
        send_drive=false;
      }
      Poco::Net::SocketAddress sender;
      char buffer[256];
      int act=recv_sock.receiveFrom(buffer,255,sender);
      if (act>0)
      {
        buffer[act]=0;
        std::istringstream is(buffer);
        for(int i=0;i<7;++i)
          is >> last_values[i];
      }
      Sleep(5);
    }
  }
  
  virtual bool drive(int left, int right) override
  {
    std::ostringstream os;
    os << left << ' ' << right;
    drive_cmd=os.str();
    send_drive=true;
    return true;
  }
  
  virtual bool sense(std::vector<double>& values) override
  {
    values=last_values;
    return true;
  }
};

client_ptr udp_client(const char* robot_ip, const int port)
{
  return client_ptr(new PocoUDPRobotClient(robot_ip,port));
}


