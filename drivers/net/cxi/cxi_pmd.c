/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2025 Hewlett Packard Enterprise Development LP
 */

#include "cxi_pmd.h"

#include <bus_vdev_driver.h>

#include "cxi_eth.h"
#include "cxi_txrx.h"
#include "rte_cxi_pmd.h"

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

    // These calls will change the parameter values only for the keys found in the kvlist
    rc = rte_kvargs_process(kvlist, CXI_PMD_NUM_QUEUES, parse_arg_count, params->nb_tx_queues);
    if (rc) goto return_code;
    if (params->nb_tx_queues > CXI_MAX_QUEUES) {
        PMD_LOG(ERR, "Requested number of TX queues %u greater than max %u",
                params->nb_tx_queues, CXI_MAX_QUEUES);
        rc = -ENODEV;
        goto return_code;
    }

return_code:
    return rc;
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

static int
cxi_probe(struct rte_vdev_device *dev)
{
    PMD_LOG(INFO, "cxi_probe on NUMA socket %u", rte_socket_id());

    const char *name = rte_vdev_device_name(dev);

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

    int rc;
    struct rte_eth_dev * const eth_dev = rte_eth_vdev_allocate(dev, sizeof(*internals));
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

    eth_dev->dev_ops = &ops;
    eth_dev->tx_pkt_burst = cxi_tx_pkt_burst;
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
RTE_PMD_REGISTER_PARAM_STRING(net_cxi, CXI_PMD_NUM_QUEUES "=<int> ");
