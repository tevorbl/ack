/* $Header$ */
/* CODE FOR THE INITIALISATION OF GLOBAL VARIABLES */

#include	<em.h>

#include	"debug.h"
#include	"nobitfield.h"

#include	"string.h"
#include	"arith.h"
#include	"align.h"
#include	"label.h"
#include	"expr.h"
#include	"type.h"
#include	"struct.h"
#include	"field.h"
#include	"assert.h"
#include	"Lpars.h"
#include	"class.h"
#include	"sizes.h"
#include	"idf.h"
#include	"level.h"
#include	"def.h"

extern char *symbol2str();

#define con_byte(c)	C_con_ucon(itos((long)(c) & 0xFF), (arith)1)

struct expr *do_array(), *do_struct(), *IVAL();
struct expr *strings = 0; /* list of string constants within initialiser */

/*	do_ival() performs the initialisation of a global variable
	of type tp with the initialisation expression expr by calling IVAL().
	Guided by type tp, the expression is evaluated.
*/
do_ival(tpp, expr)
	struct type **tpp;
	struct expr *expr;
{
	if (IVAL(tpp, expr) != 0)
		too_many_initialisers(expr);

	/*	The following loop declares the string constants
		used in the initialisation.
		The code for these string constants may not appear in
		the code of the initialisation because a data label
		in EM causes the current initialisation to be completed.
		E.g. char *s[] = {"hello", "world"};
	*/
	while (strings != 0) {
		C_df_dlb(strings->SG_DATLAB);
		C_con_scon(strings->SG_VALUE, (arith)strings->SG_LEN);
		strings = strings->next;
	}
}


/*	store_string() collects the string constants appearing in an
	initialisation.
*/
store_string(expr)
	struct expr *expr;
{
	expr->next = strings;
	strings = expr;
}


/*	IVAL() recursively guides the initialisation expression through the
	different routines for the different types of initialisation:
	-	array initialisation
	-	struct initialisation
	-	fundamental type initialisation
	Upto now, the initialisation of a union is not allowed!
	An initialisation expression tree consists of normal expressions
	which can be joined together by ',' nodes, which operator acts
	like the lisp function "cons" to build lists.
	IVAL() returns a pointer to the remaining expression tree.
*/
struct expr *
IVAL(tpp, expr)
	struct type **tpp;		/* type of global variable	*/
	struct expr *expr;		/* initialiser expression	*/
{
	register struct type *tp = *tpp;
	
	switch (tp->tp_fund) {
	case ARRAY:
		/* array initialisation	*/
		if (valid_type(tp->tp_up, "array element") == 0)
			return 0;
		if (ISCOMMA(expr))	{
			/* list of initialisation expressions */
			return do_array(expr, tpp);
		}
		/*	There might be an initialisation of a string
			like char s[] = "I am a string"
		*/
		if (tp->tp_up->tp_fund == CHAR && expr->ex_class == String)
			init_string(tpp, expr);
		else /* " int i[24] = 12;"	*/
			check_and_pad(expr, tpp);
		return 0;	/* nothing left	*/
	case STRUCT:
		/* struct initialisation */
		if (valid_type(tp, "struct") == 0)
			return 0;
		if (ISCOMMA(expr)) /* list of initialisation expressions */
			return do_struct(expr, tp);
		/* "struct foo f = 12;"	*/
		check_and_pad(expr, tpp);
		return 0;
	case UNION:
		error("union initialisation not allowed");
		return 0;
	case ERRONEOUS:
		return 0;
	default: /* fundamental type	*/
		if (ISCOMMA(expr))	{ /* " int i = {12};"	*/
			if (IVAL(tpp, expr->OP_LEFT) != 0)
				too_many_initialisers(expr);
			/*	return remainings of the list for the
				other members of the aggregate, if this
				item belongs to an aggregate.
			*/
			return expr->OP_RIGHT;
		}
		/* "int i = 12;"	*/
		check_ival(expr, tp);
		return 0;
	}
	/* NOTREACHED */
}

