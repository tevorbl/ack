#ifndef IR_H
#define IR_H

#include "ircodes.h"

struct ir
{
	int id;
	enum ir_opcode opcode;
	int size;
	struct ir* left;
	struct ir* right;
	struct ir* root;
	union
	{
		arith ivalue;
		int rvalue;
		const char* lvalue;
		struct basicblock* bvalue;
		ARRAYOF(struct ir) phivalue;
	} u;

	void* state_label; /* used by the iburg instruction selector */
	int insn_no;       /* the table rule number for this instruction */
	int goal_no;       /* the semantic type of this instruction; not stmt */
	IMAPOF(struct hop) hops; /* only for root IRs; by goal */
};

extern const char* ir_names[];

extern struct ir* new_ir0(int opcode, int size);
extern struct ir* new_ir1(int opcode, int size,
	struct ir* c1);
extern struct ir* new_ir2(int opcode, int size,
	struct ir* c1, struct ir* c2);

extern struct ir* new_labelir(const char* label);
extern struct ir* new_wordir(arith value);
extern struct ir* new_constir(int size, arith value);
extern struct ir* new_bbir(struct basicblock* bb);
extern struct ir* new_anyir(int size);
extern struct ir* new_localir(int offset);

typedef bool ir_walker_t(struct ir* node, void* user);
extern struct ir* ir_walk(struct ir* ir, ir_walker_t* callback, void* user);
extern struct ir* ir_find(struct ir* ir, int opcode);

extern void ir_print(char k, const struct ir* ir);

#endif

