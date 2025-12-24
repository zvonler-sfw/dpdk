/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include "cxi_eth.h"

#include "cxi_pmd.h"

int
cxi_rx_queue_setup(struct rte_eth_dev *dev, uint16_t rx_queue_id,
        uint16_t nb_rx_desc __rte_unused,
        unsigned int socket_id __rte_unused,
        const struct rte_eth_rxconf *rx_conf __rte_unused,
        struct rte_mempool *mb_pool)
{
    PMD_LOG(INFO, "cxi_rx_queue_setup");

    struct rte_eth_dev_data *config = dev->data;
    struct pmd_internals *internals = config->dev_private;

    if (rx_queue_id >= config->nb_rx_queues)
        return -ENODEV;

    struct cxi_rx_queue *rxq = internals->qps[rx_queue_id].rxq;
    rxq->mb_pool = mb_pool;
    config->rx_queues[rx_queue_id] = rxq;

    return 0;
}

void
cxi_rx_queue_release(struct rte_eth_dev *dev, uint16_t qid)
{
    PMD_LOG(INFO, "cxi_rx_queue_release");
    dev->data->rx_queues[qid] = NULL;
}

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

        uint64_t opackets = txq->stats.tx_pkts;
        uint64_t obytes = txq->stats.tx_bytes;
        uint64_t tx_dropped = txq->stats.tx_dropped;
        uint64_t ipackets = rxq->stats.rx_pkts;
        uint64_t ibytes = rxq->stats.rx_bytes;

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
        txq->stats.tx_pkts = 0;
        txq->stats.tx_bytes = 0;
        txq->stats.tx_busy = 0;
        txq->stats.tx_dropped = 0;

        struct cxi_rx_queue * const rxq = qp->rxq;
        rxq->stats.rx_pkts = 0;
        rxq->stats.rx_bytes = 0;
    }

    return 0;
}

int cxi_eth_dev_configure(struct rte_eth_dev *dev) {
    RTE_SET_USED(dev);
    PMD_LOG(INFO, "cxi_eth_dev_configure");
    return 0;
}

int cxi_eth_dev_infos_get(struct rte_eth_dev *dev, struct rte_eth_dev_info *infos) {
    PMD_LOG(INFO, "cxi_eth_dev_infos_get");

    if ((dev == NULL) || (infos == NULL))
        return -EINVAL;

    struct pmd_internals *internals = dev->data->dev_private;

    infos->max_mac_addrs = 1;
    infos->max_rx_queues = RTE_DIM(internals->qps);
    infos->max_tx_queues = RTE_DIM(internals->qps);
    infos->max_rx_pktlen = CXI_MAX_RX_PKTLEN;

    return 0;
}

int
cxi_eth_rx_queue_setup(struct rte_eth_dev *dev, uint16_t rx_queue_id,
		uint16_t nb_rx_desc __rte_unused,
		unsigned int socket_id __rte_unused,
		const struct rte_eth_rxconf *rx_conf __rte_unused,
		struct rte_mempool *mb_pool)
{
    PMD_LOG(INFO, "cxi_eth_rx_queue_setup");

    if ((dev == NULL) || (mb_pool == NULL))
        return -EINVAL;

    if (rx_queue_id >= dev->data->nb_rx_queues)
            return -ENODEV;

    struct pmd_internals *internals = dev->data->dev_private;
    RTE_SET_USED(internals);

    dev->data->rx_queues[rx_queue_id] = internals->qps[rx_queue_id].rxq;

    return 0;
}

int
cxi_eth_dev_start(struct rte_eth_dev *dev)
{
    PMD_LOG(INFO, "cxi_eth_dev_start");

    if (dev == NULL)
        return -EINVAL;

    dev->data->dev_link.link_status = RTE_ETH_LINK_UP;

    for (int ctr = 0; ctr < dev->data->nb_rx_queues; ++ctr)
        dev->data->rx_queue_state[ctr] = RTE_ETH_QUEUE_STATE_STARTED;
    for (int ctr = 0; ctr < dev->data->nb_tx_queues; ++ctr)
        dev->data->tx_queue_state[ctr] = RTE_ETH_QUEUE_STATE_STARTED;

    return 0;
}

int
cxi_eth_dev_stop(struct rte_eth_dev *dev)
{
    PMD_LOG(INFO, "cxi_eth_dev_stop");

    if (dev == NULL)
        return -EINVAL;

    dev->data->dev_link.link_status = RTE_ETH_LINK_DOWN;

    for (int ctr = 0; ctr < dev->data->nb_rx_queues; ++ctr)
        dev->data->rx_queue_state[ctr] = RTE_ETH_QUEUE_STATE_STOPPED;
    for (int ctr = 0; ctr < dev->data->nb_tx_queues; ++ctr)
        dev->data->tx_queue_state[ctr] = RTE_ETH_QUEUE_STATE_STOPPED;

    return 0;
}

int
cxi_eth_link_update(struct rte_eth_dev *dev __rte_unused,
		int wait_to_complete __rte_unused) {
    return 0;
}

int
cxi_eth_dev_close(struct rte_eth_dev *dev)
{
    PMD_LOG(INFO, "cxi_eth_dev_close");
    dev->data->mac_addrs = NULL;
    return 0;
}

