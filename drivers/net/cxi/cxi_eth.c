/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include "cxi_eth.h"

#include "cxi_pmd.h"

/**
 * Called by the DPDK application to set up a transmit queue.
 */
int
cxi_tx_queue_setup(struct rte_eth_dev *dev, uint16_t tx_queue_id,
        uint16_t nb_tx_desc, unsigned int socket_id,
        const struct rte_eth_txconf *tx_conf)
{
    RTE_SET_USED(nb_tx_desc);
    RTE_SET_USED(socket_id);
    RTE_SET_USED(tx_conf);

    // !@# is this check needed?
    if (dev == NULL)
        return -EINVAL;

    struct rte_eth_dev_data *config = dev->data;
    struct pmd_internals *internals = config->dev_private;

    if (tx_queue_id >= config->nb_tx_queues)
        return -ENODEV;

    config->tx_queues[tx_queue_id] =
        (struct pmd_internals*)internals->qps[tx_queue_id].txq;

    return 0;
}

/**
 * Called by the DPDK application to release a transmit queue.
 */
void
cxi_tx_queue_release(struct rte_eth_dev *dev, uint16_t tx_queue_id)
{
    struct rte_eth_dev_data *config = dev->data;
    config->tx_queues[tx_queue_id] = NULL;
}

/**
 * Called by a DPDK application to fill in the stats and qstats structures.
 */
int
cxi_stats_get(struct rte_eth_dev *dev, struct rte_eth_stats *stats,
		struct eth_queue_stats *qstats)
{
    struct rte_eth_dev_data *config = dev->data;
    struct pmd_internals * const internals = config->dev_private;

    for (int i = 0; i < config->nb_tx_queues; ++i) {
        struct cxi_queue_pair const* const qp = &internals->qps[i];
        struct cxi_tx_queue const* const txq = qp->txq;
        struct cxi_rx_queue const* const rxq = qp->rxq;

        uint64_t opackets = txq->stats.dma;
        uint64_t obytes = txq->stats.dma_bytes;
        uint64_t tx_dropped = txq->stats.tx_dropped;
        uint64_t ipackets = rxq->stats.packets;
        uint64_t ibytes = rxq->stats.bytes;

        stats->opackets += opackets;
        stats->obytes += obytes;
        stats->oerrors += tx_dropped;
        stats->ipackets += ipackets;
        stats->ibytes += ibytes;
        // !@# Can we compute imissed?

        // The qstats pointer is non-null only if RTE_ETH_DEV_AUTOFILL_QUEUE_XSTATS is set
        if (qstats && i < RTE_ETHDEV_QUEUE_STAT_CNTRS) {
            qstats->q_opackets[i] = opackets;
            qstats->q_obytes[i] = obytes;
            qstats->q_errors[i] += tx_dropped;
            qstats->q_ipackets[i] += ipackets;
            qstats->q_ibytes[i] += ibytes;
        }
    }

    return 0;
}

int
cxi_stats_reset(struct rte_eth_dev *dev) {
    struct rte_eth_dev_data *config = dev->data;
    struct pmd_internals * const internals = config->dev_private;

    for (int i = 0; i < config->nb_tx_queues; ++i) {
        struct cxi_queue_pair const* const qp = &internals->qps[i];

        struct cxi_tx_queue * const txq = qp->txq;
        txq->stats.dma = 0;
        txq->stats.dma_bytes = 0;
        txq->stats.tx_busy = 0;
        txq->stats.tx_dropped = 0;

        struct cxi_rx_queue * const rxq = qp->rxq;
        rxq->stats.packets = 0;
        rxq->stats.bytes = 0;
    }

    return 0;
}

