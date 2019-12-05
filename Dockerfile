FROM debian
MAINTAINER H!Guo

RUN apt-get update -y && \
    apt-get install -y cmake libgflags-dev libgoogle-glog-dev libgtest-dev libevent-dev git libssl-dev build-essential
    
RUN cd /opt && git clone https://github.com/google/s2geometry.git && cd s2geometry && \
    mkdir build && cd build && \
    cmake -DGTEST_ROOT=/usr/src/gtest .. && \
    make && make install
    
COPY tools/sysctl.conf /etc/sysctl.conf 
COPY tools/limits.conf /etc/security/limits.conf 

ADD . /opt/earth-server/

EXPOSE 40000

RUN mkdir -p /opt/earth-server/build && \
    cd /opt/earth-server/build && cmake .. && \
    make && ls -l && ./earth_server