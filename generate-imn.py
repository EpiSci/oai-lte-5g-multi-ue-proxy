#!/usr/bin/env python3
"""
Generate .imn file for a multi-UE scenario

  Example: ./generate-scenario.py 5 > Core_PROXY_5_UEs.imn
"""

import sys

NODETYPE = '''\
node {n1} {
    type {router}'''

NODEMODEL = '''\
    model mdr'''

NETWORK_CONFIG = '''\
    network-config {
	hostname {eNB}
	!
	interface {eth0}
	 ip address 10.0.0.{ip}/32
	!
    }'''

CANVAS = '''\
    canvas c1
    iconcoords {{posnode}}
    labelcoords {{poslabel}}'''


CANVAS_OPTIONS = '''\
canvas c1 {
    name {Canvas1}
}

option global {
    interface_names no
    ip_addresses yes
    ipv6_addresses no
    node_labels yes
    link_labels yes
    show_api no
    background_images no
    annotations yes
    grid yes
    traffic_start 0
}

option session {
}
'''

def replace_and_print_configinfo(prototype, rtable):
    """Replace prototype lines with rtable information string, then print lines."""
    lines = prototype.split('\n')
    for line in lines:
        for key, value in rtable.items():
            line = line.replace(key, value)
        print(line)

def print_lines(prototype):
    """Print prototype lines without any change."""
    lines = prototype.split('\n')
    for line in lines:
        print(line)

def peerinfo(nodename, wlandname, wlanid):
    """Construct peer information for interfaces."""
    if nodename != wlandname:
        peer = '{eth0' + ' ' + wlandname + '}'
        infpeerinfo = '    interface-peer {}'.format(peer)
    else:
        peerlist = []
        for i in range(wlanid):
            peer = '{e%s n%s}' % (i, i + 1)
            peerlist += ['    interface-peer {}'.format(peer)]
        infpeerinfo = '\n'.join(peerlist)
    return infpeerinfo

def add_nodeinfo(args):
    """Construct replace table and add node information."""
    nodename, hostname, nid, posnode, poslabel, wlandname, ues, wlanid = args
    ntype = 'router' if nodename != wlandname else 'wlan'
    inf = 'eth0' if nodename != wlandname else 'wireless'
    ipinfo = nid if nodename != wlandname else '0'
    infpeerinfo = peerinfo(nodename, wlandname, wlanid)

    rtable = {
        '{n1}': nodename,
        '{router}': ntype,
        '{eNB}': hostname,
        '{eth0}': inf,
        '{ip}': ipinfo,
        '{posnode}': posnode,
        '{poslabel}': poslabel,
        '{len_ues}': ues,
        '{infpeer}': infpeerinfo
    }
    if nodename != wlandname:
        replace_and_print_configinfo(NODETYPE, rtable)
        print_lines(NODEMODEL)
        replace_and_print_configinfo(NETWORK_CONFIG, rtable)
        replace_and_print_configinfo(CANVAS, rtable)
        print_lines(infpeerinfo)
        print_lines('}\n')
    else:
        replace_and_print_configinfo(NODETYPE, rtable)
        replace_and_print_configinfo(NETWORK_CONFIG, rtable)
        replace_and_print_configinfo(CANVAS, rtable)
        print_lines(infpeerinfo)
        print_lines('}\n')

def add_linkinfo_and_options(wlandname, len_nodes):
    """Add link information and further options."""
    for i in range(1, len_nodes):
        link = 'link l{}'.format(i) + ' {'
        nodes = '    nodes {' + wlandname + ' ' + 'n' + str(i) + '}'
        linknode = [link, nodes, '}']
        linklist = '\n'.join(linknode) + '\n'
        print_lines(linklist)
    print_lines(CANVAS_OPTIONS)

def get_pos(nodeid, num_ues, wlanid):
    """set node position in canvas for each node."""
    maxfill = 5
    layer = int(num_ues / maxfill / 2)
    halfstart = num_ues // 2 // maxfill * maxfill + 1
    xcenter, ycenter = 300, 150 if num_ues < max(halfstart, 11) else 150 + 100 * (layer - 1)
    j = (nodeid - 1) % maxfill
    k = 1 + (nodeid - 1) // maxfill
    if nodeid == 0:
        xpos, ypos = xcenter + 150, ycenter
    elif nodeid == wlanid:
        xpos, ypos = xcenter, ycenter
    elif nodeid < halfstart:
        xpos, ypos = 100 + 150 * j, ycenter - 100 * k
    else:
        xpos, ypos = 100 + 150 * j, ycenter + 100 * k -100 * layer
    label_delta = 60 if ypos < ycenter else 40
    posnode = str(xpos) + '.0 ' + str(ypos) + '.0'
    poslabel = str(xpos) + '.0 ' + str(ypos + label_delta) + '.0'
    return posnode, poslabel

def generate_nodes(num_ues):
    """Construct configuration information for nodes and links."""
    wlanid = num_ues + 1
    len_nodes = num_ues + 2
    wlandname = 'n' + str(len_nodes)
    uemax = str(num_ues)
    for i in range(len_nodes):
        ueid, nid = str(i), str(i + 1)
        nodename = 'n' + nid
        hostname = 'eNB' if i == 0 else 'PROXY' if i == wlanid else 'UE' + ueid
        posnode, poslabel = get_pos(i, num_ues, wlanid)
        args = nodename, hostname, nid, posnode, poslabel, wlandname, uemax, wlanid
        add_nodeinfo(args)
    add_linkinfo_and_options(wlandname, len_nodes)

def main():
    """Generate node information for the number of nodes specified as the 2nd arguments."""
    default_num_ues = 1
    num_ues = int(sys.argv[1]) if len(sys.argv) > 1 else default_num_ues
    if num_ues < 1:
        sys.exit('The number of UEs is requied to be positive integer.')
    generate_nodes(num_ues)

if __name__ == '__main__':
    main()
