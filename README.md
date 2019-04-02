# 编译

~~~
git clone https://github.com/guohai163/earth-library.git
cd earth-library
mkdir build
cd build
cmake ..
make
./earth-library
~~~

# 命令
* set

~~~ html
    set <key> 33.462 112.333\r\n
~~~
    
* get

~~~ html
    get <key>\r\n
~~~
* search

~~~ html
    search 33.462 112.333 1500\r\n
~~~
    
* delete

~~~ html
    delete <key>\r\n
~~~

# 参考
* [Beej's Guide to Network Programming](http://beej.us/guide/bgnet/)
* [Unix Network Programming](http://www.unpbook.com/)