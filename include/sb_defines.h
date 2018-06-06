/*
 * sb_defines.h
 *
 *  Created on: 2014年5月27日
 *      Author: Figoo
 */

#ifndef SB_DEFINES_H_
#define SB_DEFINES_H_

#define likely(x) __builtin_expect((x),1)
#define unlikely(x) __builtin_expect((x),0)

/*              */
/*    网络协议类型定义                  */
#define ETHER_LEN	6				//!< Ethernet MAC addr Length
#define ETHERTYPE_VLAN			0x8100  //!< vlan protocol
#define	ETHERTYPE_IP      	0x0800	//!< IP Protocol
#define	ETHERTYPE_ARP      	0x0806	//!< Address resolution
#define	ETHERTYPE_REVARP  	0x8035	//!< Reverse ARP
#define	ETHERTYPE_PPPOED	0x8863	//!< PPPOED Protocol
#define	ETHERTYPE_PPPOE	0x8864		//!< PPPOE Protocol
#define	PF_IPPROTO_TCP	6			//!< TCP Protocol
#define	PF_IPPROTO_UDP	17			//!< UDP Protocol
#define PF_PROTO_UNDEF	-1			//!< 未知协议

#define MAX_PROCESS_NAME_SIZE 32			//进程名的最大长度


#define EXE_BUILD "0.0.1"			//编译版本

#define VERSION "1.0.0"				//版本定义

#define	SB_FILENAME_LEN	100		/*              */

//      配置文件        日志级别        /*              */
#define	LOG_LEVEL_FAILURE	0	//!< 日志级别-FAILURE
#define	LOG_LEVEL_OVERLOAD	1	//!< 日志级别-OVERLOAD
#define	LOG_LEVEL_ERROR		2	//!< 日志级别-ERROR
#define	LOG_LEVEL_WARNING	3	//!< 日志级别-WARNING
#define	LOG_LEVEL_NOTICE	4	//!< 日志级别-NOTICE
#define	LOG_LEVEL_DEBUG		5	//!< 日志级别-DEBUG

//      日志文件个数上限                /*              */
#define	MAX_LOG_FILE_NUM	500	//!< 日志文件最多允许个数
#define MAX_LOG_FILE_SIZE (1536 * 1024 * 1024)

/* 程序运行路径 */

#define PROCESS_WORKING_DIR "/opt/sb/"
#define PROCESS_UPDATE_DIR PROCESS_WORKING_DIR "update/"

#define PROCESS_LOG_DIR "log/"  //!< 日志文件相对运行路径的路径
#define PROCESS_CONFIG_DIR      "config/"       //!< 配置文件相对运行路径的路径
#define PROCESS_STATUS_DIR      "status/"       //!< 状态文件相对运行路径的路径
#define PROCESS_MPIPE_DIR "mpipe/" //!< mpipe对应的路径

#define DEVICE_INI_FILENAME PROCESS_WORKING_DIR PROCESS_CONFIG_DIR "device.ini"
#define ADV_INI_FILENAME PROCESS_WORKING_DIR PROCESS_CONFIG_DIR "adv.ini"

#define GB_INI_FILENAME PROCESS_WORKING_DIR PROCESS_CONFIG_DIR "gb_sgsn.ini"
#define IUPS_INI_FILENAME PROCESS_WORKING_DIR PROCESS_CONFIG_DIR "iups_sgsn.ini"
#define GN_INI_FILENAME PROCESS_WORKING_DIR PROCESS_CONFIG_DIR "gn_sgsn.ini"
#define S1MME_INI_FILENAME PROCESS_WORKING_DIR PROCESS_CONFIG_DIR "s1_mme.ini"
#define S1SGW_INI_FILENAME PROCESS_WORKING_DIR PROCESS_CONFIG_DIR "s1_sgw.ini"
#define S6A_INI_FILENAME PROCESS_WORKING_DIR PROCESS_CONFIG_DIR "s6a_mme.ini"
#define S10_INI_FILENAME PROCESS_WORKING_DIR PROCESS_CONFIG_DIR "s10_mme.ini"
#define S11_INI_FILENAME PROCESS_WORKING_DIR PROCESS_CONFIG_DIR "s11_mme.ini"
#define S5_INI_FILENAME PROCESS_WORKING_DIR PROCESS_CONFIG_DIR "s5_sgw.ini"