/*	do_array() initialises the members of an array described
	by type tp with the expressions in expr.
	Two important cases:
	-	the number of members is known
	-	the number of members is not known
	In the latter case, do_array() digests the whole expression
	tree it is given.
	In the former case, do_array() eats as many members from
	the expression tree as are needed for the array.
	If there are not sufficient members for the array, the remaining
	members are padded with zeroes
*/
struct expr *
do_array(expr, tpp)
	struct expr *expr;
	struct type **tpp;
{
	register struct type *tp = *tpp;
	register arith elem_count;
	
	ASSERT(tp->tp_fund == ARRAY && ISCOMMA(expr));
	/*	the following test catches initialisations like
		char c[] = {"just a string"};
		or
		char d[] = {{"just another string"}};
		The use of the brackets causes this problem.
		Note: although the implementation of such initialisations
		is completely foolish, we did it!! (no applause, thank you)
	*/
	if (tp->tp_up->tp_fund == CHAR) {
		register struct expr *f = expr->OP_LEFT;
		register struct expr *g = 0;

		while (ISCOMMA(f)) {	/* eat the brackets!!!	*/
			g = f;
			f = f->OP_LEFT;
		}
		if (f->ex_class == String) { /* hallelujah, it's a string! */
			init_string(tpp, f);
			return g ? g->OP_RIGHT : expr->OP_RIGHT;
		}
		/* else: just go on with the next part of this function */
		if (g != 0)
			expr = g;
	}
	if (tp->tp_size == (arith)-1) {
		/* declared with unknown size: [] */
		for (elem_count = 0; expr; elem_count++) {
			/* eat whole initialisation expression	*/
			if (ISCOMMA(expr->OP_LEFT)) {
				/* the member expression is embraced	*/
				if (IVAL(&(tp->tp_up), expr->OP_LEFT) != 0)
					too_many_initialisers(expr);
				expr = expr->OP_RIGHT;
			}
			else {
				if (aggregate_type(tp->tp_up))
					expr = IVAL(&(tp->tp_up), expr);
				else {
					check_ival(expr->OP_LEFT, tp->tp_up);
					expr = expr->OP_RIGHT;
				}
			}
		}
		/* set the proper size	*/
		*tpp = construct_type(ARRAY, tp->tp_up, elem_count);
	}
	else {		/* the number of members is already known	*/
		arith dim = tp->tp_size / tp->tp_up->tp_size;

		for (elem_count = 0; elem_count < dim && expr; elem_count++) {
			if (ISCOMMA(expr->OP_LEFT)) {
				/* embraced member initialisation	*/
				if (IVAL(&(tp->tp_up), expr->OP_LEFT) != 0)
					too_many_initialisers(expr);
				expr = expr->OP_RIGHT;
			}
			else {
				if (aggregate_type(tp->tp_up))
					/* the member is an aggregate	*/
					expr = IVAL(&(tp->tp_up), expr);
				else {
					check_ival(expr->OP_LEFT, tp->tp_up);
					expr = expr->OP_RIGHT;
				}
			}
		}
		if (expr && elem_count == dim)
			/*	all the members are initialised but there
				remains a part of the expression tree which
				is returned
			*/
			return expr;
		if ((expr == 0) && elem_count < dim) {
			/*	the expression tree is completely absorbed
				but there are still members which must be
				initialised with zeroes
			*/
			do
				pad(tp->tp_up);
			while (++elem_count < dim);
		}
	}
	return 0;
}


/*	do_struct() initialises a struct of type tp with the expression expr.
	The main loop is just controlled by the definition of the selectors
	during which alignment is taken care of.
*/
struct expr *
do_struct(expr, tp)
	struct expr *expr;
	struct type *tp;
{
	struct sdef *sd = tp->tp_sdef;
	arith bytes_upto_here = (arith)0;
	arith last_offset = (arith)-1;
	
