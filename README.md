# 介绍

earth-server是一个基于谷歌S2类库实现的，地球经纬度算法数据库。目前支持如下功能：

1. 向数据库中增加、删除对象
2. 搜索指定位置最近的若干个坐标标识。
3. 以守护进程方式启动

# 编译

~~~
git clone https://github.com/guohai163/earth-server.git
cd earth-library
mkdir build
cd build
cmake ..
make
./earth-library
~~~

# 命令
* add 存储一个新坐标点

~~~ html
    add <key> <Latitude> <Longitude>\r\n
~~~

* get 搜索某一位置最近N个节点

~~~ html
    get <Latitude> <Longitude> <queryNumber>\r\n
~~~
    
* delete 删除一个坐标点

~~~ html
    delete <key> <Latitude> <Longitude>\r\n
~~~

* search 搜索某一位置周边指定秘书的结果集

~~~ html
    search <Latitude> <Longitude> <queryMeter>\r\n
~~~

# 参考
* [Beej's Guide to Network Programming](http://beej.us/guide/bgnet/)
* [Unix Network Programming](http://www.unpbook.com/)
* [S2Geometry](http://s2geometry.io/)
* [libevent](http://libevent.org/)
