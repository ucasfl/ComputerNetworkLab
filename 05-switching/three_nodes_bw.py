#!/usr/bin/python

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.link import TCLink
from mininet.cli import CLI

# Mininet will assign an IP address for each interface of a node
# automatically, but hub or switch does not need IP address.
def clearIP(n):
    for iface in n.intfList():
        n.cmd('ifconfig %s 0.0.0.0' % (iface))

class SwitchingTopo(Topo):
    def build(self):
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')
        h3 = self.addHost('h3')
        s1 = self.addHost('s1')

        self.addLink(h1, s1, bw=20)
        self.addLink(h2, s1, bw=10)
        self.addLink(h3, s1, bw=10)

if __name__ == '__main__':
    topo = SwitchingTopo()
    net = Mininet(topo = topo, link = TCLink, controller = None)

    h1, h2, h3, s1 = net.get('h1', 'h2', 'h3', 's1')
    h1.cmd('ifconfig h1-eth0 10.0.0.1/24')
    h2.cmd('ifconfig h2-eth0 10.0.0.2/24')
    h3.cmd('ifconfig h3-eth0 10.0.0.3/24')
    clearIP(s1)

    h1.cmd('./disable_offloading.sh')
    h2.cmd('./disable_offloading.sh')
    h3.cmd('./disable_offloading.sh')

    net.start()
    CLI(net)
    net.stop()