	ASSERT(tp->tp_fund == STRUCT && ISCOMMA(expr));
	/* as long as there are selectors and there is an initialiser..	*/
	while (sd && expr) {
		if (ISCOMMA(expr->OP_LEFT)) {	/* embraced expression	*/
			if (IVAL(&(sd->sd_type), expr->OP_LEFT) != 0)
				too_many_initialisers(expr);
			expr = expr->OP_RIGHT;
		}
		else {
			if (aggregate_type(sd->sd_type))
				/* selector is an aggregate itself	*/
				expr = IVAL(&(sd->sd_type), expr);
			else {
#ifdef NOBITFIELD
				/* fundamental type, not embraced */
				check_ival(expr->OP_LEFT, sd->sd_type);
				expr = expr->OP_RIGHT;
#else
				if (is_anon_idf(sd->sd_idf))
					/*	a hole in the struct due to
						the use of ";:n;" in a struct
						definition.
					*/
					put_bf(sd->sd_type, (arith)0);
				else {
					/* fundamental type, not embraced */
					check_ival(expr->OP_LEFT,
							sd->sd_type);
					expr = expr->OP_RIGHT;
				}
#endif NOBITFIELD
			}
		}
		/* align upto the next selector	boundary	*/
		if (sd->sd_sdef)
			bytes_upto_here += zero_bytes(sd);
		if (last_offset != sd->sd_offset) {
			/* don't take the field-width more than once	*/
			bytes_upto_here +=
				size_of_type(sd->sd_type, "selector");
			last_offset = sd->sd_offset;
		}
		sd = sd->sd_sdef;
	}
	/* perfect fit if (expr && (sd == 0)) holds	*/
	if ((expr == 0) && (sd != 0)) {
		/*	there are selectors left which must be padded with
			zeroes
		*/
		do {
			pad(sd->sd_type);
			/* take care of the alignment restrictions	*/
			if (sd->sd_sdef)
				bytes_upto_here += zero_bytes(sd);
			/* no field thrown-outs here	*/
			bytes_upto_here +=
				size_of_type(sd->sd_type, "selector");
		} while (sd = sd->sd_sdef);
	}
	/* keep on aligning...	*/
	while (bytes_upto_here++ < tp->tp_size)
		con_byte(0);
	return expr;
}

/*	check_and_pad() is given a simple initialisation expression
	where the type can be either a simple or an aggregate type.
	In the latter case, only the first member is initialised and
	the rest is zeroed.
*/
check_and_pad(expr, tpp)
	struct expr *expr;
	struct type **tpp;
{
	/* expr is of a fundamental type	*/
	struct type *tp = *tpp;

	if (tp->tp_fund == ARRAY) {
		if (valid_type(tp->tp_up, "array element") == 0)
			return;
		check_and_pad(expr, &(tp->tp_up));	/* first member	*/
		if (tp->tp_size == (arith)-1)
			/*	no size specified upto here: just
				set it to the size of one member.
			*/
			tp = *tpp = construct_type(ARRAY, tp->tp_up, (arith)1);
		else {
			register dim = tp->tp_size / tp->tp_up->tp_size;
			/* pad remaining members with zeroes */
			while (--dim > 0)
				pad(tp->tp_up);
		}
	}
	else
	if (tp->tp_fund == STRUCT) {
		register struct sdef *sd = tp->tp_sdef;

		if (valid_type(tp, "struct") == 0)
			return;
		check_and_pad(expr, &(sd->sd_type));
		/* Next selector is aligned by adding extra zeroes */
		if (sd->sd_sdef)
			zero_bytes(sd);
		while (sd = sd->sd_sdef) { /* pad remaining selectors	*/
			pad(sd->sd_type);
			if (sd->sd_sdef)
				zero_bytes(sd);
		}
	}
	else	/* simple type	*/
		check_ival(expr, tp);
}