#define FILTER_INI_FILENAME PROCESS_WORKING_DIR PROCESS_CONFIG_DIR "ps_filter.ini"

#define PROCESS_MPIPE_FILE_NAME "classifier" //!< mpipe文件名

#define MAX_SHM_SIZE  ((size_t)1500*1024*1024)  //共享内存最大值

#define  NAME_MUTEX_POS  ".start.mutex"	//!< 有名信号量名称后缀

//进程ID定义
#define PROCESS_ID_SB		1		//!< SigBee进程ID main
#define PROCESS_ID_SIG_PARSER	2	//!< sig_parser 进程ID
#define PROCESS_ID_DATA_PARSER 3	//!< data_parser 进程ID
#define PROCESS_ID_XDR_SENDER 4		//!< xdr_sender 进程ID
#define PROCESS_ID_UI_SENDER	5	//!< ui_sender 进程ID
#define PROCESS_ID_LOGD	6			//!< logd 进程ID

//进程名称
#define PROCESS_NAME_SB	"sigbee"					//!< SigBee进程名 即初始化父进程
#define PROCESS_NAME_SIG_PARSER	"sig_parser"		//!< sig_parser进程名
#define PROCESS_NAME_DATA_PARSER "data_parser" 		//!< data_parser进程名
#define PROCESS_NAME_XDR_SENDER	"xdr_sender"		//!< xdr_sender进程名
#define PROCESS_NAME_UI_SENDER	"ui_sender"			//!< ui_sender进程名
#define PROCESS_NAME_LOGD	"logd"					//!< logd进程名

#define	SB_FILENAME_LEN	100		/*              */
#define	SB_BUF_LEN_MINI	10		/*              */
#define	SB_BUF_LEN_TINY	64		/*              */
#define	SB_BUF_LEN_SMALL	128	/*              */
#define	SB_BUF_LEN_NORMAL	512	/*              */
#define	SB_BUF_LEN_BIG	1024	/*              */
#define	SB_BUF_LEN_LARGE	2048	/*              */
#define	SB_BUF_LEN_HUGE	4096	/*              */
#define	SB_BUF_LEN_1K	1024	/*      1K      */
#define	SB_BUF_LEN_1M	(SB_BUF_LEN_1K*SB_BUF_LEN_1K)	/*      1M      */
			/*              */
//      文件大小                /*              */
#define	MAX_FILE_LEN	(10*SB_BUF_LEN_1M)	/*      10M     */
#define	MAX_LOG_FILE_LEN	(20*SB_BUF_LEN_1M)	/*      10M     */

#define CAPTURE_INTERFACE_SIZE 16	//!< 配置中capture_interface数组大小
#define MAX_LINK_NAME 10	//!< 网卡名称最大字母数
#define MAX_DEV_NUM	5		//!< 后端DPI，全量数据接收设备最大数

#define XDR_SERVER_PORT	9002
#define DPI_SERVER_PORT 9002

#define MAX_PS_RULE_NUM 1000	//!< PS全量过滤条件最大个数

#define MAX_SIG_XDR_LEN 256		//!< sig_parser到xdr_sender的共享队列的数据长度
#define MAX_DATA_XDR_LEN 1024		//!< data_parser到xdr_sender的共享队列的数据长度

//		单条日志内容长度上限
#define MAX_LOG_CONENT_LEN	256

#define MY_MIN(a,b) ((a)>(b)?(b):(a))   //两数中的小者

#define KEEPALIVETIME 5	//心跳时间为5s

#define DEFAULT_HASH_KEY						( 0x12345678 )

#define MAX_PRO_IP_NUM 1000	//!< 无线协议识别IP最大数
#define MAX_PRO_IP_NODE_NUM 256
#define MAX_PRO_IP_LINK_NUM 256

//接口定义
#define GN_INT		0x01
#define IUPS_INT	0x03
#define GB_INT		0x04
#define S1MME_INT	0x11
#define S6A_INT		0x12
#define S11_INT		0x13
#define S10_INT		0x14
#define S1U_INT		0x15
#define S5_INT		0x16

