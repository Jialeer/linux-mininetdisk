**可在Linux平台运行的一个小型多线程模拟网盘，能够实现多客户端上传，下载文件功能以及基本的客户端的命令行反馈，能够反馈如cd，ls，pwd，rm，mkdir，touch等命令。**

**网络服务器的I/O复用采用epoll实现，基于TCP协议的数据传输，服务器用来反馈客户端的命令行结果通过管道读取。**

编译：
在Mybash目录下分别编译server.cpp和client.cpp
g++ -o cli client.cpp
g++ -o ser server.cpp
测试：
分别在server/    client1/   client2/   目录下运行ser，cli
terminal1:
cd server/
../ser

terminal2:
cd client1/
../cli

terminal3:
cd client2/
../cli
