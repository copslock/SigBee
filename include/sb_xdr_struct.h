/*
 * sb_xdr_struct.h
 *
 *  Created on: 2014年5月29日
 *      Author: Figoo
 */

#ifndef SB_XDR_STRUCT_H_
#define SB_XDR_STRUCT_H_

#include "sb_type.h"
#include "sb_defines.h"

//! XDR类型
#define XDR_ATTACH	0
#define XDR_IDENTITY	1
#define XDR_AUTH	2
#define XDR_DETACH	3
#define XDR_PDPACT	4
#define XDR_PDPDEACT	5
#define XDR_PDPMOD	6
#define XDR_RAB	7
#define XDR_RAU	8
#define XDR_BSSGP	9
#define XDR_RANAP	10
#define XDR_RELOCATION	11
#define XDR_SERVICEREQ	12
#define XDR_2GPAGING	13
#define XDR_OTHER	14

//! XDR流程ID
#define PROC_ATTACH	0
#define PROC_MS_DETACH	1
#define PROC_NET_DETACH	2
#define PROC_RAU	3
#define PROC_MS_PDPACT	4
#define PROC_NET_PDPACT	5
#define PROC_MS_PDPACT_SEC	6
#define PROC_NET_PDPACT_SEC	7
#define PROC_MS_PDPMOD	8
#define PROC_NET_PDPMOD	9
#define PROC_MS_PDPDEACT	10
#define PROC_NET_PDPDEACT	11
#define PROC_MS_SERVICEREQ	12
#define PROC_NET_SERVICEREQ	13
#define PROC_RELOCATION	14
#define PROC_RANAP	15
#define PROC_BSSGP	16
#define PROC_2GPAGING	17

#define PROC_COMM	100
#define PROC_MMS	101
#define PROC_DNS	102
#define PROC_HTTP	103
#define PROC_FTP	104
#define PROC_EMAIL	105
#define PROC_VOIP	106
#define PROC_RTSP	107
#define PROC_IM	108
#define PROC_P2P	109

#define PROC_OTHER	255

#pragma pack(1)
//! XDR头
typedef struct _xdr_header_tag
{
	sb_u16 len;			//!< CDR/TDR总长度，长度包含数据包头内容
	sb_u8 type;			//!< 标识该数据为CDR/TDR数据还是MSG数据
	sb_u8 version;		//!< CDR/TDR的版本号，高4位表示主版本，低4位为子版本
	sb_u16 var_ie_num;	//!< CDR中包含的可变IE总数目
	sb_u16 proto_type;	//!< 标识协议类型，取值定义如下：(0x0开头的A+Abis预留；0x1开头Gb/IuPS预留；0x2开头LTE预留；0x3开头WLAN预留)
	sb_u64 id;			//!< 平台生成的全局唯一的CDR标识，用最高的一个字节的前2位表示网络域，后面6位表示采集网关设备ID

}xdr_header_t;

//! 公共信息
typedef struct _xdr_pub_header_tag
{
	sb_u8 interface;			//!< 接口类型，0x01：Gn，0x03:IuPS，0x04：Gb，0x11：S1-MME，0x12：S6a，0x13：S11， 0x14：S10，0x15：S1-U
	sb_u8 imsi[15];
	sb_u8 imei[16];
	sb_u32 ptmsi;
	sb_u32 mcc;
	sb_u32 mnc;
	sb_u32 lac;
	sb_u32 rac;
	sb_u32 cid;
	sb_u32 old_mcc;
	sb_u32 old_mnc;
	sb_u32 old_lac;
	sb_u32 old_rac;
	sb_u32 old_cid;
	sb_u32 sgsn_sig_ip;
	sb_u32 bsc_sig_ip;
	sb_u8 rat;				//!< 无线网络类型， 1：UTRAN，2：GERAN，3：WLAN，4：GAN，5：HSPA，6：EUTRAN
	sb_u8 apn[32];
}xdr_pub_header_t;

