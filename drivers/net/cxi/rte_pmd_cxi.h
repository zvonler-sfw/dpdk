/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2025 Hewlett Packard Enterprise Development LP
 */

#ifndef _RTE_PMD_CXI_H_
#define _RTE_PMD_CXI_H_

RTE_LOG_REGISTER_DEFAULT(cxi_pmd_logtype, NOTICE);
#define RTE_LOGTYPE_CXI_PMD cxi_pmd_logtype

#define PMD_LOG(level, ...) \
	RTE_LOG_LINE_PREFIX(level, CXI_PMD, "%s(): ", __func__, __VA_ARGS__)

#endif /* _RTE_PMD_CXI_H_ */
