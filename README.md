# mqtt交叉
编译mosquitto可能出现的错误，然后安装下面的包
libssl-dev xsltproc libxslt1-dbg  libc-ares-dev g++ uuid-dev docbook-xsl

websockets的安装
拷贝到编译mosquitto代理服务器主机上并且解压：
进入到下载的源码中：*****/libwebsockets-master
创建一个build文件夹
进入到build目录中
执行cmke，使用命令：cmake ..
然后make然后make install
mkdir build
cd build
cmake ..
make
make install
