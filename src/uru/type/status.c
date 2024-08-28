#include "uru_status.h"

char* uru_status_desc_str[URU_STATUS_NUM] = {
    URU_STATUS_PACK(URU_STATUS_PACK_SELECT_DESC),
};

const char* uru_status_string(uru_status_t status) {
    if (status < 0 || status >= URU_STATUS_NUM) {
        return "Unknown error";
    }
    return uru_status_desc_str[status];
}

uru_transfer_t* URU_TRANSFER_TYPE_DMA_PUT_ALL = (uru_transfer_t*)3;
uru_transfer_t* URU_TRANSFER_TYPE_DMA_GET_ALL = (uru_transfer_t*)4;