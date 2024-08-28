/**
 * @file uri_def.h
 * @author Guangnan Feng (nobugday@gamil.com)
 * @brief This header include all the URI definitions that should be include in all URI channel modules.
 * @attention   <uri/api/uri_api.h> should NOT be included in this file
 *              because the interfaces in it are implemented by uri/base/[file].c.
 *              The interfaces that each module should implemented are the function pointer defined in the following headers.
 * @version 1.0
 * @date 2023-12-27
 * @copyright Copyright (c) 2023
 */

#include <uru/dev/dev.h>
#include <uru/dev/trace.h>
#include <uru/mpi/mpi.h>
#include <uru/sys/preprocessor.h>
#include <uru/sys/realloc.h>
#include <uru/type/flag.h>
#include <uru/type/queue.h>
#include <uru/type/uru_status.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * @defgroup URI_DEF Unified RMA API (URI) Definitions
 * @brief This section include all URI API.
 */

/**
 * @defgroup URI_CHANNEL URI channel definitions.
 * @ingroup URI_DEF
 * @brief This section contains URI channel definitions.
 */

/**
 * @defgroup URI_MEM URI Memory Management definitions.
 * @ingroup URI_DEF
 * @brief This section contains URI Memory Management definitions.
 */

/**
 * @defgroup URI_RDMA URI RDMA write operations definitions.
 * @ingroup URI_DEF
 * @brief This section contains URI RDMA write operations definitions.
 */

#ifndef URI_MAX_CHANNEL_NUM
    /**
     * @ingroup URI_CHANNEL
     * @brief All the attributions of channel are in `uri_channel_t`
     */
    #define URI_MAX_CHANNEL_NUM 16
#endif

/**
 * @ingroup URI_CHANNEL
 * @brief All the attributions of channel are in \ref uri_channel_s
 */
typedef struct uri_channel_s uri_channel_t;

/**
 * @ingroup URI_MEM
 * @brief All the attributions of channel are in \ref uri_mem_s
 */
typedef struct uri_mem_s uri_mem_t;

/**
 * @ingroup URI_RDMA
 * @brief All the attributions of channel are in \ref uri_plan_s
 */
typedef struct uri_plan_s uri_plan_t;

/**
 * @ingroup URI_CHANNEL
 * @brief Open the channel (device). It will call the channel corresponding function.
 */
typedef uru_status_t (*uri_channel_open_func_p)(uri_channel_t* channel_p);

/**
 * @ingroup URI_CHANNEL
 * @brief Run the channel daemon once. It will call the channel corresponding function.
 */
typedef uru_status_t (*uri_channel_daemon_func_p)(uri_channel_t* channel_p);

/**
 * @ingroup URI_CHANNEL
 * @brief Close the channel (device). It will call the channel corresponding function.
 */
typedef uru_status_t (*uri_channel_close_func_p)(uri_channel_t* channel_p);

/**
 * @ingroup URI_MEM
 * @brief Get the real size of uri_mem_h. It will call the channel corresponding function.
 */
typedef uru_status_t (*uri_mem_h_size_func_p)(uri_channel_t* channel_p, size_t* size);

/**
 * @ingroup URI_MEM
 * @brief Register a memory region in a channel. It will call the channel corresponding function.
 *  * @details The address and size of registered memory region should get from \ref uri_mem_h
 */
typedef uru_status_t (*uri_mem_reg_func_p)(uri_channel_t* channel_p, void* mem_addr, size_t mem_size, uri_mem_t* mem_p);

/**
 * @ingroup URI_RDMA
 * @brief Create an RDMA write plan on a specific channel. It will call the channel corresponding function.
 */
typedef uru_status_t (*uri_rdma_plan_create_func_p)(
    uri_channel_t* channel_p, uint8_t once_now, size_t num_wrt, uru_transfer_t* dma_type_arr,
    uri_mem_t** loc_mem_pp, size_t* loc_offset_arr, uru_flag_t** loc_flag_p_arr, int64_t* loc_flag_add_arr, uint64_t* dma_size_arr,
    uri_mem_t** rmt_mem_pp, size_t* rmt_offset_arr, uru_flag_t** rmt_flag_p_arr, int64_t* rmt_flag_add_arr,
    uri_plan_t** plan_pp);

/**
 * @ingroup URI_RDMA
 * @brief Start the RDMA write plan. It will call the channel corresponding function.
 */
typedef uru_status_t (*uri_rdma_plan_start_func_p)(uri_channel_t* channel_p, uri_plan_t* plan_p);

/**
 * @ingroup URI_RDMA
 * @brief Destroy the RDMA write plan. It will call the channel corresponding function.
 */
