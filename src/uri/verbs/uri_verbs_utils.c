#include <uri/verbs/uri_verbs.h>

/******************************************************************************
 * *	Function: modify_qp_to_init
 * *
 * *	Input
 * *	qp	QP to transition
 * *
 * *	Output
 * *	none
 * *
 * *	Returns
 * *	0 on success, ibv_modify_qp failure code on failure
 * *
 * *	Description
 * *	Transition a QP from the RESET to INIT state
 * ******************************************************************************/
void modify_qp_to_init(struct ibv_qp* qp) {
    struct ibv_qp_attr attr;
    int attr_mask;
    int rc;

    memset(&attr, 0, sizeof(attr));

    attr.qp_state = IBV_QPS_INIT;
    attr.port_num = VERBS_PORT_NUM;
    attr.qp_access_flags = IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
    attr_mask = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;

    rc = ibv_modify_qp(qp, &attr, attr_mask);
    if (rc) {
        printf("failed to modify QP state to INIT (%s)\n", strerror(errno));
        fflush(stdout);
        exit(URU_ERROR_EXIT);
    }
}

/******************************************************************************
 * *	Function: modify_qp_to_rtr
 * *
 * *	Input
 * *	qp		QP to transition
 * *	remote_qpn	remote QP number
 * *	dlid		destination LID
 * *	dgid		destination GID (mandatory for RoCEE)
 * *
 * *	Output
 * *	none
 * *
 * *	Returns
 * *	0 on success, ibv_modify_qp failure code on failure
 * *
 * *	Description
 * *	Transition a QP from the INIT to RTR state, using the specified QP number
 * ******************************************************************************/
void modify_qp_to_rtr(struct ibv_qp* qp, uint32_t remote_qpn, uint16_t dlid, union ibv_gid* dgid, int gid_idx, enum ibv_mtu active_mtu) {
    struct ibv_qp_attr attr;
    int attr_mask;
    int rc;

    memset(&attr, 0, sizeof(attr));

    attr.qp_state = IBV_QPS_RTR;
    attr.path_mtu = active_mtu;
    attr.dest_qp_num = remote_qpn;
    attr.max_dest_rd_atomic = 16;
    attr.min_rnr_timer = 0x12;

    attr.ah_attr.is_global = 0;
    attr.ah_attr.dlid = dlid;
    // attr.ah_attr.sl = 0;
    attr.ah_attr.port_num = VERBS_PORT_NUM;
    attr_mask = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN | IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;
    if (dlid == 0) {
        attr.ah_attr.is_global = 1;
        memcpy(&attr.ah_attr.grh.dgid, dgid, 16);
        attr.ah_attr.grh.flow_label = 3;
        attr.ah_attr.grh.hop_limit = 64;
        attr.ah_attr.grh.sgid_index = gid_idx;
        attr.ah_attr.grh.traffic_class = 106;
    }

    rc = ibv_modify_qp(qp, &attr, attr_mask);
    if (rc) {
        printf("failed to modify QP state to RTR (%s)\n", strerror(errno));
        fflush(stdout);
        exit(URU_ERROR_EXIT);
    }
}

/******************************************************************************
 * *	Function: modify_qp_to_rts
 * *
 * *	Input
 * *	qp	QP to transition
 * *
 * *	Output
 * *	none
 * *
 * *	Returns
 * *	0 on success, ibv_modify_qp failure code on failure
 * *
 * *	Description
 * *	Transition a QP from the RTR to RTS state
 * ******************************************************************************/
void modify_qp_to_rts(struct ibv_qp* qp) {
    struct ibv_qp_attr attr;
    int attr_mask;
    int rc;

    memset(&attr, 0, sizeof(attr));

    attr.qp_state = IBV_QPS_RTS;
    attr.timeout = 0x12;
    attr.retry_cnt = 6;
    attr.rnr_retry = 0;
    attr.max_rd_atomic = 16;

    attr_mask = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;

    rc = ibv_modify_qp(qp, &attr, attr_mask);
    if (rc) {
        printf("failed to modify QP state to RTS (%s)\n", strerror(errno));
        fflush(stdout);
        exit(URU_ERROR_EXIT);
    }
}