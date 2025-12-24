/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2025 Hewlett Packard Enterprise Development LP
 */

#include "cxi_pmd.h"

#include <ethdev_driver.h>
#include <ethdev_vdev.h>
#include <bus_vdev_driver.h>
#include <rte_kvargs.h>

#include "cxi_eth.h"
#include "cxi_txrx.h"

// Stubs
struct cxi_cq {};
struct cxi_eq {};
struct cxil_dev {};
struct cxil_lni {};
struct cxi_cp {};

RTE_LOG_REGISTER_DEFAULT(cxi_pmd_logtype, NOTICE);

static struct rte_eth_link pmd_link = {
	.link_speed = RTE_ETH_SPEED_NUM_10G,
	.link_duplex = RTE_ETH_LINK_FULL_DUPLEX,
	.link_status = RTE_ETH_LINK_DOWN,
	.link_autoneg = RTE_ETH_LINK_FIXED,
};

// The parameters that can be passed to the PMD are defined here.
#define CXI_PMD_NUM_QUEUES "num_queues"

static const char * const pmd_params[] = {
    CXI_PMD_NUM_QUEUES,
    NULL
};

static int
parse_arg_count(char const* name, char const *value, void *dest)
{
    char *end;
    int count = strtol(value, &end, 10);
    if (count < 1) {
        PMD_LOG(ERR, "Argument for parameter %s must be a positive integer", name);
        return -EINVAL;
    }
    *((int*)dest) = count;
    return 0;
}

// Configures the RTE ethernet device from the passed parameters
static int
configure_rte_eth_dev(struct rte_eth_dev * const eth_dev, struct rte_kvargs * const kvlist)
{
    struct rte_eth_dev_data * const params = eth_dev->data;

    // Set the default configuration
    params->nb_tx_queues = 1;
    params->nb_rx_queues = 1;
    params->dev_flags |= RTE_ETH_DEV_AUTOFILL_QUEUE_XSTATS;

    // These calls will change the parameter values only for the keys found in the kvlist
    int rc = rte_kvargs_process(kvlist, CXI_PMD_NUM_QUEUES, parse_arg_count, &params->nb_tx_queues);
    if (rc) goto return_code;
    if (params->nb_tx_queues > CXI_MAX_QUEUES) {
        PMD_LOG(ERR, "Requested number of queues %u greater than max %u",
                params->nb_tx_queues, CXI_MAX_QUEUES);
        rc = -ENODEV;
        goto return_code;
    }
    params->nb_rx_queues = params->nb_tx_queues;

return_code:
    return rc;
}

static const struct eth_dev_ops ops = {
	.rx_queue_setup = cxi_rx_queue_setup,
	.rx_queue_release = cxi_rx_queue_release,
	.tx_queue_setup = cxi_tx_queue_setup,
	.tx_queue_release = cxi_tx_queue_release,
	.stats_get = cxi_stats_get,
	.dev_configure = cxi_eth_dev_configure,
	.dev_infos_get = cxi_eth_dev_infos_get,
	.dev_start = cxi_eth_dev_start,
	.dev_stop = cxi_eth_dev_stop,
	.link_update = cxi_eth_link_update,
	.dev_close = cxi_eth_dev_close,
    /**
	.mtu_set = eth_mtu_set,
	.mac_addr_set = eth_mac_address_set,
	.stats_reset = eth_stats_reset,
	.reta_update = eth_rss_reta_update,
	.reta_query = eth_rss_reta_query,
	.rss_hash_update = eth_rss_hash_update,
	.rss_hash_conf_get = eth_rss_hash_conf_get
    */
};

