/*
 * base_lib_pack.c
 *
 *  Created on: 2014年6月3日
 *      Author: Figoo
 */

#include "sb_base_pack.h"

//dmac array that we wang to set when send packet before
static const sb_u8 mac[6]=DMAC_FOR_SET;

sb_s32 sb_send_packet(gxio_mpipe_equeue_t* equeue, gxio_mpipe_idesc_t* idesc)
{
        if(equeue == NULL || idesc == NULL) return ARGS_ERROR;

        // a structure of edesc for send to eDMA or egerss
        gxio_mpipe_edesc_t edesc;

        // set edesc structuer useing some paramters in idesc  structure
        gxio_mpipe_edesc_copy_idesc(&edesc,idesc);

        //get data start
/*        void* start = gxio_mpipe_idesc_get_l2_start(idesc);
        if(start == NULL ) return DATA_START_ERROR;

        // get data length
        size_t length = gxio_mpipe_idesc_get_l2_length(idesc);
        if(length == 0) return DATA_LENGTH_ERROR;

        // read length data to cache from start by fast
        tmc_mem_prefetch(start,length);

        //set dmac to us want
        memcpy(start,mac,6);
        __insn_mf();*/
        edesc.hwb = 0;
        // put edesc to egerss buffer and send to ePket buffer by hardware
        sb_s32 ret = gxio_mpipe_equeue_put(equeue,edesc);
        if(ret !=0 )return DATA_SEND_ERROR;

//        gxio_mpipe_iqueue_drop(iqueue,idesc);

        return DATA_SEND_OK;
}

