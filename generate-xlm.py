#!/usr/bin/env python3
"""
Generate .xml file for a multi-UE scenario

  Example: ./generate-xml.py 5 > Core_PROXY_5_UEs.imn
"""

import sys

TITLE = '''\
<?xml version='1.0' encoding='UTF-8'?>'''

TAIL = '''\
</scenario>'''

SERVICES = '''\
      <services>
        <service name="zebra"/>
        <service name="OSPFv3MDR"/>
        <service name="IPForward"/>
      </services>'''

SESSION_METADATA = '''\
  <session_metadata>
    <configuration name="canvas c1" value="{name {Canvas1}}"/>
    <configuration name="global_options" value="interface_names=no ip_addresses=yes ipv6_addresses=no node_labels=yes link_labels=yes show_api=no background_images=no annotations=yes grid=yes traffic_start=0"/>
  </session_metadata>'''

DEFAULT_SERVICES = '''\
  <default_services>
    <node type="mdr">
      <service name="zebra"/>
      <service name="OSPFv3MDR"/>
      <service name="IPForward"/>
    </node>
    <node type="PC">
      <service name="DefaultRoute"/>
    </node>
    <node type="prouter"/>
    <node type="router">
      <service name="zebra"/>
      <service name="OSPFv2"/>
      <service name="OSPFv3"/>
      <service name="IPForward"/>
    </node>
    <node type="host">
      <service name="DefaultRoute"/>
      <service name="SSH"/>
    </node>
  </default_services>'''

def print_lines(prototype):
    """Print prototype lines without any change."""
    lines = prototype.split('\n')
    for line in lines:
        print(line)

def add_nodeinfo(args):
    """Construct replace table and add node information."""
    nodeid, nodename, posnode, poslabel, networkid = args
    ntype = 'mdr' if nodeid != networkid else 'WIRELESS_LAN'
    xpos, ypos = posnode.split()
    if nodeid < networkid: 
        if nodeid == 1: 
            print('  <devices>')
        device = '    <device id="{}" name="{}" type="{}" class="" image="">'.format(nodeid, nodename, ntype) + '\n'
        device += '      <position x="{}" y="{}"/>'.format(xpos, ypos)
        print_lines(device)
        print_lines(SERVICES)
        print('    </device>')
        if nodeid == networkid -1:
            print('  </devices>')
    if nodeid == networkid:
        print('  <networks>')
        network = '    <network id="{}" name="{}" type="{}">'.format(nodeid, nodename, ntype) + '\n'
        network += '      <position x="{}" y="{}"/>'.format(xpos, ypos)
        print_lines(network)
        print('    </network>')
        print('  </networks>')

def add_linkinfo_and_options(len_nodes):
    """Add link information and further options."""
    print('  <links>')
    for i in range(1, len_nodes):
        link = '    <link node1="{}" node2="{}">'.format(len_nodes, i) + '\n'
        link += '      <iface2 id="0" name="eth0" ip4="10.0.0.{}" ip4_mask="32"/>'.format(i) + '\n'
        link += '    </link>'
        print_lines(link)
    print('  </links>')
    print_lines(SESSION_METADATA)
    print_lines(DEFAULT_SERVICES)

def get_pos(nodeidx, num_ues, wlanidx):
    """set node position in canvas for each node."""
    maxfill = 5
    layer = int(num_ues / maxfill / 2)
    halfstart = num_ues // 2 // maxfill * maxfill + 1
    xcenter, ycenter = 300, 150 if num_ues < max(halfstart, 11) else 150 + 100 * (layer - 1)
    j = (nodeidx - 1) % maxfill
    k = 1 + (nodeidx - 1) // maxfill
    if nodeidx == 0:
        xpos, ypos = xcenter + 150, ycenter
    elif nodeidx == wlanidx:
        xpos, ypos = xcenter, ycenter
    elif nodeidx < halfstart:
        xpos, ypos = 100 + 150 * j, ycenter - 100 * k
    else:
        xpos, ypos = 100 + 150 * j, ycenter + 100 * k -100 * layer
    label_delta = 60 if ypos < ycenter else 40
    posnode = str(xpos) + '.0 ' + str(ypos) + '.0'
    poslabel = str(xpos) + '.0 ' + str(ypos + label_delta) + '.0'
    return posnode, poslabel

def generate_nodes(num_ues):
    """Construct configuration information for nodes and links."""
    wlanidx = num_ues + 1
    len_nodes = num_ues + 2
    uemax = str(num_ues)
    #node_list = range(len_nodes)
    node_list = [wlanidx] + list(range(wlanidx))
    print_lines(TITLE)
    print('<scenario name="Core_PROXY_{}_UEs.xml">'.format(num_ues))
    for i in node_list:
        ueid, nodeid = str(i), i + 1
        nodename = 'eNB' if i == 0 else 'PROXY' if i == wlanidx else 'UE' + ueid
        posnode, poslabel = get_pos(i, num_ues, wlanidx)
        args = nodeid, nodename, posnode, poslabel, len_nodes
        add_nodeinfo(args)
    add_linkinfo_and_options(len_nodes)
    print_lines(TAIL)

def main():
    """Generate node information for the number of nodes specified as the 2nd arguments."""
    default_num_ues = 1
    num_ues = int(sys.argv[1]) if len(sys.argv) > 1 else default_num_ues
    if num_ues < 1:
        sys.exit('The number of UEs is requied to be positive integer.')
    generate_nodes(num_ues)

if __name__ == '__main__':
    main()
