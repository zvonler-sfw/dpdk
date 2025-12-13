# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2025 Hewlett Packard Enterprise Development LP

if not is_linux
    build = false
    reason = 'only supported on Linux' # !@# 64-bit only?
    subdir_done()
endif

# Try to locate libcxi - only build if available
#libcxi_dep = dependency('libcxi', required: false)
#if libcxi_dep.found()
    #libcxi_dep = cc.find_library('cxi', required: false)
#endif
#if not libcxi_dep.found() or not cc.links(min_c_code, depdendencies: libcxi_dep)
    #build = false
    #reason = 'missing dependency, "libcxi"'
    #subdir_done()
#endif
#ext_deps += libcxi_dep

deps + ['ethdev']

headers = files('rte_cxi_pmd.h')
sources = files(
    'cxi_eth.c',
    'cxi_pmd.c',
    'cxi_txrx.c',
)

# !@# Can we set this?
# require_iova_in_mbuf = false

#testpmd_sources += files('cxi_testpmd.c')
