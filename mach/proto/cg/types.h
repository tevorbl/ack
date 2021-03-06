/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 */
#ifndef TYPES_H_
#define TYPES_H_


#ifndef TEM_WSIZE
TEM_WSIZE should be defined at this point
#endif
#ifndef TEM_PSIZE
TEM_PSIZE should be defined at this point
#endif
#if TEM_WSIZE>4 || TEM_PSIZE>4
Implementation will not be correct unless a long integer
has more then 4 bytes of precision.
#endif

typedef unsigned char byte;
typedef char * string;

#if TEM_WSIZE>2 || TEM_PSIZE>2
#define full            long
#else
#define full            int
#endif

#define word long
#ifndef WRD_FMT
#define WRD_FMT "%ld"
#endif /* WRD_FMT */

#endif /* TYPES_H_ */
