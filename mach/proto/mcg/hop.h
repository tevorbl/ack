#ifndef HOP_H
#define HOP_H

enum insel_type
{
	INSEL_STRING,
    INSEL_HREG,
	INSEL_VREG,
	INSEL_VALUE,
	INSEL_EOI
};

struct insel
{
	enum insel_type type;
	union
	{
		const char* string;
        struct hreg* hreg;
		struct vreg* vreg;
		struct ir* value;
	}
	u;
};

struct constraint
{
    uint32_t attrs;
};

struct hop
{
	int id;
    struct basicblock* bb;
	struct ir* ir;
	ARRAYOF(struct insel) insels;
	struct vreg* output;

    PMAPOF(struct vreg, struct constraint) constraints;

	ARRAYOF(struct vreg) ins;
	ARRAYOF(struct vreg) outs;
    ARRAYOF(struct vreg) throughs;
    register_assignment_t regsin;
    register_assignment_t regsout;
};

extern struct hop* new_hop(struct basicblock* bb, struct ir* ir);

extern void hop_add_string_insel(struct hop* hop, const char* string);
extern void hop_add_hreg_insel(struct hop* hop, struct hreg* hreg);
extern void hop_add_vreg_insel(struct hop* hop, struct vreg* vreg);
extern void hop_add_value_insel(struct hop* hop, struct ir* ir);
extern void hop_add_eoi_insel(struct hop* hop);

extern void hop_print(char k, struct hop* hop);

#endif

/* vim: set sw=4 ts=4 expandtab : */