//标识协议类型,(0x0开头的A+Abis预留；0x1开头Gb/IuPS预留；0x2开头LTE预留；0x3开头WLAN预留)
#define PROTO_A_BSSAP 0x0001
#define PROTO_A_RANAP 0x0002
#define PROTO_A_BICC 0x0003
#define PROTO_A_H248 0x0004
#define PROTO_A_ABIS 0x0005
#define PROTO_A_AABIS 0x0006
#define PROTO_GB_BSSGP 0x1001
#define PROTO_GB_RANAP 0x1002
#define PROTO_GB_GTP_C 0x1003
#define PROTO_GB_GTP_U 0x1004
#define PROTO_GB_MAP 0x1005
#define PROTO_GB_RADIUS 0x1006
#define PROTO_LTE_S1AP_NAS 0x2001
#define PROTO_LTE_GTPV2 0x2002
#define PROTO_LTE_DIAMETER 0x2003
#define PROTO_LTE_S1U_COMMON 0x2010
#define PROTO_LTE_S1U_HTTP 0x2011
#define PROTO_LTE_S1U_DNS 0x2012
#define PROTO_LTE_S1U_MMS 0x2013
#define PROTO_LTE_S1U_FTP 0x2014
#define PROTO_LTE_S1U_EMAIL 0x2015
#define PROTO_LTE_S1U_VOIP 0x2016
#define PROTO_LTE_S1U_IM 0x2017
#define PROTO_LTE_S1U_RTSP 0x2018
#define PROTO_LTE_S1U_P2P 0x2019

//RAT
#define RAT_NONE 0
#define RAT_UTRAN 1
#define RAT_GERAN 2
#define RAT_WLAN 3
#define RAT_GAN 4
#define RAT_HSPAE 5
#define RAT_EUTRAN 6

//信令面流程编码
#define ATTATCH_XDR	0
#define MS_DETACH_XDR 1
#define NETWORK_GPRS_DETACH_XDR 2
#define RAU_XDR 3
#define MS_PDP_ACTIVE_XDR 4
#define NETWORK_PDP_ACTIVE_XDR 5
#define MS_SECONDE_PDP_ACTIVE_XDR 6
#define NETWORK_SECONDE_PDP_ACTIVE_XDR 7
#define MS_PDP_MODIFY_XDR 8
#define NETWORK_PDP_MODIFY_XDR 9
#define MS_PDP_DEACTIVE_XDR 10
#define NETWORK_PDP_DEACTIVE_XDR 11
#define MS_SERVICE_REQUEST_XDR 12
#define NETWORK_SERVICE_REQUEST_XDR 13
#define RELOCATION_XDR 14
#define RANAP_XDR 15
#define BSSGP_XDR 16
#define GPRS_PAGING_XDR 17
#define CTRL_OTHER_XDR 254

//数据面流程编码
#define COMMON_XDR 100
#define DNS_XDR 101
#define MMS_XDR 102
#define HTTP_XDR 103
#define FTP_XDR 104
#define EMAIL_XDR 105
#define VOIP_XDR 106
#define RTSP_XDR 107
#define IM_XDR 108
#define P2P_XDR 109
#define USER_OTHER_XDR 0xffff

//消息成功类型
#define PROCEDURE_SUCC 0
#define PROCEDURE_FAIL 1
#define PROCEDURE_TIMEOUT 2

//DPI应用大类
#define APP_IM 1
#define APP_READ 2
#define APP_WEIBO 3
#define APP_NAV 4
#define APP_VIDEO 5
#define APP_MUSIC 6
#define APP_APPSTORE 7
#define APP_GAME 8
#define APP_PAY 9
#define APP_CARTOON 10
#define APP_EMAIL 11
#define APP_P2P 12
#define APP_VOIP 13
#define APP_MMS 14
#define APP_WEB 15
#define APP_ECONOMIC 16
#define APP_SAV 17
#define APP_OTHER 18

//CDR ID网络域类型，（平台生成的全局唯一的CDR标识，用一个字节的前2位按网络域分开）
#define NET_CS 0x00
#define NET_PS 0x40
#define NET_LTE 0x80
#define NET_WLAN 0xE0

//CI规则类型
#define CI_GPRS	0
#define CI_TD_SCDMA	1
#define CI_TD_LTE	2

#endif /* SB_DEFINES_H_ */
