/*
 * init.h
 *
 *  Created on: 2014年5月27日
 *      Author: Figoo
 */

#ifndef INIT_H_
#define INIT_H_

sb_s32 read_dev_config (sb_s8 * device_ini_filename);
sb_s32 read_adv_config (sb_s8 * ini_filename);
sb_s32 read_ips_config (void);
sb_s32 read_filter_config (sb_s8 * ini_filename);
sb_s32 init_instance (sb_s32 argc, sb_s8 ** argv);
sb_s32 init_initialize (sb_s32 argc, sb_s8 ** argv);
sb_s32 init_config (sb_s8 * process_name, sb_u16 argc, sb_s8 ** argv);
sb_s32 init_global_vars(void);
void release_resource (void);
sb_s32 check_process (sb_s8 * process_name, sb_u16 argc, sb_s8 ** argv);
sb_s32 start_daemon (sb_s8 * process_name, sb_u16 argc, sb_s8 ** argv);
void delete_name_mutex (void);
void exit_sb (void);
sb_s32 sb_statistic_init(void);
sb_s32 create_process( sb_s32 argc , sb_s8 ** argv );

void sig_parser_main (void);
void data_parser_main (void);
void xdr_sender_main(sb_s32 argc, sb_s8 ** argv);
void ui_sender_main(void);

#endif /* INIT_H_ */
