/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2025 Hewlett Packard Enterprise Development LP
 */

#ifndef _CXI_TXRX_H_
#define _CXI_TXRX_H_

#include <stdint.h>
#include <rte_mbuf_core.h>

uint16_t
cxi_tx_pkt_burst(void *q, struct rte_mbuf **bufs, uint16_t nb_bufs);

uint16_t
cxi_rx_pkt_burst(void *q, struct rte_mbuf **bufs, uint16_t nb_bufs);

#endif
