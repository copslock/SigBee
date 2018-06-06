/*
 * sb_base_mica.h
 *
 *  Created on: 2014年6月3日
 *      Author: Figoo
 */

#ifndef SB_BASE_MICA_H_
#define SB_BASE_MICA_H_

#define MICA_TYPE_CRYPTO		1				//加密解密类型
#define MICA_TYPE_COMP			2				//压缩解压类型
/* mica压缩解压结构体 */
typedef struct mica_tag
{
	gxio_mica_context_t cb;			//mica context
	sb_s32 mica_type;				//类型MICA_TYPE_CRYPTO 或者 MICA_TYPE_COMP
	tmc_alloc_t alloc;				//分配内存结构体，用来释放使用
	gxio_mica_context_t cb_comp;	//context 压缩解压
	sb_u8* mem;						//内存起始地址
	sb_u64 mem_size;				//内存大小 压缩解压一个大页，加密解密一个小页
	sb_u32 used;					//是否可以使用 初始化后可以使用
}mica_t;

/* mica_info结构体 */
typedef struct mica_info_tag
{
	mica_t mica_crypto;				//加密解密结构体
	mica_t mica_comp;				//压缩解压结构体
}mica_info_t;

/* md5的全局结构体，包括源目的内存地址 encrypt_context 结构体*/
typedef struct mica_md5_tag
{
	sb_u8* src_addr;							//源地址
	sb_u8* dst_addr;							//目的地址
	sb_u8* enc_metadata;						//mica用于操作的内存地址
	sb_s32 max_src_length;						//源数据最大长度
	sb_s32 enc_metadata_length;					//mica用于操作的内存地址最大长度
	sb_s32 src_length;							//保存最近一次操作的源数据的长度，输出时需要使用
	gxcr_context_t encrypt_context;				//
}mica_md5_t;


extern  mica_info_t g_mica_info;


sb_s32 init_mica(mica_t* ,sb_s32 mica_type) ; //初始化mica_t MICA_TYPE_CRYPTO 为 加密解密 , MICA_TYPE_COMP为 压缩解压



sb_s32 init_mica_info(sb_s32 flag);//初始化mica初始化 每个进程都需要初始化   MICA_TYPE_CRYPTO 为 加密解密 , MICA_TYPE_COMP为 压缩解压 ， MICA_TYPE_CRYPTO | MICA_TYPE_COMP 为2者都需要




sb_s32 init_mica_md5(void); //md5 编码 初始化 使用全局变量，分配
sb_s32 do_mica_md5(void *srcdata, sb_s32 len,void *dstdata); //md5 编码

sb_s32 start_mica_md5(void *srcdata, sb_s32 len); //异步md5编码

sb_s32 wait_for_mica_md5(void *dstdata);								//异步md5编码等待结果，大约需要等待1000个周期


#endif /* SB_BASE_MICA_H_ */
