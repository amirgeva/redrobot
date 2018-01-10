#include <iostream>
#include <thread>
#include <chrono>
#include <rclient.h>

void Sleep(unsigned ms)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

bool done=false;
int l=-9999,r=-9999;

void kbd()
{
  while (!done)
  {
    std::cin >> l;
    if (l>1000) { done=true; break; }
    std::cin >> r;
    if (r>1000) { done=true; break; }
  }
  l=r=-9999;
}

int main(int argc, char* argv[])
{
  client_ptr robot=udp_client("192.168.1.155",2777);
  std::vector<double> v;
  std::thread kbd_thread(kbd);
  while (!done)
  {
    if (l>=-9999 && r>=-9999)
    {
      robot->drive(l,r);
      l=r=-9999;
    }
    robot->sense(v);
    for(double d : v) std::cout << d << " ";
    std::cout << std::endl;
    Sleep(100);
  }
  kbd_thread.join();
  robot->drive(0,0);
  return 0;
}
