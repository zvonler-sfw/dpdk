/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2025 Hewlett Packard Enterprise Development LP
 */

#include "rte_cxi_pmd.h"

#include <bus_vdev_driver.h>

static int
cxi_probe(struct rte_vdev_device *dev)
{
    if (rte_eal_process_type() == RTE_PROC_SECONDARY) {
        PMD_LOG(ERR, "Probing by secondary process not yet supported");
        return -1;
    }

    int rc;

    // !@# Where does this struct live?
    struct cxil_dev *dev;
    rc = cxil_open_device(0, &dev);
    if (rc) {
        PMD_LOG(ERR, "Failed to open CXI device: %d", rc);
        goto return_error;
    }

    // !@# Where does this struct live?
    struct cxil_lni *lni;
    rc = cxil_alloc_lni(dev, &lni, CXI_DEFAULT_SVC_ID);
    if (rc) {
        PMD_LOG(ERR, "Failed to allocate LNI: %d", rc);
        goto close_device;
    }

    // !@# Where does this struct live?
    struct cxi_cp *cp;
    rc = cxil_alloc_cp(lni, 0, CXI_TC_ETH, CXI_TC_TYPE_DEFAULT, &cp);
    if (rc) {
        PMD_LOG(ERR, "Failed to allocate LNI: %d", rc);
        goto destroy_lni;
    }
    // !@# Errors below this point should goto destroy_cp

    struct cxi_cq_alloc_opts cq_opts = {
            .count = CXI_MAX_CQ_COUNT,
            .policy = CXI_CQ_UPDATE_LOW_FREQ_EMPTY,
            .flags = CXI_CQ_IS_TX | CXI_CQ_TX_ETHERNET,
            .lcid = cp->lcid,
    };


    // End of normal execution path
    return 0;

destroy_cp:
    // !@# Call can fail but we may not care since already in error path
    cxil_destroy_cp(cp);
destroy_lni:
    // !@# Call can fail but we may not care since already in error path
    cxil_destroy_lni(lni);
close_device:
    cxil_close_device(dev);
return_error:
    return rc;
}

static int
cxi_remove(struct rte_vdev_device *dev)
{
    // destroy cp
    // destroy lni
    // close device
    return 0;
}

static struct rte_vdev_driver cxi_pmd = {
    .probe = cxi_probe,
    .remove = cxi_remove,
};

RTE_PMD_REGISTER_VDEV(net_cxi, cxi_pmd);
