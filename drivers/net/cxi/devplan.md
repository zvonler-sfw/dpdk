# PMD Driver Development Plan

## 1 Jan 2026 - 31 Mar 2026

### Deliverable: Ping test on simulated Cassini hardware

A `dpdk-testpmd` instance should be able to use the Cassini PMD to connect two
simulated Cassini network interfaces so that UDP packets received on one port
are forwarded and transmitted back on the other. This forwarding functionality
should be demonstrated with a packet generator and packet capture.

#### Ping test identification

Using https://github.com/kogdenko/dpdk-ping

 * ☑ Test executes on memif interface
 * ☑ Test executes on MLX5 hardware in SFW host
   * ~40GB/s achievable with these commands:
     * `sudo dpdk-testpmd -l 14-22 -a 0000:82:00.0 --proc-type=primary --file-prefix=pmd -- -i --port-topology=loop --nb-cores=8 --txq=8 --rxq=8`
     * `sudo build/dpdk-ping --proc-type=primary --file-prefix=ping -- -B 2000000 -L 1200 -R on -l 23-27 -H 72:55:0B:F5:5A:2D -t 30 -p 0000:82:00.1 -b true`
 
#### Cassini packet generator/receiver: `cxi_udp_gen`

 * ☑ Fixed to load and shutdown without error
 * ☑ Generates packets visible to tcpdump
 * ☑ Updating to use cxi_context like other test programs
 * ☑ Adding receive functionality - learning to program set list
 * ☑ Receive side now working

#### Netsim setup

Moved environment to cn10.wham.cray.next.com

 * ☑ Able to start VM with two interfaces
 * ☑ Able to send packets from one interface and capture on another
 * ☑ Understand how ethernet devices are added to set list by test-eth-driver script
 * ☐ Demonstrate cxi_udp_gen traffic simultaneous with regular ethernet

#### Cassini PMD driver

 * ☑ Loads without error
 * ☑ Passes `test_ethdev_api`
 * ☑ Passes `pmd_perf_autotest`
 * ☐ Initializes CXI device
 * ☐ Forwards packets
 * ☐ Spreads traffic across multiple TX/RX queues
 * ☐ Cooperates with other clients of the PF

### Functionality Milestones

 * ☑ Send UDP packets with cxi_udp_gen using libcxi API
 * ☑ Receive UDP packets with cxi_udp_gen using libcxi API
 * ☐ Send UDP packets with testpmd and DPDK CXI driver
 * ☐ Receive UDP packets with testpmd and DPDK CXI driver