//! Attach子流程XDR
typedef struct _xdr_gb_iups_attach_tag
{
	xdr_header_t header;
	xdr_pub_header_t pub_header;

	sb_u8 procedure_type;		//!< 流程类型编码,0--Attach:从attach request开始到 accept complete结束
	sb_u64 procedure_id;		//!< 流程的ID,系统内唯一，最高字节表示设备ID，第二字节表示核ID
	sb_u16 sub_procedure_id;	//!< attach子消息标识，指示是attach请求消息的ID，遵照3GPP协议的消息编码
	sb_u64 start_time;			//!< Attach 请求时间
	sb_u64 end_time;			//!< Attach accept时间
	sb_u16 result_code;			//!< GMM Cause Code，遵照3GPP协议规定
	sb_u8 result_type;			//!< attach成功/失败标示/超时标示，0：成功；1：失败；2：超时
	sb_u8 attach_type;			//!< Attach子类型，3GPP协议规定的Attach类型，1：GPRS attach，2：reserve，3：combine attach，4：紧急附着
	sb_u32 old_ptmsi;			//!< 用户old_P-TMSI
}xdr_gb_iups_attach_t;

//! 取标识子流程XDR
typedef struct _xdr_gb_iups_identity_tag
{
	xdr_header_t header;
	xdr_pub_header_t pub_header;

	sb_u8 procedure_type;		//!< 流程类型编码,0-17
	sb_u64 procedure_id;		//!< 流程的ID,系统内唯一，最高字节表示设备ID，第二字节表示核ID
	sb_u16 sub_procedure_id;	//!< 取标识子消息标识，指示是取标识请求事物的ID，遵照3GPP协议的消息编码
	sb_u64 start_time;			//!< 取标识请求时间
	sb_u64 end_time;			//!< 取标识结束时间
	sb_u16 result_code;			//!< 取标识结果码，GMM Cause Code，遵照3GPP协议规定
	sb_u8 result_type;			//!< 取标识成功/失败标示/超时标示，0：成功；1：失败；2：超时
}xdr_gb_iups_identity_t;

//! 鉴权子流程XDR
typedef struct _xdr_gb_iups_auth_tag
{
	xdr_header_t header;
	xdr_pub_header_t pub_header;

	sb_u8 procedure_type;		//!< 流程类型编码,0-17
	sb_u64 procedure_id;		//!< 流程的ID,系统内唯一，最高字节表示设备ID，第二字节表示核ID
	sb_u16 sub_procedure_id;	//!< 鉴权子消息标识，指示是鉴权请求事物的ID，遵照3GPP协议的消息编码
	sb_u64 start_time;			//!< 鉴权请求时间
	sb_u64 end_time;			//!< 鉴权结束时间
	sb_u16 result_code;			//!< 鉴权结果码，GMM Cause Code，遵照3GPP协议规定
	sb_u8 result_type;			//!< 鉴权成功/失败标示/超时标示，0：成功；1：失败；2：超时
}xdr_gb_iups_auth_t;

//! Detach子流程XDR
typedef struct _xdr_gb_iups_detach_tag
{
	xdr_header_t header;
	xdr_pub_header_t pub_header;

	sb_u8 procedure_type;		//!< 流程类型编码,1--MS-Initiated Detach：从 detach request开始到detach accept结束,2--Network-Initiated GPRS Detach：从 detach request开始到detach accept结束
	sb_u64 procedure_id;		//!< 流程的ID,系统内唯一，最高字节表示设备ID，第二字节表示核ID
	sb_u16 sub_procedure_id;	//!< detach子消息标识，指示是detach请求消息的ID，遵照3GPP协议的消息编码
	sb_u64 start_time;			//!< detach请求时间
	sb_u64 end_time;			//!< detach结束时间
	sb_u16 result_code;			//!< detach结果码，GMM Cause Code，遵照3GPP协议规定
	sb_u8 result_type;			//!< detach成功/失败标示/超时标示，0：成功；1：失败；2：超时
	sb_u8 detach_type;			//!< detach子类型，3GPP协议规定的detach类型，从MS到network（1：GPRS detach，2：IMSIdetach，3：Combined GPRS/IMSI detach），从network到MS（1：re-attach required，2：re-attach not required，3：IMSI detach (after VLR failure)）
}xdr_gb_iups_detach_t;