/*	pad() fills an element of type tp with zeroes.
	If the element is an aggregate, pad() is called recursively.
*/
pad(tp)
	struct type *tp;
{
	switch (tp->tp_fund) {
	case ARRAY:
	{
		register long dim;

		if (valid_type(tp->tp_up, "array element") == 0)
			return;

		dim = tp->tp_size / tp->tp_up->tp_size;

		/* Assume the dimension is known	*/
		while (dim-- > 0)
			pad(tp->tp_up);
		break;
	}
	case STRUCT:
	{
		register struct sdef *sdef = tp->tp_sdef;

		if (valid_type(tp, "struct") == 0)
			return;

		do {
			pad(sdef->sd_type);
			if (sdef->sd_sdef)
				zero_bytes(sdef);
		} while (sdef = sdef->sd_sdef);
		break;
	}
#ifndef NOBITFIELD
	case FIELD:
		put_bf(tp, (arith)0);
		break;
#endif NOBITFIELD
	case INT:
	case SHORT:
	case LONG:
	case CHAR:
	case ENUM:
	case POINTER:
		C_con_ucon("0",  tp->tp_size);
		break;
	case FLOAT:
	case DOUBLE:
		C_con_fcon("0", tp->tp_size);
		break;
	case UNION:
		error("initialisation of unions not allowed");
		break;
	case ERRONEOUS:
		break;
	default:
		crash("(generate) bad fundamental type %s\n",
			symbol2str(tp->tp_fund));
	}
}

/*	check_ival() checks whether the initialisation of an element
	of a fundamental type is legal and, if so, performs the initialisation
	by directly generating the necessary code.
	No further comment is needed to explain the internal structure
	of this straightforward function.
*/
check_ival(expr, type)
	struct expr *expr;
	struct type *type;
{
	/*	The philosophy here is that ch7cast puts an explicit
		conversion node in front of the expression if the types
		are not compatible.  In this case, the initialisation
		expression is no longer a constant.
	*/
	
	switch (type->tp_fund) {
	case CHAR:
	case SHORT:
	case INT:
	case LONG:
	case ENUM:
		ch7cast(&expr, '=', type);
		if (!is_cp_cst(expr))	{
			illegal_init_cst(expr);
			break;
		}
		con_int(expr);
		break;
#ifndef NOBITFIELD
	case FIELD:
		ch7cast(&expr, '=', type->tp_up);
		if (!is_cp_cst(expr))	{
			illegal_init_cst(expr);
			break;
		}
		put_bf(type, expr->VL_VALUE);
		break;
#endif NOBITFIELD
	case FLOAT:
	case DOUBLE:
		ch7cast(&expr, '=', type);
		if (expr->ex_class == Float)
			C_con_fcon(expr->FL_VALUE, expr->ex_type->tp_size);
		else
		if (expr->ex_class == Oper && expr->OP_OPER == INT2FLOAT) {
			expr = expr->OP_RIGHT;
			if (!is_cp_cst(expr))	{
				illegal_init_cst(expr);
				break;
			}
			C_con_fcon(itos(expr->VL_VALUE), type->tp_size);
		}
		else
			illegal_init_cst(expr);
		break;
	case POINTER:
		ch7cast(&expr, '=', type);
		switch (expr->ex_class) {
		case Oper:
			illegal_init_cst(expr);
			break;
		case String:	/* char *s = "...." */
		{
			label datlab = data_label();
			
			C_ina_dlb(datlab);
			C_con_dlb(datlab, (arith)0);
			expr->SG_DATLAB = datlab;
			store_string(expr);
			break;
		}
		case Value:
		{
			struct value *vl = &(expr->ex_object.ex_value);
			struct idf *idf = vl->vl_idf;

			ASSERT(expr->ex_type->tp_fund == POINTER);
			if (expr->ex_type->tp_up->tp_fund == FUNCTION) {
				if (idf)
					C_con_pnam(idf->id_text);
				else	/* int (*func)() = 0	*/
					con_int(expr);
			}
			else
			if (idf) {
				register struct def *def = idf->id_def;

				if (def->df_level >= L_LOCAL) {
					if (def->df_sc != STATIC)
						/*	Eg.  int a;
							static int *p = &a;
						*/
						expr_error(expr,
							"illegal initialisation"
						);
					else
						C_con_dlb(
							(label)def->df_address,
							vl->vl_value
						);
				}
				else
					C_con_dnam(idf->id_text, vl->vl_value);
			}
			else
				con_int(expr);
			break;
		}
		default:
			crash("(check_ival) illegal initialisation expression");
		}
		break;
	case ERRONEOUS:
		break;
	default:
		crash("(check_ival) bad fundamental type %s",
			symbol2str(type->tp_fund));
	}
}

