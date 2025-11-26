/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include "cxi_eth.h"

/**
 * Called by the DPDK application to set up a transmit queue.
 */
int
cxi_tx_queue_setup(struct rte_eth_dev *dev, uint16_t tx_queue_id,
        uint16_t nb_tx_desc, unsigned int socket_id,
        const struct rte_eth_txconf *tx_conf)
{
    // !@# is this check needed?
    if (dev == NULL)
        return -EINVAL;

    struct rte_eth_dev_data *config = dev->data;
    struct pmd_internals *internals = config->dev_private;

    if (tx_queue_id >= config->nb_tx_queues)
        return -ENODEV;

    config->tx_queues[tx_queue_id] = internals->cxi_tx_queues[tx_queue_id];

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
        uint64_t packets = internals->cxi_tx_queue[i].stats.dma;
        uint64_t bytes = internals->cxi_tx_queue[i].stats.dma_bytes;
        uint64_t tx_dropped = internals->cxi_tx_queue[i].stats.tx_dropped;

        stats->opackets += packets;
        stats->obytes += bytes;
        stats->oerrors += tx_dropped;

        // The qstats pointer is non-null only if RTE_ETH_DEV_AUTOFILL_QUEUE_XSTATS is set
        if (qstats && i < RTE_ETHDEV_QUEUE_STAT_CNTRS) {
            qstats->q_opackets[i] = packets;
            qstats->q_obytes[i] = bytes;
            qstats->q_errors[i] += tx_dropped;
        }
    }

    return 0;
}
