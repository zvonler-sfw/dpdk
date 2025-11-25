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


