/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2025 Hewlett Packard Enterprise Development LP
 */

#include "rte_cxi_pmd.h"

#include <bus_vdev_driver.h>

struct cxi_tx_queue {
    struct cxi_cq *cq;
    struct cxi_eq *eq;
};

struct pmd_internals {
    struct cxil_dev *dev;
    struct cxil_lni *lni;
    struct cxi_cp *cp;

    struct cxi_tx_queue[CXI_MAX_QUEUES];
};

#define CXI_PMD_NUM_QUEUES "num_queues"

static const char * const pmd_params[] = {
    CXI_PMD_NUM_QUEUES,
    NULL
};

static int
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

static void
cxi_tx_queue_release(struct rte_eth_dev *dev, uint16_t tx_queue_id)
{
    struct rte_eth_dev_data *config = dev->data;
    config->tx_queues[tx_queue_id] = NULL;
}


static const struct eth_dev_ops ops = {
	.tx_queue_setup = cxi_tx_queue_setup,
	.tx_queue_release = cxi_tx_queue_release,
    /**
	.dev_close = eth_dev_close,
	.dev_start = eth_dev_start,
	.dev_stop = eth_dev_stop,
	.dev_configure = eth_dev_configure,
	.dev_infos_get = eth_dev_info,
	.rx_queue_setup = eth_rx_queue_setup,
	.rx_queue_release = eth_rx_queue_release,
	.mtu_set = eth_mtu_set,
	.link_update = eth_link_update,
	.mac_addr_set = eth_mac_address_set,
	.stats_get = eth_stats_get,
	.stats_reset = eth_stats_reset,
	.reta_update = eth_rss_reta_update,
	.reta_query = eth_rss_reta_query,
	.rss_hash_update = eth_rss_hash_update,
	.rss_hash_conf_get = eth_rss_hash_conf_get
    */
};

static uint16_t
cxi_tx_pkt_burst(void *q, struct rte_mbuf **bufs, uint16_t nb_bufs)
{
    // !@# q is created in cxi_tx_queue_setup
}

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
configure_rte_eth_dev(struct ret_eth_dev * const eth_dev, struct rte_kvargs * const kvlist)
{
    struct rte_eth_dev_data * const params = eth_dev->data;

    // Set the default configuration
    params->nb_tx_queues = 1;

    // These calls will change the parameter values only for the keys found in the kvlist
    rc = rte_kvargs_process(kvlist, CXI_PMD_NUM_QUEUES, parse_arg_count, params->nb_tx_queues);
    if (rc) goto return_code;

return_code:
    return rc;
}

static int
cxi_probe(struct rte_vdev_device *dev)
{
    PMD_LOG(INFO, "cxi_probe on NUMA socket %u", rte_socket_id());

    const char *name = rte_vdev_device_name(dev);

    int rc;
    if (rte_eal_process_type() == RTE_PROC_SECONDARY) {
        struct pmd_internals *internals;
        eth_dev = rte_eth_dev_attach_secondary(name);
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

    struct rte_eth_dev * const eth_dev = rte_eth_vdev_allocate(dev, sizeof(*internals));
    if (!eth_dev) {
        rc = -ENOMEM;
        goto return_error;
    }
    eth_dev->dev_ops = &ops;
    eth_dev->tx_pkt_burst = cxi_tx_pkt_burst;

    // Parse parameters to the PMD
    struct rte_kvargs *kvlist = rte_kvargs_parse(rte_vdev_device_args(dev), pmd_params);
    if (kvlist == NULL) {
        PMD_LOG(ERR, "Failed to parse PMD parameters");
        rc = -EINVAL;
        goto free_eth_dev;
    }
    configure_rte_eth_dev(eth_dev, kvlist);
    ret_kvargs_free(kvlist);


    // The dev_private member points to a structure that maintains the
    // CXI specific data. Any error setting up CXI will be logged and
    // returned to the DPDK framework as a failure to probe.
    struct pmd_internals * const internals = eth_dev->data->dev_private;
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
    // End of CXI initialization

    rte_eth_dev_probing_finish(eth_dev);
    return 0;
    // End of normal execution path

destroy_cp:
    // !@# Call can fail but we may not care since already in error path
    cxil_destroy_cp(internals->cp);
destroy_lni:
    // !@# Call can fail but we may not care since already in error path
    cxil_destroy_lni(internals->lni);
close_device:
    cxil_close_device(internals->dev);
free_eth_dev:
    rte_eth_dev_release_port(eth_dev);
return_error:
    return rc;
}

static int
cxi_remove(struct rte_vdev_device *dev)
{
    PMD_LOG(INFO, "cxi_remove on NUMA socket %u", rte_socket_id());

    const char *name = rte_vdev_device_name(dev);
    struct rte_eth_dev *eth_dev = rte_eth_vdev_allocated(name);
    if (eth_dev == NULL)
        goto finished; // Nothing to do

    if (rte_eal_process_type() != RTE_PROC_PRIMARY)
        goto release_port;

    // Only the primary process does the CXI teardown
    struct pmd_internals *internals = eth_dev->data->dev_private;
    cxil_destroy_cp(internals->cp);
    cxil_destroy_lni(internals->lni);
    cxil_close_device(internals->dev);

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
RTE_PMD_REGISTER_PARAM_STRING(net_cxi, "num_queues=<int> ");