/*	init_string() initialises an array of characters by specifying
	a string constant.
	Alignment is taken care of.
*/
init_string(tpp, expr)
	struct type **tpp;	/* type tp = array of characters	*/
	struct expr *expr;
{
	register struct type *tp = *tpp;
	register arith length;
	char *s = expr->SG_VALUE;
	arith ntopad;

	length = expr->SG_LEN;
	if (tp->tp_size == (arith)-1)	{
		/* set the dimension	*/
		tp = *tpp = construct_type(ARRAY, tp->tp_up, length);
		ntopad = align(tp->tp_size, word_align) - tp->tp_size;
	}
	else {
		arith dim = tp->tp_size / tp->tp_up->tp_size;

		ntopad = align(dim, word_align) - length;
		if (length > dim)
			expr_error(expr,
				"too many characters in initialiser string");
	}
	/* throw out the characters of the already prepared string	*/
	do
		con_byte(*s++);
	while (--length > 0);
	/* pad the allocated memory (the alignment has been calculated)	*/
	while (ntopad-- > 0)
		con_byte(0);
}

#ifndef NOBITFIELD
/*	put_bf() takes care of the initialisation of (bit-)field
	selectors of a struct: each time such an initialisation takes place,
	put_bf() is called instead of the normal code generating routines.
	Put_bf() stores the given integral value into "field" and
	"throws" the result of "field" out if the current selector
	is the last of this number of fields stored at the same address.
*/
put_bf(tp, val)
	struct type *tp;
	arith val;
{
	static long field = (arith)0;
	static arith offset = (arith)-1;
	register struct field *fd = tp->tp_field;
	register struct sdef *sd =  fd->fd_sdef;
	static struct expr expr;

	ASSERT(sd);
	if (offset == (arith)-1) {
		/* first bitfield in this field	*/
		offset = sd->sd_offset;
		expr.ex_type = tp->tp_up;
		expr.ex_class = Value;
	}
	if (val != 0)	/* insert the value into "field"	*/
		field |= (val & fd->fd_mask) << fd->fd_shift;
	if (sd->sd_sdef == 0 || sd->sd_sdef->sd_offset != offset) {
		/* the selector was the last stored at this address	*/
		expr.VL_VALUE = field;
		con_int(&expr);
		field = (arith)0;
		offset = (arith)-1;
	}
}
#endif NOBITFIELD

int
zero_bytes(sd)
	struct sdef *sd;
{
	/*	fills the space between a selector of a struct
		and the next selector of that struct with zero-bytes.
	*/
	register int n =
		sd->sd_sdef->sd_offset - sd->sd_offset -
		size_of_type(sd->sd_type, "struct member");
	register count = n;

	while (n-- > 0)
		con_byte((arith)0);
	return count;
}

int
valid_type(tp, str)
	struct type *tp;
	char *str;
{
	if (tp->tp_size < 0) {
		error("size of %s unknown", str);
		return 0;
	}
	return 1;
}

con_int(expr)
	register struct expr *expr;
{
	register struct type *tp = expr->ex_type;

	if (tp->tp_unsigned)
		C_con_ucon(itos(expr->VL_VALUE), tp->tp_size);
	else
		C_con_icon(itos(expr->VL_VALUE), tp->tp_size);
}

illegal_init_cst(expr)
	struct expr *expr;
{
	if (expr->ex_type->tp_fund != ERRONEOUS)
		expr_error(expr, "illegal initialisation constant");
}

too_many_initialisers(expr)
	struct expr *expr;
{
	expr_error(expr, "too many initialisers");
}

aggregate_type(tp)
	struct type *tp;
{
	return tp->tp_fund == ARRAY || tp->tp_fund == STRUCT;
}
