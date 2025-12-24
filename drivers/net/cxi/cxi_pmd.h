/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2025 Hewlett Packard Enterprise Development LP
 */

#ifndef _CXI_PMD_H
#define _CXI_PMD_H

#include <stdint.h>

#include <ethdev_driver.h>

#include "rte_cxi_pmd.h"

#define CXI_MAX_QUEUES 16
#define CXI_MAX_RX_PKTLEN ((uint32_t)-1)

struct cxi_tx_queue {
    struct pmd_internals *internals;
    struct cxi_cq *cq;
    struct cxi_eq *eq;

    struct {
        RTE_ATOMIC(uint64_t) tx_pkts;
        RTE_ATOMIC(uint64_t) tx_bytes;
        RTE_ATOMIC(uint64_t) tx_busy;
        RTE_ATOMIC(uint64_t) tx_dropped;
    } stats;
};

struct pmd_internals;

struct cxi_rx_queue {
    struct pmd_internals *internals;
    struct rte_mempool *mb_pool;

    struct {
        uint64_t rx_pkts;
        uint64_t rx_bytes;
    } stats;
};

struct cxi_queue_pair {
    struct cxi_tx_queue *txq;
    struct cxi_rx_queue *rxq;
};

/**
 * The pmd_internals struct holds CXI-specific members used for making
 * calls into libcxi.
 */
struct pmd_internals {
    struct cxil_dev *dev;
    struct cxil_lni *lni;
    struct cxi_cp *cp;

    struct cxi_queue_pair qps[CXI_MAX_QUEUES];

    struct rte_ether_addr eth_addr;

    int packet_size;
    int port_id;
};

#endif
