/**
 * @file uri.h
 * @author Guangnan Feng (nobugday@gamil.com)
 * @brief This file defines Unified RMA API (URI).
 * @version 1.0
 * @date 2023-12-24
 * @copyright Copyright (c) 2023
 */

#pragma once

#include <uru/type/uru_status.h>
#include <uru/type/flag.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * @defgroup URI_API Unified RMA API (URI)
 * @ingroup URI_DEF
 * @brief   This section contains Unified RMA API (URI).
 *          URI API will be used by UNR implementation.
 */

/**
 * @ingroup URI_API
 * @brief Indicate uri.h is included.
 */
#define URI_API_H

/**
 * @ingroup URI_API
 * @brief  Abbreviation for "URI Channel Handle".
 */
typedef void* uri_channel_h;

/**
 * @ingroup URI_API
 * @brief Abbreviation for "URI Memory Handle". */
typedef void* uri_mem_h;

/**
 * @ingroup URI_API
 * @brief Abbreviation for "URI Plan Handle". */
typedef void* uri_plan_h;

/**
 * @ingroup URI_API
 * @brief Used to describe the channel attribute. Use \ref uri_channel_attr_get to get the attribution.
 */
typedef struct uri_channel_attr_s {
    const char* name;        /**< Name of the channel */
    const char* type;        /**< Type of the channel */
    const char* description; /**< Description of the channel */
    size_t blocksize;        /**< Best Data Block size of the channel */
    int channel_id;          /**< The channel ID that current channel use*/
} uri_channel_attr_t;

/**
 * @ingroup URI_API
 * @brief Query for list of available channels.
 * @param channel_h_arr All the available channels.
 * @param num_channels  Number of available channels.
 */
uru_status_t uri_query_channel_list(uri_channel_h** channel_h_pp, int* num_channels);

/**
 * @ingroup URI_API
 * @brief Get the channel information string.
 * @param[in] channel_h The channel to get information.
 * @param[out] attr     The attribution got from `channel_h`.
 */
uru_status_t uri_channel_attr_get(uri_channel_h channel_h, uri_channel_attr_t** attr_pp);

/**
 * @ingroup URI_API
 * @brief Open the channel (device).
 * @param[in] channel_h The channel to open.
 */
uru_status_t uri_channel_open(uri_channel_h channel_h, size_t mem_h_offset);

/**
 * @ingroup URI_API
 * @brief Run the channel daemon once.
 * @details This function will be called in an endless loop in UNR.
 *          Thus, it should not block the process.
 *          The daemon will only be called after \ref uri_channel_open.
 * @param channel_h 
 * @return uru_status_t 
 */
uru_status_t uri_channel_daemon(uri_channel_h channel_h);

/**
 * @ingroup URI_API
 * @brief Close the channel (device).
 * @param[in] channel_h The channel to close.
 */
uru_status_t uri_channel_close(uri_channel_h channel_h);

/**
 * @ingroup URI_API
 * @brief Get the real size of uri_mem_h.
 * @param[in] channel_h The channel to query the real size of uri_mem_h
 * @param[out] size     The real size of uri_mem_h. 
 * @return uru_status_t 
 */
uru_status_t uri_mem_h_size(uri_channel_h channel_h, size_t* size);

/**
 * @ingroup URI_API
 * @brief Register a memory region that can be used by RDMA send and receive.
 * @param[in] channel_h The channel to register.
 * @param[out] mem_h    The handle of the memory region.
 *                      The local process use it to check whether the data is received.
 */
uru_status_t uri_mem_reg(uri_channel_h channel_h, void* mem_addr, size_t mem_size, uri_mem_h mem_h);

/**
 * @brief   Create an RDMA write plan on a specific channel 
 *          and post a local event in sender process and a remote event in receiver process.
 * @param channel_h         The channel to create the plan.
 * @param num_wrt           Number of RDMA writes.
 * @param loc_mem_h_arr     An array of the local memory handle.
 * @param loc_offset_arr    An array of the address offset in the local memory handle.
 * @param loc_flag_p_arr    An array of uru_flag_t pointers, indicates the local flags will be added after sending.
 * @param loc_flag_add_arr  An array of numbers that will be atomic added to the local uru_flag_t after sending.
 * @param wrt_size_arr      An array of the size of data to be written in each RDMA write operation.
 * @param rmt_mem_h_arr     An array of the remote memory handle.
 * @param rmt_offset_arr    An array of the address offset in the remote memory handle.
 * @param rmt_flag_p_arr    An array of uru_flag_t pointers, indicates the remote flags will be added after received.
 * @param rmt_flag_add_arr  An array of numbers that will be atomic added to the remote uru_flag_t after received.
 * @param plan_h            The handle of the plan.
 * @return uru_status_t 
 */
uru_status_t uri_rdma_plan_create(
    uri_channel_h channel_h, uint8_t once_now, size_t num_wrt, uru_transfer_t* dma_type_arr,
    uri_mem_h* loc_mem_h_arr, size_t* loc_offset_arr, uru_flag_t** loc_flag_p_arr, int64_t* loc_flag_add_arr, uint64_t* dma_size_arr,
    uri_mem_h* rmt_mem_h_arr, size_t* rmt_offset_arr, uru_flag_t** rmt_flag_p_arr, int64_t* rmt_flag_add_arr,
    uri_plan_h* plan_h);

/**
 * @ingroup URI_API
 * @brief Start the RDMA write plan. It will commit the RDMA write operations to the NIC.
 * @param channel_h The channel to start the plan. It has to be the same channel as the plan create.
 * @param plan_h    The plan to start. It should be create by \ref uri_wrt_plan_create.
 * @return uru_status_t 
 */
uru_status_t uri_rdma_plan_start(uri_channel_h channel_h, uri_plan_h plan_h);

/**
 * @ingroup URI_API
 * @brief Destroy the RDMA write plan.
 * @param channel_h The channel to destroy the plan.
 * @param plan_h    The plan to destroy. It should be create by \ref uri_wrt_plan_create.
 * @return uru_status_t 
 */
uru_status_t uri_rdma_plan_destroy(uri_channel_h channel_h, uri_plan_h plan_h);

#ifdef URI_DEF_H
    #error "You should ONLY include \ref uri_def.h and NOT \ref uri.h"
#endif