//! PDP激活子流程XDR
typedef struct _xdr_gb_iups_pdpact_tag
{
	xdr_header_t header;
	xdr_pub_header_t pub_header;

	sb_u8 procedure_type;		//!< 流程类型编码,4,5,6,7
	sb_u64 procedure_id;		//!< 流程的ID,系统内唯一，最高字节表示设备ID，第二字节表示核ID
	sb_u8 net_sub_procedure_id;		//!< 网络发起PDP请求的消息编码，指示是PDP Activation请求消息的ID，遵照3GPP协议的消息编码
	sb_u64 net_request_time;		//!< 网络发起PDP请求子消息开始时间
	sb_u64 net_accept_time;			//!< 网络发起PDP请求子消息结束时间
	sb_u8 net_result_type;			//!< PDP激活成功/失败标示/超时标示，0：成功；1：失败；2：超时
	sb_u8 sub_procedure_id;		//!< PDP激活子消息标识，指示是PDP激活请求消息的ID，遵照3GPP协议的消息编码
	sb_u64 start_time;			//!< PDP激活请求时间
	sb_u64 accept_time;			//!< PDP激活结束时间
	sb_u16 result_code;			//!< PDP激活结果码，SM Cause Code，遵照3GPP协议规定
	sb_u8 result_type;			//!< PDP激活成功/失败标示/超时标示，0：成功；1：失败；2：超时
	sb_u8 pdp_addr[16];			//!< 分配给用户的PDP地址
	sb_u8 connect_type;			//!< 交互类型
	sb_u16 up_rate;				//!< 上行速率
	sb_u16 down_rate;			//!< 下行速率
	sb_u16 gua_up_rate;				//!< 上行保证速率
	sb_u16 gua_down_rate;			//!< 下行保证速率
}xdr_gb_iups_pdpact_t;

//! PDP去激活子流程XDR
typedef struct _xdr_gb_iups_pdpdeact_tag
{
	xdr_header_t header;
	xdr_pub_header_t pub_header;

	sb_u8 procedure_type;		//!< 流程类型编码,10,11
	sb_u64 procedure_id;		//!< 流程的ID,系统内唯一，最高字节表示设备ID，第二字节表示核ID
	sb_u8 sub_procedure_id;		//!< PDP去激活子消息标识，指示是PDP去激活请求消息的ID，遵照3GPP协议的消息编码
	sb_u64 start_time;			//!< PDP去激活请求时间
	sb_u64 end_time;			//!< PDP去激活结束时间
	sb_u16 result_code;			//!< PDP去激活结果码，GMM Cause Code，遵照3GPP协议规定
	sb_u8 result_type;			//!< PDP去激活成功/失败标示/超时标示，0：成功；1：失败；2：超时
}xdr_gb_iups_pdpdeact_t;

//! PDP修改子流程XDR
typedef struct _xdr_gb_iups_pdpmod_tag
{
	xdr_header_t header;
	xdr_pub_header_t pub_header;

	sb_u8 procedure_type;		//!< 流程类型编码,4,5,6,7
	sb_u64 procedure_id;		//!< 流程的ID,系统内唯一，最高字节表示设备ID，第二字节表示核ID
	sb_u8 net_sub_procedure_id;		//!< 网络发起PDP修改请求的消息编码，指示是PDP 修改请求消息的ID，遵照3GPP协议的消息编码
	sb_u64 net_request_time;		//!< 网络发起PDP修改请求子消息开始时间
	sb_u64 net_accept_time;			//!< 网络发起PDP修改请求子消息结束时间
	sb_u8 net_result_type;			//!< PDP修改激活成功/失败标示/超时标示，0：成功；1：失败；2：超时
	sb_u8 sub_procedure_id;		//!< PDP修改子消息标识，指示是PDP修改请求消息的ID，遵照3GPP协议的消息编码
	sb_u64 start_time;		//!< PDP修改请求时间
	sb_u64 end_time;			//!< PDP修改结束时间
	sb_u16 result_code;			//!< PDP修改结果码，SM Cause Code，遵照3GPP协议规定
	sb_u8 result_type;			//!< PDP修改成功/失败标示/超时标示，0：成功；1：失败；2：超时
	sb_u8 connect_type;			//!< 交互类型
	sb_u16 up_rate;				//!< 上行速率
	sb_u16 down_rate;			//!< 下行速率
	sb_u16 gua_up_rate;				//!< 上行保证速率
	sb_u16 gua_down_rate;			//!< 下行保证速率
}xdr_gb_iups_pdpmod_t;

