#include <iostream>
#include <random>
#include <thread>
#include <chrono>

#include <rclient.h>
#include <asiopp.h>

class BoostUDPRobotClient : public RClient
{
  std::string robot;
  int port;
  bool done;
  std::thread th;
  std::string drive_speeds;
  bool send_drive;
  std::vector<double> sensors;
  udp_socket_ptr drive_socket;
public:
  BoostUDPRobotClient(const char* robot_ip, int server_port)
  : robot(robot_ip)
  , port(server_port)
  , done(false)
  , send_drive(false)
  , th([&](){main_loop();})
  , sensors(7,0.0)
  , drive_socket(SocketManager::instance()->create_send_udp_socket(robot,port))
  {}

  ~BoostUDPRobotClient()
  {
    done=true;
    th.join();
  }
  
  void main_loop()
  {
    try
    {
      udp_socket_ptr recv_socket=SocketManager::instance()->create_recv_udp_socket(9209);
      while (!done)
      {
        std::string incoming=recv_socket->receive();
        if (incoming.size()>0)
        {
          std::vector<double> vals;
          double value;
          std::istringstream is(incoming);
          for(int i=0;i<7;++i)
            is >> sensors[i];
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
    } catch (const std::exception& e)
    {
      std::cout << e.what() << std::endl;
    }
  }
  
  virtual bool drive(int left, int right) override
  {
    std::ostringstream os;
    os << left << ' ' << right;
    drive_speeds=os.str();
    drive_socket->send(drive_speeds);
  }
  
  virtual bool sense(std::vector<double>& values) override
  {
    values=sensors;
    return true;
  }

};

client_ptr boost_udp_client(const char* robot_ip, const int port)
{
  return client_ptr(new BoostUDPRobotClient(robot_ip,port));
}

