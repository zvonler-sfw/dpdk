# PMD Driver Development Plan

## 1 Jan 2026 - 31 Mar 2026

### Deliverable: Ping test on simulated Cassini hardware

A `dpdk-testpmd` instance should be able to use the Cassini PMD to connect two
simulated Cassini network interfaces so that UDP packets received on one port
are forwarded and transmitted back on the other. This forwarding functionality
should be demonstrated with a packet generator and packet capture.

### Functionality Milestones

#### Cassini packet generator: `cxi_udp_gen`

 * ☑ Fixed to load and shutdown without error
 * ☑ Generates packets

#### Netsim setup

 * ☑ Able to start VM with two interfaces
 * ☐ Able to start two VMs with connected interfaces
 * ☐ Able to send packets from one VM and capture on another

#### Cassini PMD driver

 * ☑ Loads without error
 * ☑ Passes `test_ethdev_api`
 * ☑ Passes `pmd_perf_autotest`
 * ☐ Initializes CXI device
 * ☐ Forwards packets
 * ☐ Spreads traffic across multiple TX/RX queues
 * ☐ Cooperates with other clients of the PF
