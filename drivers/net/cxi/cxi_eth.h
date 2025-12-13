/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2025 Hewlett Packard Enterprise Development LP
 */

#ifndef _CXI_ETH_H_
#define _CXI_ETH_H_

#include <ethdev_driver.h>

int
cxi_tx_queue_setup(struct rte_eth_dev *dev, uint16_t tx_queue_id,
        uint16_t nb_tx_desc, unsigned int socket_id,
        const struct rte_eth_txconf *tx_conf);

void
cxi_tx_queue_release(struct rte_eth_dev *dev, uint16_t tx_queue_id);

int
cxi_stats_get(struct rte_eth_dev *dev, struct rte_eth_stats *stats,
		struct eth_queue_stats *qstats);

int cxi_stats_reset(struct rte_eth_dev *dev);

#endif
