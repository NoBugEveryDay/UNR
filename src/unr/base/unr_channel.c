#include <unr/base/unr_def.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

unr_channels_t* unr_channels = NULL;
MPI_Datatype MPI_UNR_MEM_H;
MPI_Datatype MPI_UNR_BLK_H;
MPI_Datatype MPI_UNR_SIG_H;

uri_channel_h find_channel(char* name) {
    uri_channel_h* uri_channel_arr;
    int uri_num_channels;
    uri_query_channel_list(&uri_channel_arr, &uri_num_channels);

    for (int i = 0; i < uri_num_channels; i++) {
        uri_channel_attr_t* attr;
        uri_channel_attr_get(uri_channel_arr[i], &attr);

        if (strcmp(attr->name, name) == 0) {
            return uri_channel_arr[i];
        }
    }

    return NULL;
}

pthread_t daemon_thread_h;
volatile char daemon_thread_continue = 1;
volatile char daemon_thread_running = 1;
void* unr_daemon_thread(void* arg) {
    while (daemon_thread_continue) {
        for (int i = 0; i < unr_channels->num_avail_channels; i++) {
            uri_channel_h channel = unr_channels->avail_channels[i];
            uri_channel_daemon(channel);
        }
        usleep(1); /* Without it, the program may be stalled in some cases. */
    }
    daemon_thread_running = 0;
    return NULL;
}

