/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include "cxi_txrx.h"

uint16_t
cxi_tx_pkt_burst(void *q, struct rte_mbuf **bufs, uint16_t nb_bufs)
{
    struct cxi_tx_queue * const txq = q;
    struct c_dma_eth_cmd eth_cmd = {
        /*
        .read_lac = txq->dev->phys_lac,
        .fmt = C_PKT_FORMAT_STD,
        .checksum_ctrl = C_CHECKSUM_CTRL_NONE,
        */
        .eq = txq->eq->eqn,
    };

    for (int i = 0; i < nb_bufs; i++) {
        struct rte_mbuf *m = bufs[i];
        eth_cmd.user_ptr = rte_pktmbuf_mtod(m, uintptr_t);
        eth_cmd.total_len = m->data_len;
        int rc = cxi_cq_emit_dma_eth(txq, &eth_cmd);
        if (rc) {
            txq->stats.tx_busy++;
            return i;
        }
        txq->stats.dma++;
        txq->stats.dma_bytes += m->data_len;
    }

    return nb_bufs;
}

