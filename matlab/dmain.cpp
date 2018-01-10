#include <iostream>
#include <sstream>
#include <string>
#include <cstdint>
#include "mex.h"

#include "rclient.h"

template<class T, class CHK>
void get_data(const mxArray* prhs[], int index, T& t, CHK c, const char* type_name)
{
  if (!c(prhs[index]))
  {
    std::ostringstream os;
    os << "Parameter " << index << " should comply to " << type_name;
    throw os.str();
  }
  T* ptr = reinterpret_cast<T*>(mxGetData(prhs[index]));
  t = *ptr;
}

#define GET(prhs,index,t,c) get_data(prhs,index,t,c,#c)

client_ptr client;


void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	try
	{
		if (nrhs != 4)
			throw std::string("Invalid number of parameters\nUsage: redrobot(IP,CmdX,Left,Right)");
		if (nlhs != 7)
			mexErrMsgTxt("Invalid number of outputs\n[X,Y,DX,DY,S1,S2,S3]");

		if (!mxIsChar(prhs[0])) throw std::string("First parameter must be IP address as a string");
		double dcmd, dleft, dright;
		GET(prhs, 1, dcmd, mxIsDouble);
		GET(prhs, 2, dleft, mxIsDouble);
		GET(prhs, 3, dright, mxIsDouble);
		char* c_ip = mxArrayToString(prhs[0]);
		std::string ip(c_ip);
		mxFree(c_ip);
		int cmd = int(dcmd), left = int(dleft), right = int(dright);

		double x = -9999, y = -9999, dx=-9999, dy=-9999, s1=-1, s2=-1, s3=-1;
		switch (cmd)
		{
			case 0:
			{
				// Init client
				if (client) throw "Already initialized";
        mexPrintf("Connecting to '%s'\n", ip.c_str());
				client = udp_client(ip.c_str(),9209);
				break;
			}
			case 1:
			{
        if (!client) throw "Client not initialized";
        mexPrintf("Sending: %d %d\n", left, right);
				client->drive(left, right);
			}
			default:
			{
        if (!client) throw "Client not initialized";
        std::vector<double> vals;
				client->sense(vals);
				if (vals.size() >= 7)
				{
					x = vals[0];
					y = vals[1];
					dx = vals[2];
          dy = vals[3];
          s1 = vals[4];
          s2 = vals[5];
          s3 = vals[6];
        }
			}

		}

		plhs[0] = mxCreateDoubleScalar(x);
		plhs[1] = mxCreateDoubleScalar(y);
		plhs[2] = mxCreateDoubleScalar(dx);
    plhs[3] = mxCreateDoubleScalar(dy);
    plhs[4] = mxCreateDoubleScalar(s1);
    plhs[5] = mxCreateDoubleScalar(s2);
    plhs[6] = mxCreateDoubleScalar(s3);
  }
	catch (const std::string& s)
	{
		mexErrMsgTxt(s.c_str());
	}
	catch (const char* s)
	{
		mexErrMsgTxt(s);
	}
}

