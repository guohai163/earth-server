#coding: utf-8
import time
from gevent import socket
from locust import TaskSet, task, between, Locust, events
import binascii
import struct

import random
from random import uniform
import string
import math

class SocketClient(object):

    def __init__(self):
        # 仅在新建实例的时候创建socket.
        self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.__connected = False

    def __getattr__(self, name):
        # print(name)
        skt = self._socket

        def wrapper(*args, **kwargs):
            # print(name)
            start_time = time.time()
            if name == "connect":
                # print(args[0])
                try:
                    skt.connect(args[0])
                except Exception as e:
                    total_time = int((time.time() - start_time) * 1000)
                    events.request_failure.fire(request_type="connect", name=name, response_time=total_time, response_length=0, exception=e)
                else:
                    total_time = int((time.time() - start_time) * 1000)
                    events.request_success.fire(request_type="connect", name=name, response_time=total_time, response_length=0)
            elif name == "send":
                print(' '.join(hex(ord(i)) for i in args[0]))
                # skt.send(b'\x23\x23\x02\x00')
                # skt.send(binascii.hexlify(args[0]))
                skt.sendall(args[0])
                data = skt.recv(1024)
                # print(data)
            elif name == "close":
                skt.close()
        return wrapper

class SocketLocust(Locust):
    def __init__(self, *args, **kwargs):
        super(SocketLocust, self).__init__(*args, **kwargs)
        self.client = SocketClient()


def ranstr(num):
    salt = ''.join(random.sample(string.ascii_letters + string.digits, num))
    return salt


def generate_random_gps():
   return uniform(-180,180), uniform(-90, 90)

class UserBehavior(TaskSet):
    def on_start(self):
        self.client.connect((self.locust.host, self.locust.port))
    def on_stop(self):
        print("end conn")
        self.client.close()

    @task(1)
    def sendAddCmd(self):
        lat, log = generate_random_gps()
        dataBody = [
            'add ',
            ranstr(6),
            ' ',
            format(log,'f'),
            ' ',
            format(lat,'f'),
            '\x0d','\x0a']
        start_time = time.time()
        try:
            self.client.send("".join(dataBody))
        except Exception as e:
            total_time = int((time.time() - start_time) * 1000)
            events.request_failure.fire(request_type="earthtest", name="add", response_time=total_time, response_length=0, exception=e)
        else:
            total_time = int((time.time() - start_time) * 1000)
            events.request_success.fire(request_type="earthtest", name="add", response_time=total_time, response_length=0)
    @task(900)
    def sendGetCmd(self):

        lat, log = generate_random_gps()
        dataBody = [
            'get ',
            format(log,'f'),
            ' ',
            format(lat,'f'),
            ' 5',
            '\x0d','\x0a']
        start_time = time.time()
        try:
            self.client.send("".join(dataBody))
        except Exception as e:
            total_time = int((time.time() - start_time) * 1000)
            events.request_failure.fire(request_type="earthtest", name="get", response_time=total_time, response_length=0, exception=e)
        else:
            total_time = int((time.time() - start_time) * 1000)
            events.request_success.fire(request_type="earthtest", name="get", response_time=total_time, response_length=0)

        

class SocketUser(SocketLocust):
    
    # 目标地址
    host = "127.0.0.1"
    # 目标端口
    port = 32102
    task_set = UserBehavior
    wait_time = between(0.1, 1)
