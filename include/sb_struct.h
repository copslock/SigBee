/*
 * bs_struct.h
 *
 *  Created on: 2014年5月28日
 *      Author: Figoo
 */

#ifndef BS_STRUCT_H_
#define BS_STRUCT_H_

#include "sb_type.h"
#include "sb_defines.h"

//! 时间戳
union sb_time_t{
	sb_u64 capture_time_total;      //!< 时间戳
	struct {
		sb_u64 time_stamp_sec : 32;
		sb_u64 time_stamp_ns  : 32;
	}capture_time_detail;
};

//! 网卡配置
typedef struct _interface_tag
{
	sb_u16 interface_num;	//!<  网卡的个数
	sb_s8 interface_name[CAPTURE_INTERFACE_SIZE][MAX_LINK_NAME];				//!< 截包网卡名
	sb_u8 channel_no[CAPTURE_INTERFACE_SIZE];
}cap_interface_t;

//! 后端设备配置
typedef struct _dev_ip_id_tag
{
	sb_u32 dev_ip;		//!< 设备IP
	sb_u32 dev_id;		//!< 设备ID
}dev_ip_id_t;

//! device.ini配置信息
typedef struct _device_config_tag
{
	sb_u32 device_identity;			//!< 本机的ID
	sb_u32 is_lb_exist;				//!< 是否有前置的负载均衡设备，0：没有，1：有

	cap_interface_t cap_ports;		//!< 所有数据输入端口

	cap_interface_t gb_ports;		//!< GB接入口
	cap_interface_t iups_ports;		//!< IUPS接入口
	cap_interface_t gn_ports;		//!< GN接入口
	cap_interface_t s1mme_ports;	//!< S1MME接入口
	cap_interface_t s1u_ports;		//!< S1U接入口
	cap_interface_t s6a_ports;		//!< S6a接入口
	cap_interface_t s10_ports;		//!< S10接入口
	cap_interface_t s11_ports;		//!< S11接入口
	cap_interface_t s5_ports;		//!< S5接入口

	sb_u32 xdr_dev_ip;				//!< XDR接收服务器IP
	sb_u16 xdr_dev_port;			//!< XDR接收服务器端口号

	sb_u32 dpi_flag;				//!< 是否发送DPI数据，0：不需要，1：需要
	sb_u32 dpi_dev_num;				//!< DPI设备个数
	dev_ip_id_t dpi_dev[MAX_DEV_NUM];	//!< DPI设备信息，包括IP和设备ID
	sb_u32 dpi_send_port_num;		//!< 发送DPI数据包网卡个数
	sb_s8 dpi_interface_name[MAX_DEV_NUM][MAX_LINK_NAME];				//!< 发送DPI数据包网卡名

	sb_u32 ql_flag;				//!< 是否发送全量数据，0：不需要，1：需要
	sb_u32 ql_dev_num;				//!< 全量数据接收设备个数
	sb_u32 ql_dev_id[MAX_DEV_NUM];	//!< 全量数据接收设备ID
	sb_u32 ql_send_port_num;		//!< 发送全量包网卡个数
	sb_s8 ql_interface_name[MAX_DEV_NUM][MAX_LINK_NAME];				//!< 发送全量包网卡名

	sb_u32 psql_flag;				//!< 是否发送PS全量数据，0：不需要，1：需要
	sb_u32 psql_dev_num;				//!< PS全量数据接收设备个数
	sb_u32 psql_dev_id[MAX_DEV_NUM];	//!< 全量数据接收设备ID
	sb_u32 psql_send_port_num;		//!< 发送PS全量数据包网卡个数
	sb_s8 psql_interface_name[MAX_DEV_NUM][MAX_LINK_NAME];				//!< 发送PS全量数据包网卡名

} device_config_t;

//! adv.ini配置信息
typedef struct _adv_config_tag
{
	sb_s32 dataplane_cpu_start;		//!< 起始的dataplane CPU ID，从0开始
	sb_s32 sig_parser_cpus;		//!< sig_parser模块所使用的核数
	sb_s32 data_parser_cpus;		//!< data_parser模块所使用的核数
	sb_s32 xdr_sender_cpus;			//!< xdr_sender模块所使用的核数

	sb_s32 log_level;				//!< 日志级别，默认为0

	sb_s32 mpipe_iqueue_size;		//!< mpipe每个iqueue队列的数量
	sb_s32 mpipe_equeue_size;		//!< mpipe每个equeue队列的数量
	sb_s32 mpipe_queue_buffer_size;	//!< 每条队列的buffer stack数量

	sb_s32 sig_xdr_queue_size;		//!< sig_parser模块到XDR发送模块每个queue队列的数量
	sb_s32 data_xdr_queue_size;		//!< data_parser模块到XDR发送模块每个queue队列的数量
	sb_s32 sig_user_queue_size;		//!< sig_parser模块到data_parser模块每个用户信息queue队列的数量
	sb_s32 sig_data_queue_size;		//!< sig_parser模块到data_parser模块每个数据包信息queue队列的数量
	sb_s32 dpi_user_queue_size;		//!< data_parser模块到dpi用户信息发送模块每个queue队列的数量

	sb_s32 user_timeout;                  //!< 用户超时时间
	sb_s32 xdr_timeout;                  //!< XDR超时时间
	sb_s32 xdr_event_timeslice;             //!< Event时间片
	sb_s32 tcp_udp_timeout;                 //!< TCP/UDP流超时时间
	sb_s32 tcp_udp_inactive_gap;            //!< TCP/UDP流活动间隙

	sb_s32 attach_request_timeout;		//!< Attach超时时间
	sb_s32 rau_request_timeout;			//!< Routing area update超时时间
	sb_s32 pdp_request_timeout;			//!< Activate PDP context超时时间
	sb_s32 detach_request_timeout;		//!< Detach超时时间

	sb_s32 wtp_transaction_inactive_gap;			//!< WTP Transaction之间的时间差
	sb_s32 wtp_inactive_gap;						//!< WTP Transaction内部消息之间的时间差
} adv_config_t;

//! ip与无线协议对应关系
typedef struct _ip_proto_tag
{
	sb_u32 ip;		//!< IP
	sb_u8 proto_id;		//!< 协议ID
}ip_proto_t;

//! 自定义sb_in_addr结构，考虑平台无关性的in_addr结构
typedef sb_u32 sb_in_addr;

//! PS全量过滤条件结构
typedef struct _ps_filter_rule_tag
{
	sb_u32 id;		//!< 规则ID
	sb_u32 user_ip;	//!< 用户层IP地址
	sb_u64 imsi;	//!< IMSI
	sb_u64 imei;	//!< IMEI
	sb_u64 msisdn;	//!< MSISDN
	sb_u32 rn_ip1;	//!< 外层ip地址1
	sb_u32 rn_ip2;	//!< 外层ip地址2
	sb_u32 cid_type;	//!< 小区号类型，0：MCC-MNC-LAC-RAC-CI（GPRS），1：MCC-MNC-LAC-SAC（TD-SCDMA），2：MCC-MNC-CI（TD-LTE）
	sb_u16 mcc;
	sb_u16 mnc;
	sb_u16 lac;
	sb_u16 rac;
	sb_u32 ci;
}ps_filter_rule_t;

#endif /* BS_STRUCT_H_ */
