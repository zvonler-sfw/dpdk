/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2025 Hewlett Packard Enterprise Development LP
 */

#ifndef _RTE_CXI_PMD_H_
#define _RTE_CXI_PMD_H_

#include <rte_log.h>

extern int cxi_pmd_logtype;
#define RTE_LOGTYPE_CXI_PMD cxi_pmd_logtype

#define PMD_LOG(level, ...) \
	RTE_LOG_LINE_PREFIX(level, CXI_PMD, "%s(): ", __func__, __VA_ARGS__)

#endif /* _RTE_CXI_PMD_H_ */