typedef uru_status_t (*uri_rdma_plan_destroy_func_p)(uri_channel_t* channel_p, uri_plan_t* plan_p);

/**
 * @ingroup URI_CHANNEL
 * @brief Contains all the function pointers that the URI channel needs.
 */
typedef struct {
    uri_channel_open_func_p channel_open;           /**< Pointer to corresponding function in the channel */
    uri_channel_daemon_func_p channel_daemon;       /**< Pointer to corresponding function in the channel */
    uri_channel_close_func_p channel_close;         /**< Pointer to corresponding function in the channel */
    uri_mem_h_size_func_p mem_h_size;               /**< Pointer to corresponding function in the channel */
    uri_mem_reg_func_p mem_reg;                     /**< Pointer to corresponding function in the channel */
    uri_rdma_plan_create_func_p rdma_plan_create;   /**< Pointer to corresponding function in the channel */
    uri_rdma_plan_start_func_p rdma_plan_start;     /**< Pointer to corresponding function in the channel */
    uri_rdma_plan_destroy_func_p rdma_plan_destroy; /**< Pointer to corresponding function in the channel */
} uri_channel_func_ptrs_t;

/**
 * @ingroup URI_CHANNEL
 * @brief All the general attributions of channel.
 */
struct uri_channel_s {
    struct {
        char* name;                     /**< Name of the channel */
        const char* type;               /**< Type of the channel */
        char* description;              /**< Description of the channel */
        size_t blocksize;               /**< Best Data Block size of the channel */
        int channel_id;                 /**< The channel ID that current channel use*/
    } attr;                             /**< Attributes of the channel, it should be the same as \ref uri_channel_attr_t */
    int num_devices;                    /**< Total number of the devices, which means the number of channels that use the same type of the device of this channel. */
    int device_id;                      /**< The device ID that current channel use*/
    char opened;                        /**< Whether the channel is opened */
    size_t mem_h_offset;                /**< The offset of the \ref uri_mem_h in \ref unr_mem_h */
    uri_channel_func_ptrs_t* func_ptrs; /**< Function pointers */
    void* info;                         /**< Pointer for channel module to store specific data */
};

/**
 * @ingroup URI_MEM
 * @brief All the general attributions of registered memory region.
 */
struct uri_mem_s {
    /* Reserved for some common attributes of for uri_mem_t */
    /* Space for uri_[xxx]_mem_t */
};

/**
 * @ingroup URI_RDMA
 * @brief All the general attributions of RDMA write plan.
 */
struct uri_plan_s {
    /* Reserved for some common attributes of for uri_mem_t */
    /* Space for uri_[xxx]_plan_t */
};

/**
 * @ingroup URI_MEM
 * @brief Get the number of devices.
 * @details Each channel module should implement this function.
 * @return The number of devices.
 */

/**
 * @ingroup URI_MEM
 * @brief
 * @param channel_p
 * @param mem_p
 * @param addr
 * @param size
 * @param rank
 */
uru_status_t uri_mem_get_info(uri_channel_t* channel_p, uri_mem_t* mem_p, void** addr, size_t* size, int32_t* rank);

/**
 * @ingroup URI_CHANNEL
 * @brief Register channels as constructors.
 * @details Each channel module should call this macro to register available channel.
 * @param[in] get_num_devices   The function to get the number of devices.
 *                              The function has no input parameters and returns the number of devices.
 * @param[in] init  The function to initialize the channel.
 *                  The function should have three input parameters:
 *                  ( \ref uri_channel_t* channel_p, int device_id, int num_devices )
 *                  The function should fill in channel_p->channel_p and channel_p->func_ptrs.
 */
#define URI_CHANNEL_REGISTER(get_num_devices, init)                                                \
    extern uri_channel_t* uri_channel_arr[URI_MAX_CHANNEL_NUM];                                    \
    extern int uri_num_channels;                                                                   \
    URU_STATIC_INIT {                                                                              \
        int num_devices = get_num_devices();                                                       \
        for (int i = 0; i < num_devices; i++) {                                                    \
            uri_channel_arr[uri_num_channels] = (uri_channel_t*)uru_malloc(sizeof(uri_channel_t)); \
            memset(uri_channel_arr[uri_num_channels], 0, sizeof(uri_channel_t));                   \
            init(uri_channel_arr[uri_num_channels], i, num_devices);                               \
            uri_num_channels++;                                                                    \
        }                                                                                          \
    }

/**
 * @ingroup URI_DEF
 * @brief Indicate uri.h is included.
 */
#define URI_DEF_H

#ifdef URI_API_H
    #ifndef URI_API_AND_URI_DEF_EXCEPTION
        #error "You should ONLY include uri_def.h and NOT uri.h"
    #endif
#endif