uru_status_t unr_init() {

    static int unr_initialized = 0;
    if (unr_initialized) {
        return URU_STATUS_OK;
    }
    unr_initialized = 1;

    { /* Check whether MPI is initialized and initialize uru_mpi */
        int flag;
        MPI_Initialized(&flag);
        URU_ASSERT_REASON(flag, "MPI should be initialized after calling unr_init");
        uru_mpi_init();
    }

    uri_channel_h* uri_channel_arr;
    int uri_num_channels;
    uri_query_channel_list(&uri_channel_arr, &uri_num_channels);
    unr_channels = (unr_channels_t*)uru_calloc(1, sizeof(unr_channels_t) + 3 * uri_num_channels * sizeof(uri_channel_h));
    unr_channels->avail_channels = (uri_channel_h*)(unr_channels + 1);
    unr_channels->inter_channels = unr_channels->avail_channels + uri_num_channels;
    unr_channels->intra_channels = unr_channels->inter_channels + uri_num_channels;

    { /* Get channels that user selected to use from environment variables */
        for (int k = 1; k >= 0; k--) {
            char* env_name = k ? "UNR_INTER" : "UNR_INTRA";
            char* env = getenv(env_name);
            uri_channel_h* channels = k ? unr_channels->inter_channels : unr_channels->intra_channels;
            int* num_channels = k ? &(unr_channels->num_inter_channels) : &(unr_channels->num_intra_channels);

            if (env == NULL) {
                *num_channels = 1;
                channels[0] = find_channel("mpi");
                URU_ASSERT_REASON(channels[0], "`%s` not set, and no default MPI channel is available", env_name);
            } else {
                /* Split env by `,` */
                char* token = strtok(env, ",");
                while (token != NULL) {
                    *num_channels += 1;
                    channels[*num_channels - 1] = find_channel(token);
                    URU_ASSERT_REASON(channels[*num_channels - 1] != NULL, "Cannot find channel %s", token);
                    token = strtok(NULL, ",");
                }
                URU_ASSERT_REASON(*num_channels > 0, "No %s channel is available", env_name)
            }
        }
    }

#ifdef URU_ENABLE_LOG
    for (int k = 1; k >= 0; k--) {
        uri_channel_h* channels = k ? unr_channels->inter_channels : unr_channels->intra_channels;
        int num_channels = k ? unr_channels->num_inter_channels : unr_channels->num_intra_channels;
        printf(k ? "Inter channels: " : "Intra channels: ");
        for (int i = 0; i < num_channels; i++) {
            uri_channel_attr_t* attr;
            uri_channel_attr_get(channels[i], &attr);
            if (i) {
                printf(",");
            }
            printf("%s", attr->name);
            fflush(stdout);
        }
        printf("\n");
    }
#endif

    /* Check duplicate channel. It must be done BEFORE open channels */
    for (int k = 1; k >= 0; k--) {
        uri_channel_h* channels = k ? unr_channels->inter_channels : unr_channels->intra_channels;
        int num_channels = k ? unr_channels->num_inter_channels : unr_channels->num_intra_channels;

        for (int i = 1; i < num_channels; i++) {
            /* Check duplicate channel */
            for (int j = 0; j < i; j++) {
                if (channels[i] == channels[j]) {
                    uri_channel_attr_t* attr;
                    uri_channel_attr_get(channels[i], &attr);
                    URU_ASSERT_REASON(0, "Duplicate inter channel [%s]", attr->name);
                }
            }
        }
    }

    { /* Combine inter_channels and intra_channels to get avail_channels and generate channel_id */
        for (int i = 0; i < unr_channels->num_inter_channels; i++) {
            unr_channels->avail_channels[unr_channels->num_avail_channels] = unr_channels->inter_channels[i];
            unr_channels->num_avail_channels++;
        }
        unr_channels->intra_channel_reuse = (int*)uru_malloc(sizeof(int) * unr_channels->num_intra_channels);
        for (int i = 0; i < unr_channels->num_intra_channels; i++) {
            unr_channels->intra_channel_reuse[i] = -1;
            for (int j = 0; j < unr_channels->num_inter_channels; j++) {
                if (unr_channels->intra_channels[i] == unr_channels->inter_channels[j]) {
                    unr_channels->intra_channel_reuse[i] = j;
                    break;
                }
            }
            if (unr_channels->intra_channel_reuse[i] == -1) {
                unr_channels->avail_channels[unr_channels->num_avail_channels] = unr_channels->intra_channels[i];
                unr_channels->num_avail_channels++;
            }
        }
        for (int i = 0; i < unr_channels->num_avail_channels; i++) {
            uri_channel_attr_t* attr;
            uri_channel_attr_get(unr_channels->avail_channels[i], &attr);
            attr->channel_id = i;
        }
    }

    /* Initialization for unr_mem_t and open channel */
    unr_channels->unr_mem_t_size = sizeof(unr_mem_t);
    unr_channels->inter_uri_mem_h_offset = (size_t*)uru_malloc(sizeof(size_t) * unr_channels->num_inter_channels);
    unr_channels->intra_uri_mem_h_offset = (size_t*)uru_malloc(sizeof(size_t) * unr_channels->num_intra_channels);
    for (int k = 1; k >= 0; k--) {
        uri_channel_h* channels = k ? unr_channels->inter_channels : unr_channels->intra_channels;
        int num_channels = k ? unr_channels->num_inter_channels : unr_channels->num_intra_channels;
        size_t* uri_mem_h_offset = k ? unr_channels->inter_uri_mem_h_offset : unr_channels->intra_uri_mem_h_offset;
        for (int i = 0; i < num_channels; i++) {
            if (k || unr_channels->intra_channel_reuse[i] == -1) {
                uri_mem_h_offset[i] = unr_channels->unr_mem_t_size;
                uri_channel_open(channels[i], unr_channels->unr_mem_t_size); /* Open Channel at here */
                size_t tmp;
                uri_mem_h_size(channels[i], &tmp);
                unr_channels->unr_mem_t_size += tmp;
            } else {
                uri_mem_h_offset[i] = unr_channels->inter_uri_mem_h_offset[unr_channels->intra_channel_reuse[i]];
            }
        }
    }
    unr_channels->loc_mem_arr_len = 0;
    unr_channels->rmt_mem_arr_len = (int*)uru_calloc(uru_mpi->size, sizeof(int));
    unr_channels->loc_mem_arr = NULL;
    unr_channels->rmt_mem_arr = (uri_mem_h**)uru_calloc(uru_mpi->size, sizeof(uri_mem_h*));

    /* Some checks. Some checks must be done AFTER open channels */
    unr_channels->inter_channel_blk_size = (size_t*)uru_malloc(sizeof(size_t) * unr_channels->num_inter_channels);
    unr_channels->intra_channel_blk_size = (size_t*)uru_malloc(sizeof(size_t) * unr_channels->num_intra_channels);
    for (int k = 1; k >= 0; k--) {
        uri_channel_h* channels = k ? unr_channels->inter_channels : unr_channels->intra_channels;
        int num_channels = k ? unr_channels->num_inter_channels : unr_channels->num_intra_channels;
        size_t* channel_blk_size = k ? unr_channels->inter_channel_blk_size : unr_channels->intra_channel_blk_size;
        size_t* channel_blk_size_sum = k ? (&unr_channels->inter_channel_blk_size_sum) : (&unr_channels->intra_channel_blk_size_sum);
        /* Check: channel blocksize should be power of 2 */
        for (int i = 0; i < num_channels; i++) {
            uri_channel_attr_t* attr;
            uri_channel_attr_get(channels[i], &attr);
            channel_blk_size[i] = attr->blocksize;
            *channel_blk_size_sum += attr->blocksize;
            URU_ASSERT_REASON((attr->blocksize > 0) && ((attr->blocksize & (attr->blocksize - 1)) == 0),
                              "Invalid %s channel blocksize %s:%zu", k ? "inter-node" : "intra-node", attr->name, attr->blocksize);
        }

        /* Limit by current implementation in get_unr_blk_div() */
        uri_channel_attr_t* attr_0;
        uri_channel_attr_get(channels[0], &attr_0);
        for (int i = 1; i < num_channels; i++) {

            /* Check: blocksize of all the inter/intra-node channel should be the same */
            uri_channel_attr_t* attr_i;
            uri_channel_attr_get(channels[i], &attr_i);
            URU_ASSERT_REASON(attr_i->blocksize == attr_0->blocksize, "Different inter-node channel blocksize %s:%zu and %s:%zu is not support yet",
                              attr_i->name, attr_i->blocksize, attr_0->name, attr_0->blocksize);

            /* Check: The type of all the inter/intra-node channel should be the same */
            URU_ASSERT_REASON(strcmp(attr_i->type, attr_0->type) == 0, "Different inter-node channel type [%s] and [%s]", attr_i->name, attr_0->name);
        }
    }

    MPI_Type_contiguous(unr_channels->unr_mem_t_size, MPI_BYTE, &MPI_UNR_MEM_H);
    MPI_Type_commit(&MPI_UNR_MEM_H);
    URU_ASSERT(sizeof(unr_blk_h) == sizeof(unr_blk_t));
    MPI_Type_contiguous(sizeof(unr_blk_h), MPI_BYTE, &MPI_UNR_BLK_H);
    MPI_Type_commit(&MPI_UNR_BLK_H);
    URU_ASSERT(sizeof(unr_sig_h) == sizeof(unr_sig_t));
    MPI_Type_contiguous(sizeof(unr_sig_h), MPI_BYTE, &MPI_UNR_SIG_H);
    MPI_Type_commit(&MPI_UNR_SIG_H);
    URU_ASSERT(sizeof(uru_queue_t) == 24);

    /* Create daemon thread */
    URU_ASSERT(pthread_create(&daemon_thread_h, NULL, unr_daemon_thread, NULL) == 0);

    return URU_STATUS_OK;
}

uru_status_t unr_finalize() {

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);

    daemon_thread_continue = 0;
    while (daemon_thread_running) {
        /* Do nothing and wait */
    };

    { /* Check whether MPI is finalized */
        int flag;
        MPI_Finalized(&flag);
        URU_ASSERT_REASON(flag == 0, "MPI should be finalized AFTER calling unr_finalize");
    }

    MPI_Type_free(&MPI_UNR_MEM_H);
    MPI_Type_free(&MPI_UNR_BLK_H);
    MPI_Type_free(&MPI_UNR_SIG_H);

    { /* Close Channel */
        for (int i = 0; i < unr_channels->num_avail_channels; i++) {
            uri_channel_close(unr_channels->avail_channels[i]);
        }
    }

    return URU_STATUS_OK;
}