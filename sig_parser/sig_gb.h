/*
 * sig_gb.h
 *
 *  Created on: 2014年6月18日
 *      Author: Figoo
 */

#ifndef SIG_GB_H_
#define SIG_GB_H_

#define NS_UNITDATA 0x00	//NS-UNITDATA
#define MAX_HASH        500000
#define TIMEOUT_TLLI    36000   //老化时间为10小时

#define BSSGP_LLC_PDU                   0x0E
#define BSSGP_PDU_LIFETIME				0x16
#define BSSGP_TLLI                         0x1F

#define GPRS_NS_HD_LEN	4			//!< GPRS NS head length
#define BSSGP_HD_LEN	12			//!< BSSGP head length(min)

#define DL_UNITDATA 0x00			//!< BSSGP DL-UNITDATA type
#define UL_UNITDATA 0x01			//!< BSSGP UL-UNITDATA type

#define MIN_BSSGP_LEN	14			//!< BSSGP头最小长度
#define MIN_UP_BSSGP_LEN 20			//!< BSSGP上行数据头最小长度

//! GPRS Network Service结构
typedef struct _ns{
	sb_u8 pdu_type;				//!< 0x00:NS-UNITDATA
	sb_u8 sdu_ctrl;				//!< SDU Control bits
	sb_u16 bvci;				//!< BSSGP Virtual Connection Identifier
} sb_ns_t;

//! IEI结构
typedef struct _iei{
	sb_u8 type;					//!< 类型
	sb_u8 head_len;				//!< 头长度，type+length
	sb_u16 length;				//!< 数据长度
	sb_u8 *value;				//!< 值
} sb_iei_t;

typedef struct _msc{
	sb_u32 gprs_msc;			//!< GPRS Multi Slot Class
	sb_u32 egprs_msc;			//!< EGPRS Multi Slot Class
	sb_u8 umts;					//!< UMTS
} gprs_msc_t;

//! BaseStation SubSystem GPRS Protocol结构
typedef struct _bssgp{
	sb_u8 pdu_type;				//!< PDU类型，0x00:DL-UNITDATA, 0x01:UL-UNITDATA
	sb_u32 tlli;				//!< Temporary logical link Identity
	sb_u32 old_tlli;            //!< Old Temporary logical link Identity

	sb_u32 gprs_msc;			//!< GPRS Multi Slot Class
	sb_u32 egprs_msc;			//!< EGPRS Multi Slot Class
	sb_u8 umts;					//!< UMTS

	sb_u16 mcc;					//!< Mobile country code
	sb_u8 mnc;					//!< Mobile network code
	sb_u16 lac;					//!< Location area code
	sb_u8 rac;					//!< Routing area code
	sb_u16 ci;					//!< Cell Identifier

	sb_u64 imsi;				//!< IMSI

	sb_u8 llc_len;				//!< LLC length

} sb_bssgp_t;

//! LLC Protocol结构
typedef struct _llc{
	sb_u8 sapi;					//!< PDU类型，0x00:GMM, 0x01,0x05,0x09,0x0B:SNDCP
} sb_llc_t;

//! SNDCP Protocol结构
typedef struct _sndcp{
	sb_u8 sn_pdu_type;			//!< 确认模式，0：SN-DATA PDU（确认模式），1：SN-UNITDATA PDU（非确认模式），目前仅分析非确认模式
	sb_u8 nsapi;				//!< NSAPI
	sb_u8 is_first;				//!< 是否是第一个分片包，0：不是，1：是
	sb_u8 is_more;				//!< 是否是最后一个分片包，0：是，1：不是
	sb_u8 is_compress;			//!< 是否压缩，0：不是，1：是
	sb_u32 n_pdu_no;			//!< N-PDU no
	sb_u8 segment_no;                       //!< Segment no
} sb_sndcp_t;

//! GMM结构
typedef struct _gmm{
	sb_u32 tmsi;				//!< TMSI
	sb_u64 imsi;				//!< IMSI
	sb_u64 imei;				//!< IMEI
	sb_u64 msisdn;				//!< 手机号码
	sb_u8 gmm_cause;			//!< GMM cause
	sb_u8 sm_cause;				//!< SM cause

	sb_u8 nsapi;				//!< NSAPI

	sb_u32 user_ip;				//!< User IP address
	sb_u8 apn[40];				//!< APN

	sb_u8 cdr_type;				//!< CDR type
	sb_u8 cdr_subtype;			//!< detail result
	sb_u8 cdr_result;			//!< CDR result
	sb_u16 cdr_d_result;		//!< CDR detail result

	sb_u16 mcc;					//!< Mobile country code
	sb_u8 mnc;					//!< Mobile network code
	sb_u16 lac;					//!< Location area code
	sb_u8 rac;					//!< Routing area code
	sb_u16 ci;					//!< Cell Identifier

	sb_u32 gprs_msc;			//!< GPRS Multi Slot Class
	sb_u32 egprs_msc;			//!< EGPRS Multi Slot Class
	sb_u8 umts;

	sb_u8 qos_r_delay;			//!< QoS Request Delay
	sb_u8 qos_r_reliability;	//!< QoS Request Reliability
	sb_u8 qos_r_precedence;		//!< QoS Request Precedence
	sb_u8 qos_r_peak;			//!< QoS Request Peak
	sb_u8 qos_r_mean;			//!< QoS Request Mean

	sb_u8 qos_n_delay;			//!< QoS Negotiate Delay
	sb_u8 qos_n_reliability;	//!< QoS Negotiate Reliability
	sb_u8 qos_n_precedence;		//!< QoS Negotiate Precedence
	sb_u8 qos_n_peak;			//!< QoS Negotiate Peak
	sb_u8 qos_n_mean;			//!< QoS Negotiate Mean
} sb_gmm_t;

//! gb packet struct
typedef struct _sp_gb{
	sb_ns_t ns;				//!< NS层信息
	sb_bssgp_t bssgp;		//!< BSSGP层信息
	sb_llc_t llc;			//!< LLC层信息
	sb_gmm_t gmm;			//!< GMM层信息
} sp_gb_t;

sb_s32 sp_process_gb(sp_packet_t *p_packet_info, gxio_mpipe_idesc_t *pdesc);

#endif /* SIG_GB_H_ */