static int
cxi_probe(struct rte_vdev_device *dev)
{
    bool primary = rte_eal_process_type() != RTE_PROC_SECONDARY;

    PMD_LOG(INFO, "%s process cxi_probe on NUMA socket %u",
        primary ? "Primary" : "Secondary", rte_socket_id());

    const char *name = rte_vdev_device_name(dev);

    if (rte_eal_process_type() == RTE_PROC_SECONDARY) {
        struct rte_eth_dev * const eth_dev = rte_eth_dev_attach_secondary(name);
        if (!eth_dev) {
            PMD_LOG(ERR, "Failed to probe %s", name);
            return -1;
        }
        // !@# What fields need to be set?
        eth_dev->dev_ops = &ops;
        eth_dev->device = &dev->device;
        rte_eth_dev_probing_finish(eth_dev);
        return 0;
    }

    if (dev->device.numa_node == SOCKET_ID_ANY)
        dev->device.numa_node = rte_socket_id();

    int rc;
    struct rte_eth_dev * const eth_dev = rte_eth_vdev_allocate(dev, sizeof(struct pmd_internals));
    if (!eth_dev) {
        rc = -ENOMEM;
        goto return_error;
    }

    // Parse parameters to the PMD and configure the device
    struct rte_kvargs *kvlist = rte_kvargs_parse(rte_vdev_device_args(dev), pmd_params);
    if (kvlist == NULL) {
        PMD_LOG(ERR, "Failed to parse PMD parameters");
        rc = -EINVAL;
        goto free_eth_dev;
    }
    rc = configure_rte_eth_dev(eth_dev, kvlist);
    rte_kvargs_free(kvlist);
    if (rc)
        goto free_eth_dev;
    // End of parameter parsing and configuration

    // The dev_private member points to a structure that maintains the
    // CXI specific data. Any error setting up CXI will be logged and
    // returned to the DPDK framework as a failure to probe.
    struct pmd_internals * const internals = eth_dev->data->dev_private;
    internals->packet_size = 64;
    rte_eth_random_addr(internals->eth_addr.addr_bytes);
    internals->port_id = eth_dev->data->port_id;

    eth_dev->data->mac_addrs = &internals->eth_addr;
    eth_dev->data->promiscuous = 1;
    eth_dev->data->dev_link = pmd_link;

    for (int ctr = 0; ctr < CXI_MAX_QUEUES; ++ctr) {
        struct cxi_rx_queue *rxq =
            rte_zmalloc_socket(NULL, sizeof(struct cxi_rx_queue), 0, rte_socket_id());
        struct cxi_tx_queue *txq =
            rte_zmalloc_socket(NULL, sizeof(struct cxi_tx_queue), 0, rte_socket_id());
        txq->internals = internals;
        internals->qps[ctr].txq = txq;
        rxq->internals = internals;
        internals->qps[ctr].rxq = rxq;
    }

    /**
    rc = cxil_open_device(0, &internals->dev);
    if (rc) {
        PMD_LOG(ERR, "Failed to open CXI device: %d", rc);
        goto free_eth_dev;
    }
    rc = cxil_alloc_lni(&internals->dev, &internals->lni, CXI_DEFAULT_SVC_ID);
    if (rc) {
        PMD_LOG(ERR, "Failed to allocate LNI: %d", rc);
        goto close_device;
    }
    rc = cxil_alloc_cp(&internals->lni, 0, CXI_TC_ETH, CXI_TC_TYPE_DEFAULT, &internals->cp);
    if (rc) {
        PMD_LOG(ERR, "Failed to allocate CP: %d", rc);
        goto destroy_lni;
    }
    */
    // End of CXI initialization

    eth_dev->dev_ops = &ops;
    eth_dev->tx_pkt_burst = cxi_tx_pkt_burst;
    eth_dev->rx_pkt_burst = cxi_rx_pkt_burst;
    rte_eth_dev_probing_finish(eth_dev);

    PMD_LOG(INFO, "cxi_probe finished successfully eth_dev->dev_ops %p ops.dev_configure %p", eth_dev->dev_ops, ops.dev_configure);
    return 0;
    // End of normal execution path

//destroy_cp:
    // !@# Call can fail but we may not care since already in error path
    //cxil_destroy_cp(internals->cp);
//destroy_lni:
    // !@# Call can fail but we may not care since already in error path
    //cxil_destroy_lni(internals->lni);
//close_device:
    //cxil_close_device(internals->dev);
free_eth_dev:
    rte_eth_dev_release_port(eth_dev);
return_error:
    PMD_LOG(INFO, "cxi_probe returning error %d", rc);
    return rc;
}

static int
cxi_remove(struct rte_vdev_device *dev)
{
    PMD_LOG(INFO, "NUMA socket %u", rte_socket_id());

    const char *name = rte_vdev_device_name(dev);
    struct rte_eth_dev *eth_dev = rte_eth_dev_allocated(name);
    if (eth_dev == NULL)
        goto finished; // Nothing to do

    if (rte_eal_process_type() != RTE_PROC_PRIMARY)
        goto release_port;

    // Only the primary process does the CXI teardown
    struct pmd_internals *internals = eth_dev->data->dev_private;

    for (int ctr = 0; ctr < CXI_MAX_QUEUES; ++ctr) {
        rte_free(internals->qps[ctr].txq);
        rte_free(internals->qps[ctr].rxq);
    }

    cxi_eth_dev_close(eth_dev);

    /**
    cxil_destroy_cp(internals->cp);
    cxil_destroy_lni(internals->lni);
    cxil_close_device(internals->dev);
    */

release_port:
    rte_eth_dev_release_port(eth_dev);
finished:
    return 0;
}

static struct rte_vdev_driver cxi_pmd = {
    .probe = cxi_probe,
    .remove = cxi_remove,
};

RTE_PMD_REGISTER_VDEV(net_cxi, cxi_pmd);
RTE_PMD_REGISTER_PARAM_STRING(net_cxi, CXI_PMD_NUM_QUEUES "=<int> ");
