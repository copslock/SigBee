/*
 * sb_type.h
 *
 *  Created on: 2014年5月27日
 *      Author: Figoo
 */

#ifndef SB_TYPE_H_
#define SB_TYPE_H_

typedef  char		sb_s8;
typedef  unsigned char	sb_u8;

typedef  short		sb_s16;
typedef  unsigned short	sb_u16;

typedef  int		sb_s32;
typedef  unsigned int	sb_u32;

typedef  double		sb_double;

#ifdef __LP64__
 typedef  long		sb_s64;
 typedef  unsigned long	sb_u64;
#else
 typedef  long long	sb_s64;
 typedef  unsigned long long	sb_u64;
#endif


#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif


#endif /* SB_TYPE_H_ */