//! RAB子流程XDR
typedef struct _xdr_gb_iups_rab_tag
{
	xdr_header_t header;
	xdr_pub_header_t pub_header;

	sb_u8 procedure_type;		//!< 流程类型编码,4-12,15
	sb_u64 procedure_id;		//!< 流程的ID,系统内唯一，最高字节表示设备ID，第二字节表示核ID
	sb_u8 sub_procedure_id;		//!< RAB子消息标识，指示是RAB消息的ID，遵照3GPP协议的消息编码
	sb_u64 start_time;			//!< RAB子消息开始时间
	sb_u64 end_time;			//!< RAB子消息结束时间
	sb_u16 result_code;			//!< RAB子消息结果码，GMM Cause Code，遵照3GPP协议规定
	sb_u8 result_type;			//!< RAB子消息成功/失败标示/超时标示，0：成功；1：失败；2：超时
}xdr_gb_iups_rab_t;

//! RAU子流程XDR
typedef struct _xdr_gb_iups_rau_tag
{
	xdr_header_t header;
	xdr_pub_header_t pub_header;

	sb_u8 procedure_type;		//!< 流程类型编码,3
	sb_u64 procedure_id;		//!< 流程的ID,系统内唯一，最高字节表示设备ID，第二字节表示核ID
	sb_u8 sub_procedure_id;		//!< RAU子消息标识，指示是RAU请求消息的ID，遵照3GPP协议的消息编码
	sb_u64 start_time;			//!< RAU子消息开始时间
	sb_u64 end_time;			//!< RAU子消息结束时间
	sb_u16 result_code;			//!< RAU子消息结果码，GMM Cause Code，遵照3GPP协议规定
	sb_u8 result_type;			//!< RAU子消息成功/失败标示/超时标示，0：成功；1：失败；2：超时
	sb_u8 rau_type;				//!< 路由区更新类型，按照3GPP协议编码，0：RA Updating，1：Combined RA/LA updating，2：Combined RA/LA updating with IMSI attach，3：Periodic updating
	sb_u32 old_ptmsi;			//!< 用户old_P-TMSI
}xdr_gb_iups_rau_t;

//! BSSGP子流程XDR
typedef struct _xdr_gb_iups_bssgp_tag
{
	xdr_header_t header;
	xdr_pub_header_t pub_header;

	sb_u8 procedure_type;		//!< 流程类型编码,16
	sb_u64 procedure_id;		//!< 流程的ID,系统内唯一，最高字节表示设备ID，第二字节表示核ID
	sb_u8 sub_procedure_id;		//!< BSSGP子消息标识，指示是BSSGP消息的ID，遵照3GPP协议的消息编码
	sb_u64 start_time;			//!< BSSGP子消息开始时间
	sb_u64 end_time;			//!< BSSGP子消息结束时间
	sb_u16 result_code;			//!< BSSGP子消息结果码，GMM Cause Code，遵照3GPP协议规定
	sb_u8 result_type;			//!< BSSGP子消息成功/失败标示/超时标示，0：成功；1：失败；2：超时
}xdr_gb_iups_bssgp_t;

//! RANAP子流程XDR
typedef struct _xdr_gb_iups_ranap_tag
{
	xdr_header_t header;
	xdr_pub_header_t pub_header;

	sb_u8 procedure_type;		//!< 流程类型编码,15
	sb_u64 procedure_id;		//!< 流程的ID,系统内唯一，最高字节表示设备ID，第二字节表示核ID
	sb_u8 sub_procedure_id;		//!< RANAP子消息标识，指示是RANAP消息的ID，遵照3GPP协议的消息编码
	sb_u64 start_time;			//!< RANAP子消息开始时间
	sb_u64 end_time;			//!< RANAP子消息结束时间
	sb_u16 result_code;			//!< RANAP子消息结果码，GMM Cause Code，遵照3GPP协议规定
	sb_u8 result_type;			//!< RANAP子消息成功/失败标示/超时标示，0：成功；1：失败；2：超时
}xdr_gb_iups_ranap_t;

