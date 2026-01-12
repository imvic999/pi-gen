import sys
import datetime
import socket
import re
import threading

# ------------------------------------------------------------------
# This script acts as a proxy between a LoRa gateway and network
# server which communicate using the Semtech UDP protocol. It will
# intercept data comming from the gateway and apply simple
# replacements, then intercept the data coming back from the network
# server and do a similar set of replacements. This makes it appear
# as though the network server is 'seeing' a specific channel, even
# though it's actually received by the gateway on some other channel
# ------------------------------------------------------------------

def print2(*args):
    print(datetime.datetime.now().isoformat(), *args)

# The NS receives data as if from a 868.8MHz FSK channel
us_replacements:dict[bytes,bytes] = {
    b'"freq":[0-9.]+'   : b'"freq":915.7',
    b'"datr":[0-9]+'    : b'"datr":50000'
}
# The gateway receives data as if on a 40kbps 920MHz FSK channel with 40KHz fdev
ds_replacements:dict[bytes,bytes] = {
    b'"freq":[0-9.]+'   : b'"freq":918.2',
    b'"datr":[0-9]+'    : b'"datr":40000',
    b'"fdev":[0-9]+'    : b'"fdev":40000'
}
def replace(data:bytes, replacements:dict[bytes,bytes]):
    for k,v in replacements.items():
        data = re.sub(k, v, data)
    return data

class udp_connection:
    def __init__(self, nb_sock:socket.socket, sb_sock:socket.socket, gw_addr:tuple[str,int]):
        self.nb_sock = nb_sock
        self.sb_sock = sb_sock
        self.gw_addr = gw_addr

    def run(self):
        while True:
            data,_ = self.nb_sock.recvfrom(1024)
            data = replace(data, ds_replacements)
            self.sb_sock.sendto(data, self.gw_addr)
            print2("Sent DS data to", self.gw_addr, data)

gateways:dict[str,] = {}

def main(listen_port:int, nb_host:str, nb_port:int):
    udp_server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_server.bind(("0.0.0.0", listen_port))
    print2("Listening on port", listen_port)

    nb_addr = (nb_host, nb_port)

    while True:
        data,addr = udp_server.recvfrom(1024)
        cx = gateways.get(addr)
        if(cx == None):
            print2("New connection from", addr)
            nb_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            nb_sock.bind(("0.0.0.0", 0))
            cx = udp_connection(nb_sock, udp_server, addr)
            gateways[addr] = cx
            threading.Thread(target=cx.run).start()
        data = replace(data, us_replacements)
        cx.nb_sock.sendto(data, nb_addr)
        print2("Sent US data to", nb_addr, data)



if __name__ == '__main__':
    if(len(sys.argv) != 4):
        print2("Expected 3 arguments (Listen Port) (Northbound Host) (Northbound Port)")
        exit(1)

    main(int(sys.argv[1]), sys.argv[2], int(sys.argv[3]))
