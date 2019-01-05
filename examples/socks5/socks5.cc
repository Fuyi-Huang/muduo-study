#include "tunnel.h"

#include <malloc.h>
#include <stdio.h>
#include <sys/resource.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

EventLoop* g_eventLoop;
InetAddress* g_serverAddr;
std::map<string, TunnelPtr> g_tunnels;

std::map<string, const char*> host_proxy;

void onServerConnection(const TcpConnectionPtr& conn)
{
  LOG_WARN << (conn->connected() ? "UP" : "DOWN");
  if (conn->connected())
  {
    conn->setTcpNoDelay(true);
  }
  else
  {
    if(g_tunnels.find(conn->name()) != g_tunnels.end())
    {
        g_tunnels[conn->name()]->disconnect();
        g_tunnels.erase(conn->name());
    }
  }
}

string getHost(Buffer* buf)
{
    const char* crlf = buf->findCRLF();
    string nullstr = "";
    if (!crlf)
    {
        return nullstr;
    }
    const char* crlf1 = buf->findCRLF(crlf + 2);
    if (!crlf1)
    {
        return nullstr;
    }
    const char* colon = std::find(crlf + 2, crlf1, ':');
    if (colon == crlf1)
    {
        return nullstr;
    }
    return string(colon + 2, crlf1);
}

void onServerMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp)
{
    LOG_WARN << buf->readableBytes();
    
    string host = getHost(buf);
    LOG_INFO << "host:" << host << ", ends";  
    if (host == "")
    {
        LOG_ERROR << " ++++++++++++++++++++++ host empty";
        conn->send("HTTP/1.1 403 Forbidden\r\n");
        return;
    }
    if (host_proxy.find(host) == host_proxy.end())
    {
        LOG_ERROR << " ++++++++++++++++++++++ host not exists";
        host = "default";
    }
    else
    {
        LOG_ERROR << " ++++++++++++++++++++++ host exists";
    }
    LOG_ERROR << " ++++++++++++++++++++++ length: " << sizeof(host_proxy[host]) << ", end";
    LOG_ERROR << " ++++++++++++++++++++++ char at 9: " << (host_proxy[host][9]) << ", end";
    TunnelPtr tunnel(new Tunnel(g_eventLoop, *g_serverAddr, conn));
    tunnel->setFormat(host_proxy[host], 10);
    tunnel->setup();
    tunnel->connect();
    g_tunnels[conn->name()] = tunnel;
}

void init()
{
    // 下面的是用的线上机器，因为是校验的，不影响业务
    host_proxy["resource.service.kgidc.cn"] = "\x05\x01\x00\x01\n\x01\x10\xb6\x00P"; // 10.1.16.182
    host_proxy["token-internal.user.kgidc.cn"] = "\x05\x01\x00\x01\n\x10\x02=\x00P"; // 10.16.2.61
    // 下面的是将域名映射到测试机ip
    host_proxy["media.store.kgidc.cn"] = "\x05\x01\x00\x01\n\x01\x02\xbd\x00P"; // 10.1.2.189
    host_proxy["goodsmstore.kgidc.cn"] = "\x05\x01\x00\x01\n\x10\x04\xb7\x00P"; // 10.16.4.183
    host_proxy["zhuanji-service.kgidc.cn"] = "\x05\x01\x00\x01\n\x10\x04\x13\x00P"; // 10.16.4.19
    host_proxy["media.store.kgidc.cn"] = "\x05\x01\x00\x01\n\x01\x02\xbd\x00P"; // 10.1.2.189
    host_proxy["msg.mobile.kgidc.cn"] = "\x05\x01\x00\x01z\rD\xfe\x00P"; // 122.13.68.254
    host_proxy["pay-service_kgidc_cn"] = "\x05\x01\x00\x01\n\x01Q\x97\x00P"; // 10.1.81.151
    host_proxy["kupay.kugou.com"] = "\x05\x01\x00\x01\n\x01Q\x97\x00P"; // 10.1.81.151
    // 下面的是映射到自己本机的服务
    host_proxy["gz-wallet.service.kgidc.cn"] = "\x05\x01\x00\x01\x7f\x00\x00\x01\x00P"; // 127.0.0.1
    host_proxy["bj-wallet.service.kgidc.cn"] = "\x05\x01\x00\x01\x7f\x00\x00\x01\x00P"; // 127.0.0.1
    host_proxy["wallet-service.kgidc.cn"] = "\x05\x01\x00\x01\x7f\x00\x00\x01\x00P"; // 127.0.0.1

    host_proxy["default"] = "\x05\x01\x00\x01\x7f\x00\x00\x01\x00P"; // 127.0.0.1
}

int main(int argc, char* argv[])
{
  if (argc < 4)
  {
    fprintf(stderr, "Usage: %s <host_ip> <port> <listen_port>\n", argv[0]);
  }
  else
  {
    LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
    {
      // set max virtual memory to 256MB.
      size_t kOneMB = 1024*1024;
      rlimit rl = { 256*kOneMB, 256*kOneMB };
      setrlimit(RLIMIT_AS, &rl);
    }

    init();

    const char* ip = argv[1];
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    InetAddress serverAddr(ip, port);
    g_serverAddr = &serverAddr;

    uint16_t acceptPort = static_cast<uint16_t>(atoi(argv[3]));
    InetAddress listenAddr(acceptPort);

    EventLoop loop;
    g_eventLoop = &loop;

    TcpServer server(&loop, listenAddr, "TcpRelay");

    server.setConnectionCallback(onServerConnection);
    server.setMessageCallback(onServerMessage);

    server.start();

    loop.loop();
  }
}
