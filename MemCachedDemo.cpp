#define WIN32_LEAN_AND_MEAN

#pragma comment(lib,"ws2_32.lib")
#ifdef _DEBUG
    #pragma comment(lib,"MemCacheClient/lib/MemCacheClient-mt-d-x86.lib")
#else
    #pragma comment(lib,"MemCacheClient/lib/MemCacheClient-mt-x86.lib")
#endif // _DEBUG

#include <Windows.h>
#include <WinSock2.h>
#include <iostream>
#include <synchapi.h>

#include "MemCacheClient/include/MemCacheClient.h"

constexpr const char *DEFAULT_IP = "127.0.0.1";
constexpr const unsigned short DEFAULT_PORT = 11211;

int main(int argc, char * argv[]) {
    // 1.指定使用2.2版本WinSock
    WSADATA wsaData;
    int nRes = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (nRes != 0) {
        std::cerr << "WSAStartup error:"<< std::endl;
        return -1;
    }

    const char *pIp = DEFAULT_IP;
    unsigned short u16Port = DEFAULT_PORT;

    // 不输入则默认
    if (argc > 1) {
        pIp = argv[1];
    }
    if (argc > 2) {
        u16Port = std::atoi(argv[2]);
    }

    char pServerAddr[48] ={ 0 };
    sprintf_s(
        pServerAddr
        , "%s:%hu"
        , pIp
        , u16Port);

    CMemCacheClient client;
    client.SetRetryPeriod(100);
    try {
        if (client.AddServer(pServerAddr)) {
            std::cout << "add server success, addr:" << pServerAddr << std::endl;
        } else {
            std::cout << "add server faild, addr:" << pServerAddr << std::endl;
        }

        // 内部socket连接是惰性的 测试一下
        std::cout << "enter any key to req" << std::endl;
        int nChar = getchar();

        CMemCacheClient::MemRequest req;
        req.strKey = "datakey";
        req.data.WriteBytes("foobar", 5);
        client.Add(req);
        std::cout << "req ec:" << req.eResult << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "exception msg:" << e.what() << std::endl;
    }

    Sleep(1000 * 600);
}
