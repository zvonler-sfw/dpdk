/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include "cxi_pmd.h"
#include "cxi_txrx.h"

#include <rte_mbuf.h>

uint16_t
cxi_tx_pkt_burst(void *rte_q, struct rte_mbuf **bufs, uint16_t nb_bufs)
{
    struct cxi_tx_queue *q = rte_q;

    uint64_t bytes = 0;

    for (int i = 0; i < nb_bufs; ++i)
            bytes += rte_pktmbuf_pkt_len(bufs[i]);

    rte_pktmbuf_free_bulk(bufs, nb_bufs);
    q->stats.tx_pkts += nb_bufs;
    q->stats.tx_bytes += bytes;

    /*
    struct cxi_tx_queue * const txq = q;
    struct c_dma_eth_cmd eth_cmd = {
        .read_lac = txq->dev->phys_lac,
        .fmt = C_PKT_FORMAT_STD,
        .checksum_ctrl = C_CHECKSUM_CTRL_NONE,
        .eq = txq->eq->eqn,
    };

    for (int i = 0; i < nb_bufs; i++) {
        struct rte_mbuf *m = bufs[i];
        eth_cmd.user_ptr = rte_pktmbuf_mtod(m, uintptr_t);
        eth_cmd.total_len = m->data_len;
        int rc = cxi_cq_emit_dma_eth(txq, &eth_cmd);
        if (rc) {
            txq->stats.tx_busy++;
            txq->stats.tx_dropped += nb_bufs - i;
            return i;
        }
        txq->stats.dma++;
        txq->stats.dma_bytes += m->data_len;
    }
    */

    return nb_bufs;
}

uint16_t
cxi_rx_pkt_burst(void *rte_q, struct rte_mbuf **bufs, uint16_t nb_bufs)
{
    struct cxi_rx_queue *q = rte_q;

    int packet_size = q->internals->packet_size;
    if (rte_pktmbuf_alloc_bulk(q->mb_pool, bufs, nb_bufs) != 0)
        return 0;

    for (int i = 0; i < nb_bufs; ++i) {
        bufs[i]->data_len = packet_size;
        bufs[i]->pkt_len = packet_size;
        bufs[i]->port = q->internals->port_id;
    }

    q->stats.rx_pkts += nb_bufs;
    q->stats.rx_bytes += nb_bufs * packet_size;
    return nb_bufs;
}
