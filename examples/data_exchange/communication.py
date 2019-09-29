import msgpack
import socket
import argparse
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("action", choices="info")

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
