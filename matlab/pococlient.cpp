#include <iostream>
#include <sstream>
#include <fstream>
#include <random>
#include <thread>
#include <chrono>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/SocketAddress.h>
#include "rclient.h"

static void ms_delay(unsigned ms)
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
  std::thread th_rcv,th_snd;
  std::vector<double> last_values;
public:
  PocoUDPRobotClient(const char* robot_ip, int server_port)
  : ip(robot_ip)
  , port(server_port)
  , done(false)
  , send_drive(false)
  , last_values(5,0.0)
  , th_rcv([&](){main_loop();})
  , th_snd([&]() {send_loop(); })
  {}
  
  ~PocoUDPRobotClient()
  {
    done=true;
    th_snd.join();
    th_rcv.join();
  }

  void send_loop()
  {
    Poco::Net::SocketAddress robot_addr(ip, 2777);
    Poco::Net::DatagramSocket send_sock;
    send_sock.connect(robot_addr);
    while (!done)
    {
      if (send_drive)
      {
        send_sock.sendBytes(drive_cmd.c_str(), int(drive_cmd.length()));
        send_drive = false;
      }
      else ms_delay(5);
    }
  }
  
  void main_loop()
  {
  	//std::ofstream flog("c:\\work\\poco.log");
    Poco::Net::SocketAddress host_addr(Poco::Net::IPAddress(), port);
    Poco::Net::DatagramSocket recv_sock(host_addr,true);
    while (!done)
    {
      Poco::Net::SocketAddress sender;
      char buffer[256];
      int act=recv_sock.receiveFrom(buffer,255,sender);
      if (act>0)
      {
        buffer[act]=0;
        bool OLD_INTERFACE = false;
		    //flog << "Received " << act << " bytes: '" << buffer << "'" << std::endl;
		    if (OLD_INTERFACE)
		    {
          std::string from = sender.host().toString();
          if (from.substr(from.length() - 4) == ".198")
          {
            std::istringstream is(buffer);
            is >> last_values[0] >> last_values[1];
          }
          else
          {
            std::istringstream is(buffer);
            is >> last_values[2] >> last_values[3] >> last_values[4];
          }
		    }
        else
        {
          std::istringstream is(buffer);
          is >> last_values[0] >> last_values[1] >> last_values[2] >> last_values[3];
        }
      }
      ms_delay(5);
    }
  }
  
  virtual bool drive(int left, int right) override
  {
    if (!send_drive)
    {
      std::ostringstream os;
      os << left << ' ' << right;
      drive_cmd = os.str();
      send_drive = true;
    }
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