//! 重定位子流程XDR
typedef struct _xdr_gb_iups_reloc_tag
{
	xdr_header_t header;
	xdr_pub_header_t pub_header;

	sb_u8 procedure_type;		//!< 流程类型编码,14
	sb_u64 procedure_id;		//!< 流程的ID,系统内唯一，最高字节表示设备ID，第二字节表示核ID
	sb_u8 sub_procedure_id;		//!< 重定位子消息标识，指示是重定位消息的ID，遵照3GPP协议的消息编码
	sb_u64 start_time;			//!< 重定位子消息开始时间
	sb_u64 end_time;			//!< 重定位子消息结束时间
	sb_u16 result_code;			//!< 重定位子消息结果码，释放原因/失败原因
	sb_u8 result_type;			//!< 重定位子消息成功/失败标示/超时标示，0：成功；1：失败；2：超时
	sb_u16 src_rnc_id;			//!< 源RNC ID
	sb_u16 dst_rnc_id;			//!< 目的RNC ID
	sb_u16 relocation_cause;	//!< 切换原因
}xdr_gb_iups_reloc_t;

//! Service Request子流程XDR
typedef struct _xdr_gb_iups_sr_tag
{
	xdr_header_t header;
	xdr_pub_header_t pub_header;

	sb_u8 procedure_type;		//!< 流程类型编码,12，13
	sb_u64 procedure_id;		//!< 流程的ID,系统内唯一，最高字节表示设备ID，第二字节表示核ID
	sb_u8 net_sub_procedure_id;		//!< 网络发起Service Request请求的消息编码，指示是Service Request请求消息的ID，遵照3GPP协议的消息编码
	sb_u64 net_request_time;		//!< 网络发起Service Request请求子消息开始时间
	sb_u64 net_accept_time;			//!< 网络发起Service Request请求子消息结束时间
	sb_u8 net_result_type;			//!< Service Request成功/失败标示/超时标示，0：成功；1：失败；2：超时
	sb_u8 subsesstype;			//!< ps域寻呼还是cs域寻呼
	sb_u8 paging_num;			//!< paging次数
	sb_u8 sub_procedure_id;		//!< Service Request子消息标识，指示是Service Request消息的ID，遵照3GPP协议的消息编码
	sb_u64 start_time;			//!< Service Request子消息开始时间
	sb_u64 end_time;			//!< Service Request子消息结束时间
	sb_u16 result_code;			//!< Service Request子消息结果码，释放原因/失败原因
	sb_u8 result_type;			//!< Service Request子消息成功/失败标示/超时标示，0：成功；1：失败；2：超时
	sb_u8 sr_type;				//!< Service Request类型，0：signalling，1：data，2：paging resp等详看24008
}xdr_gb_iups_sr_t;

//! 2G Paging子流程XDR
typedef struct _xdr_gb_iups_paging_tag
{
	xdr_header_t header;
	xdr_pub_header_t pub_header;

	sb_u8 procedure_type;		//!< 流程类型编码,17--Gb mode GPRS Paging Procedure:从paging到收到第一条上行LLC 报文
	sb_u64 procedure_id;		//!< 流程的ID,系统内唯一，最高字节表示设备ID，第二字节表示核ID
	sb_u8 sub_procedure_id;		//!< 2G Paging子消息标识，指示是2G Paging消息的ID，遵照3GPP协议的消息编码
	sb_u64 start_time;			//!< 2G Paging子消息开始时间
	sb_u64 end_time;			//!< 2G Paging子消息结束时间
	sb_u16 result_code;			//!< 2G Paging子消息结果码，释放原因/失败原因
	sb_u8 result_type;			//!< 2G Paging子消息成功/失败标示/超时标示，0：成功；1：失败；2：超时
	sb_u8 subsesstype;			//!< ps域寻呼还是cs域寻呼
	sb_u8 paging_num;			//!< paging次数
}xdr_gb_iups_paging_t;

