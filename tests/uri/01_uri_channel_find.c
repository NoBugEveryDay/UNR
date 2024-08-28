#include <uri/api/uri.h>
#include <uru/dev/trace.h>
#include <uru/mpi/mpi.h>
#include <mpi.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    uru_mpi_init();

    uri_channel_h* uri_channel_arr;
    int uri_num_channels;
    uri_query_channel_list(&uri_channel_arr, &uri_num_channels);

    int find_channel_mpi = 0;
#ifdef URI_CHANNEL_GLEX
    int find_channel_glex = 0;
#endif
#ifdef URI_CHANNEL_VERBS
    int find_channel_verbs = 0;
#endif

    for (int i = 0; i < uri_num_channels; i++) {
        uri_channel_attr_t* attr;
        uri_channel_attr_get(uri_channel_arr[i], &attr);

        if (strcmp(attr->type, "mpi") == 0) {
            find_channel_mpi = 1;
        }

#ifdef URI_CHANNEL_GLEX
        if (strcmp(attr->type, "glex") == 0) {
            find_channel_glex = 1;
        }
#endif

#ifdef URI_CHANNEL_VERBS
        if (strcmp(attr->type, "verbs") == 0) {
            find_channel_verbs = 1;
        }
#endif
    }

    if (!find_channel_mpi) {
        printf("ERROR: No MPI channel is available!\n");
        return 1;
    }
    printf("Find MPI channel!\n");

#ifdef URI_CHANNEL_GLEX
    if (!find_channel_glex) {
        printf("ERROR: No GLEX channel is available!\n");
        return 1;
    }
    printf("Find GLEX channel!\n");
#endif

#ifdef URI_CHANNEL_VERBS
    if (!find_channel_verbs) {
        printf("ERROR: No VERBS channel is available!\n");
        return 1;
    }
    printf("Find VERBS channel!\n");
#endif

    MPI_Finalize();
    return 0;
}