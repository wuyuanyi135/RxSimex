import struct

import msgpack
import socket
import rx
from rx import interval
from rx import scheduler
import numpy as np
import argparse

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("action", choices=["info", "value", "subscribe"])

    args = parser.parse_args()

    if args.action == "info":
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(("127.0.0.1", 12305))
        s.send(msgpack.packb(["query_info"]))
        unpacker = msgpack.Unpacker()
        while True:
            b = s.recv(1024)
            unpacker.feed(b)
            for v in unpacker:
                print(v)
    elif args.action == "value":
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(("127.0.0.1", 12305))


        def packet_generator():
            cnt = 0
            while True:
                cnt += 1
                yield ["value", [0, struct.pack("d", cnt)]]
                mat = cnt * np.array([[1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12]], dtype=np.float64).T
                yield ["value", ["out2", bytes(mat.data)]]


        p = packet_generator()
        interval(1.0, scheduler.NewThreadScheduler()).subscribe(
            lambda x: s.send(msgpack.packb(next(p)))
        )

        unpacker = msgpack.Unpacker()
        while True:
            b = s.recv(1024)
            unpacker.feed(b)
            for v in unpacker:
                print(v)

    elif args.action == "subscribe":
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(("127.0.0.1", 12305))
        unpacker = msgpack.Unpacker()
        while True:
            b = s.recv(1024)
            unpacker.feed(b)
            for v in unpacker:
                if v[0] == b'port_values':
                    print("Receive port update")
                    ports = v[1]
                    for p in ports:
                        print(f"name: {p[b'name']}")
                        print(f"id: {p[b'id']}")
                        arr = np.frombuffer(p[b'data'], np.float64)
                        print(f"data: {arr}")
                        print("")