//! other子流程XDR
typedef struct _xdr_gb_iups_other_tag
{
	xdr_header_t header;
	xdr_pub_header_t pub_header;

	sb_u8 procedure_type;		//!< 流程类型编码,254
	sb_u64 procedure_id;		//!< 流程的ID,系统内唯一，最高字节表示设备ID，第二字节表示核ID
	sb_u8 sub_procedure_id;		//!< other子消息标识，指示是other消息的ID，遵照3GPP协议的消息编码
	sb_u64 start_time;			//!< other子消息开始时间
	sb_u64 end_time;			//!< other子消息结束时间
	sb_u8 result_type;			//!< other子消息成功/失败标示/超时标示，0：成功；1：失败；2：超时
}xdr_gb_iups_other_t;

//! 数据面公共信息
typedef struct _xdr_userdata_pub_tag
{
	xdr_pub_header_t pub_header;

	sb_u8 procedure_type;		//!< 流程类型编码
	sb_u64 procedure_id;		//!< 流程的ID,系统内唯一，最高字节表示设备ID，第二字节表示核ID
	sb_u64 start_time;			//!< TCP/UDP流开始时间，UTC时间），从1970/1/1 00:00:00开始到当前的毫秒数
	sb_u64 end_time;			//!< TCP/UDP流结束时间，UTC时间），从1970/1/1 00:00:00开始到当前的毫秒数
	sb_u16 app_type;			//!< 应用大类
	sb_u16 app_subtype;			//!< 应用小类
	sb_u8 l4_protocol;			//!< L4协议，0：TCP，1：UDP
	sb_u8 user_ip[16];			//!< 用户IP
	sb_u16 user_port;			//!< 用户端口
	sb_u8 server_ip[16];		//!< 服务器IP
	sb_u16 server_port;			//!< 服务器端口
	sb_u8 sgsn_user_ip[16];		//!< SGSN用户面IP
	sb_u8 bsc_user_ip[16];		//!< BSC/RNC用户面IP
	sb_u32 ul_bytes;			//!< 上行字节数
	sb_u32 dl_bytes;			//!< 下行字节数
	sb_u32 ul_packets;			//!< 上行包数
	sb_u32 dl_packets;			//!< 下行包数
	sb_u32 ul_reorder_packets;		//!< 上行乱序报文数
	sb_u32 dl_reorder_packets;		//!< 下行乱序报文数
	sb_u32 ul_retrans_packets;		//!< 上行重传报文数
	sb_u32 dl_retrans_packets;		//!< 下行重传报文数
	sb_u32 ul_ip_frag_packets;		//!< 上行IP分片包数，以内层分片为准
	sb_u32 dl_ip_frag_packets;		//!< 下行IP分片包数，以内层分片为准
}xdr_userdata_pub_t;

//! 通用数据业务子流程XDR
typedef struct _xdr_tup_tag
{
	xdr_header_t header;
	xdr_userdata_pub_t user_pub_header;

	sb_u32 tcp_create_res_delay;		//!< TCP建链响应时延（ms）
	sb_u32 tcp_create_ack_delay;		//!< TCP建链确认时延（ms）
	sb_u32 tcp_first_req_delay;			//!< TCP建链成功到第一条事务请求的时延（ms）
	sb_u32 tcp_first_res_delay;			//!< 第一条事物请求到其第一个响应包时延（ms）
	sb_u32 windows_size;				//!< 窗口大小,TCP 建链协商后的窗口
	sb_u32 mss_size;					//!< MSS尺寸
	sb_u8 tcp_create_try_times;			//!< TCP建链尝试次数，TCP SYN的次数，一次TCP流多次SYN的数值
	sb_u8 tcp_create_status;			//!< TCP连接状态指示，0：成功，1：失败
	sb_u8 session_end_status;			//!< 会话是否结束标志，1：结束，2：未结束，3：超时
}xdr_tup_t;

#pragma pack()

#endif /* SB_XDR_STRUCT_H_ */
