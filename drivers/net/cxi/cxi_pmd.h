/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2025 Hewlett Packard Enterprise Development LP
 */

#ifndef _CXI_PMD_H
#define _CXI_PMD_H

struct cxi_tx_queue {
    struct cxi_cq *cq;
    struct cxi_eq *eq;

    struct {
        uint64_t dma;
        uint64_t dma_bytes;
        uint64_t tx_busy;
        uint64_t tx_dropped;
    } stats;
};

#define CXI_MAX_QUEUES 16

/**
 * The pmd_internals struct holds CXI-specific members used for making
 * calls into libcxi.
 */
struct pmd_internals {
    struct cxil_dev *dev;
    struct cxil_lni *lni;
    struct cxi_cp *cp;

    struct cxi_tx_queue[CXI_MAX_QUEUES];
};

#endif
