#!/usr/bin/env python2

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.cli import CLI

class MOSPFTopo(Topo):
    def build(self):
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')
        r1 = self.addHost('r1')
        r2 = self.addHost('r2')
        r3 = self.addHost('r3')
        r4 = self.addHost('r4')

        self.addLink(h1, r1)
        self.addLink(r1, r2)
        self.addLink(r1, r3)
        self.addLink(r2, r4)
        self.addLink(r3, r4)
        self.addLink(r4, h2)

if __name__ == '__main__':
    topo = MOSPFTopo()
    net = Mininet(topo = topo, controller = None)

    h1, h2, r1, r2, r3, r4 = net.get('h1', 'h2', 'r1', 'r2', 'r3', 'r4')
    h1.cmd('ifconfig h1-eth0 10.0.1.11/24')

    r1.cmd('ifconfig r1-eth0 10.0.1.1/24')
    r1.cmd('ifconfig r1-eth1 10.0.2.1/24')
    r1.cmd('ifconfig r1-eth2 10.0.3.1/24')

    r2.cmd('ifconfig r2-eth0 10.0.2.2/24')
    r2.cmd('ifconfig r2-eth1 10.0.4.2/24')

    r3.cmd('ifconfig r3-eth0 10.0.3.3/24')
    r3.cmd('ifconfig r3-eth1 10.0.5.3/24')

    r4.cmd('ifconfig r4-eth0 10.0.4.4/24')
    r4.cmd('ifconfig r4-eth1 10.0.5.4/24')
    r4.cmd('ifconfig r4-eth2 10.0.6.4/24')

    h2.cmd('ifconfig h2-eth0 10.0.6.22/24')

    h1.cmd('route add default gw 10.0.1.1')
    h2.cmd('route add default gw 10.0.6.4')

    for h in (h1, h2):
        h.cmd('./scripts/disable_offloading.sh')
        h.cmd('./scripts/disable_ipv6.sh')

    for r in (r1, r2, r3, r4):
        r.cmd('./scripts/disable_arp.sh')
        r.cmd('./scripts/disable_icmp.sh')
        r.cmd('./scripts/disable_ip_forward.sh')

    # r1.cmd('./mospfd > r1-output.txt &')
    # r2.cmd('./mospfd > r2-output.txt &')
    # r3.cmd('./mospfd > r3-output.txt &')
    # r4.cmd('./mospfd > r4-output.txt &')

    net.start()
    CLI(net)
    net.stop()
