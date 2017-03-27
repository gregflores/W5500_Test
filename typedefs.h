#ifndef _TYPEDEFS_H_
#define _TYPEDEFS_H_

typedef unsigned char u_char;
typedef unsigned int u_int;
typedef unsigned long u_long;

typedef struct {
	u_char action;
	u_char target;
	u_char number;
	u_char value;
} Request;

#endif
