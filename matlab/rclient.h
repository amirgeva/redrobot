#ifndef rclient_H
#define rclient_H

#include <memory>
#include <vector>

class RClient
{
public:
	virtual ~RClient() {}
	virtual bool drive(int left, int right) = 0;
	virtual bool sense(std::vector<double>& values) = 0;
};

typedef std::unique_ptr<RClient> client_ptr;

client_ptr connect_client(const char* server, const int port, const char* user_name, const char* robot_id);
client_ptr udp_client(const char* robot_ip, const int port = 2777);

#endif // rclient_H

