/*	fcc/fm2/fpc 
	Driver for fast ACK compilers.

	Derived from the C compiler driver from Minix.

	Compile this file with
		cc -O -I<ACK home directory>/config -DF?? driver.c
	where F?? is either FCC, FPC, or FM2.
	Install the resulting binaries in the EM bin directory.
	Suggested names: afcc, afm2, and afpc.
*/

#if FM2+FPC+FCC > 1
Something wrong here! Only one of FM2, FPC, or FCC must be defined
#endif

#ifdef sun3
#define MACHNAME	"m68020"
#define SYSNAME		"sun3"
#endif

#ifdef vax4
#define MACHNAME	"vax4"
#define SYSNAME		"vax4"
#endif

#ifdef i386
#define MACHNAME	"i386"
#define SYSNAME		"i386"
#endif

#include <errno.h>
#include <signal.h>
#include <varargs.h>
#include <stdio.h>
#include <em_path.h>

/*
	Version producing ACK .o files in one pass.
*/
#define MAXARGC	256	/* maximum number of arguments allowed in a list */
#define USTR_SIZE	128	/* maximum length of string variable */

typedef char USTRING[USTR_SIZE];

struct arglist {
	int al_argc;
	char *al_argv[MAXARGC];
};

#define CPP_NAME	"$H/lib.bin/cpp"
#define LD_NAME		"$H/lib.bin/em_led"
#define CV_NAME		"$H/lib.bin/$S/cv"
#define SHELL		"/bin/sh"

char	*CPP;
char	*COMP;
char	*cc = "cc";

int kids =  -1;
int ecount = 0;

struct arglist CPP_FLAGS = {
#ifdef FCC
	7,
#else
	13,
#endif
	{
		"-D__unix",
		"-D_EM_WSIZE=4",
		"-D_EM_PSIZE=4",
		"-D_EM_SSIZE=2",
		"-D_EM_LSIZE=4",
		"-D_EM_FSIZE=4",
		"-D_EM_DSIZE=8",
#ifndef FCC
		"-DEM_WSIZE=4",
		"-DEM_PSIZE=4",
		"-DEM_SSIZE=2",
		"-DEM_LSIZE=4",
		"-DEM_FSIZE=4",
		"-DEM_DSIZE=8",
#endif
	}
};

struct arglist LD_HEAD = {
	2,
	{
		"$H/lib/$S/head_em",
#ifdef FCC
		"$H/lib/$S/head_$A"
#endif
#ifdef FM2
		"$H/lib/$S/head_m2"
#endif
#ifdef FPC
		"$H/lib/$S/head_pc"
#endif
	}
};

struct arglist LD_TAIL = {
#if defined(sun3) || defined(i386)
	5,
#else
	4,
#endif
	{
#ifdef FCC
		"$H/lib/$S/tail_$A",
#endif
#ifdef FM2
		"$H/lib/$S/tail_m2",
#endif
#ifdef FPC
		"$H/lib/$S/tail_pc",
#endif
#if defined(sun3) || defined(i386)
		"$H/lib/$M/tail_fp",
#endif
		"$H/lib/$M/tail_em",
		"$H/lib/$S/tail_mon",
		"$H/lib/$M/end_em"
	}
};

struct arglist align = {
	5, {
#ifdef sun3
		"-a0:4",
		"-a1:4",
		"-a2:0x20000",
		"-a3:4",
		"-b0:0x2020"
#endif
#ifdef vax4
		"-a0:4",
		"-a1:4",
		"-a2:0x400",
		"-a3:4",
		"-b0:0"
#endif
#ifdef i386
		"-a0:4",
		"-a1:4",
		"-a2:4",
		"-a3:4",
		"-b1:0x1880000"
#endif
	}
};

struct arglist COMP_FLAGS;

char *o_FILE = "a.out"; /* default name for executable file */

#define remove(str)	((noexec || unlink(str)), (str)[0] = '\0')
#define cleanup(str)		(str && str[0] && remove(str))
#define init(al)		((al)->al_argc = 1)

char ProgCall[128];

struct arglist SRCFILES;
struct arglist LDFILES;

int RET_CODE = 0;

struct arglist LD_FLAGS;

struct arglist CALL_VEC;

int o_flag = 0;
int c_flag = 0;
int g_flag = 0;
int v_flag = 0;
int O_flag = 0;
int ansi_c = 0;

char *mkstr();
char *malloc();
char *alloc();
char *extension();
char *expand_string();

USTRING ofile;
USTRING BASE;
USTRING tmp_file;

int noexec = 0;

extern char *strcat(), *strcpy(), *mktemp(), *strchr();

trapcc(sig)
	int sig;
{
	signal(sig, SIG_IGN);
	if (kids != -1) kill(kids, sig);
	cleanup(ofile);
	cleanup(tmp_file);
	exit(1);
}

#ifdef FCC
#define lang_suffix()	"c"
#define comp_name()	"$H/lib.bin/c_ce"
#define ansi_c_name()	"$H/lib.bin/c_ce.ansi"
#endif /* FCC */

#ifdef FM2
#define lang_suffix()	"mod"
#define comp_name()	"$H/lib.bin/m2_ce"
#endif /* FM2 */

#ifdef FPC
#define lang_suffix()	"p"
#define comp_name()	"$H/lib.bin/pc_ce"
#endif /* FPC */


#ifdef FCC
int
lang_opt(str)
	char *str;
{
	switch(str[1]) {
	case 'R':
		if (! ansi_c) {
			append(&COMP_FLAGS, str);
			return 1;
		}
		break;
	case '-':	/* debug options */
		append(&COMP_FLAGS, str);
		return 1;
	case 'a':	/* -ansi flag */
		if (! strcmp(str, "-ansi")) {
			ansi_c = 1;
			COMP = expand_string(ansi_c_name());
			return 1;
		}
		break;
	case 'w':	/* disable warnings */
		if (! ansi_c) {
			append(&COMP_FLAGS, str);
			return 1;
		}
		if (str[2]) {
			str[1] = '-';
			append(&COMP_FLAGS, &str[1]);
		}
		else append(&COMP_FLAGS, "-a");
		return 1;
	}
	return 0;
}
#endif /* FCC */

#ifdef FM2
int
lang_opt(str)
	char *str;
{
	switch(str[1]) {
	case '-':	/* debug options */
	case 'w':	/* disable warnings */
	case 'R':	/* no runtime checks */
	case 'W':	/* add warnings */
	case 'L':	/* no line numbers */
	case 'A':	/* extra array bound checks */
	case '3':	/* only accept 3rd edition Modula-2 */
		append(&COMP_FLAGS, str);
		return 1;
	case 'I':
		append(&COMP_FLAGS, str);
		break;	/* !!! */
	case 'U':	/* underscores in identifiers allowed */
		if (str[2] == '\0') {
			append(&COMP_FLAGS, str);
			return 1;
		}
		break;
	case 'e':	/* local extension for Modula-2 compiler:
			   procedure constants
			*/
		str[1] = 'l';
		append(&COMP_FLAGS, str);
		return 1;
	}
	return 0;
}
#endif /* FM2 */

#ifdef FPC
int
lang_opt(str)
	char *str;
{
	switch(str[1]) {
	case '-':	/* debug options */
	case 'a':	/* enable assertions */
	case 'd':	/* allow doubles (longs) */
	case 'i':	/* set size of integer sets */
	case 't':	/* tracing */
	case 'w':	/* disable warnings */
	case 'A':	/* extra array bound checks */
	case 'C':	/* distinguish between lower case and upper case */
	case 'L':	/* no FIL and LIN instructions */
	case 'R':	/* no runtime checks */
		append(&COMP_FLAGS, str);
		return 1;
	case 'u':
	case 'U':
		/* underscores in identifiers */
	case 's':
		/* only compile standard pascal */
	case 'c':
		/* C type strings */
		if (str[2] == '+' && str[3] == '\0') {
			str[2] = 0;
			append(&COMP_FLAGS, str);
			return 1;
		}
	}
	return 0;
}
#endif /* FPC */

main(argc, argv)
	char *argv[];
{
	char *str;
	char **argvec;
	int count;
	char *ext;
	register struct arglist *call = &CALL_VEC;
	char *file;
	char *ldfile;
	char *INCLUDE = 0;
	int compile_cnt = 0;

	setbuf(stdout, (char *) 0);
	basename(*argv++,ProgCall);

	COMP = expand_string(comp_name());
	CPP = expand_string(CPP_NAME);

#ifdef vax4
	append(&CPP_FLAGS, "-D__vax");
#endif
#ifdef sun3
	append(&CPP_FLAGS, "-D__sun");
#endif
#ifdef m68020
	append(&CPP_FLAGS, "-D__mc68020");
	append(&CPP_FLAGS, "-D__mc68000");
#endif

	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, trapcc);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, trapcc);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, trapcc);
	while (--argc > 0) {
		if (*(str = *argv++) != '-') {
			append(&SRCFILES, str);
			continue;
		}

		if (lang_opt(str)) {
		}
		else switch (str[1]) {

		case 'c':	/* stop after producing .o files */
			c_flag = 1;
			break;
		case 'D':	/* preprocessor #define */
		case 'U':	/* preprocessor #undef */
			append(&CPP_FLAGS, str);
			break;
		case 'I':	/* include directory */
			append(&CPP_FLAGS, str);
			break;
		case 'g':	/* debugger support */
			append(&COMP_FLAGS, str);
			g_flag = 1;
			break;
		case 'a':	/* -ansi flag */
			if (! strcmp(str, "-ansi")) {
				ansi_c = 1;
				return 1;
			}
			break;
		case 'o':	/* target file */
			if (argc-- >= 0) {
				o_flag = 1;
				o_FILE = *argv++;
				ext = extension(o_FILE);
				if (ext != o_FILE && ! strcmp(ext, lang_suffix())
				) {
					error("-o would overwrite %s", o_FILE);
				}
			}
			break;
		case 'u':	/* mark identifier as undefined */
			append(&LD_FLAGS, str);
			if (argc-- >= 0)
				append(&LD_FLAGS, *argv++);
			break;
		case 'O':	/* use built in peephole optimizer */
			O_flag = 1;
			break;
		case 'v':	/* verbose */
			v_flag++;
			if (str[2] == 'n')
				noexec = 1;
			break;
		case 'l':	/* library file */
			append(&SRCFILES, str);
			break;
		case 'M':	/* use other compiler (for testing) */
			strcpy(COMP, str+2);
			break;
		case 's':	/* strip */
			if (str[2] == '\0') {
				append(&LD_FLAGS, str);
				break;
			}
			/* fall through */
		default:
			warning("%s flag ignored", str);
			break;
		}
	}

	if (ecount) exit(1);

	count = SRCFILES.al_argc;
	argvec = &(SRCFILES.al_argv[0]);
	while (count-- > 0) {
		ext = extension(*argvec);
		if (*argvec[0] != '-' && 
		    ext != *argvec++ && (! strcmp(ext, lang_suffix())
		)) {
			compile_cnt++;
		}
	}

	if (compile_cnt > 1 && c_flag && o_flag) {
		warning("-o flag ignored");
		o_flag = 0;
	}

#ifdef FM2
	INCLUDE = expand_string("-I$H/lib/m2");
#endif /* FM2 */
#ifdef FCC
	INCLUDE = expand_string(ansi_c ? "-I$H/include/tail_ac" : "-I$H/include/_tail_cc");
	append(&COMP_FLAGS, "-L");
#endif /* FCC */
	count = SRCFILES.al_argc;
	argvec = &(SRCFILES.al_argv[0]);
	while (count-- > 0) {
		register char *f;
		basename(file = *argvec++, BASE);

		ext = extension(file);

		if (file[0] != '-' &&
		    ext != file && (!strcmp(ext, lang_suffix())
		)) {
			if (compile_cnt > 1) printf("%s\n", file);

			ldfile = c_flag ? ofile : alloc((unsigned)strlen(BASE)+3);
			if (
#ifdef FCC
			    !strcmp(ext, "s") &&
#endif
			    needsprep(file)) {
				strcpy(tmp_file, TMP_DIR);
				strcat(tmp_file, "/F_XXXXXX");
				mktemp(tmp_file);
				init(call);
				append(call, CPP);
				concat(call, &CPP_FLAGS);
				append(call, INCLUDE);
				append(call, file);
				if (runvec(call, tmp_file)) {
					file = tmp_file;
				}
				else {
					remove(tmp_file);
					tmp_file[0] = '\0';
					continue;
				}
			}
			init(call);
			if (o_flag && c_flag) {
				f = o_FILE;
			}
			else	f = mkstr(ldfile, BASE, ".", "o", (char *)0);
				append(call, COMP);
#ifdef FCC
				concat(call, &CPP_FLAGS);
#endif
				concat(call, &COMP_FLAGS);
#if FM2 || FCC
				append(call, INCLUDE);
#endif
				append(call, file);
				append(call, f);
			if (runvec(call, (char *) 0)) {
				file = f;
			}
			else {
				remove(f);
				continue;
			}
			cleanup(tmp_file);
			tmp_file[0] = '\0';
		}

		else if (file[0] != '-' &&
			 strcmp(ext, "o") && strcmp(ext, "a")) {
			warning("file with unknown suffix (%s) passed to the loader", ext);
		}

		if (c_flag)
			continue;

		append(&LDFILES, file);
	}

	/* *.s to a.out */
	if (RET_CODE == 0 && LDFILES.al_argc > 0) {
		init(call);
		expand(&LD_HEAD);
		cc = "cc.2g";
		expand(&LD_TAIL);
		append(call, expand_string(LD_NAME));
		concat(call, &align);
		append(call, "-o");
		strcpy(tmp_file, TMP_DIR);
		strcat(tmp_file, "/F_XXXXXX");
		mktemp(tmp_file);
		append(call, tmp_file);
		concat(call, &LD_HEAD);
		concat(call, &LD_FLAGS);
		concat(call, &LDFILES);
		if (g_flag) append(call, expand_string("$H/lib/$M/tail_db"));
#ifdef FCC
		if (! ansi_c) append(call, expand_string("$H/lib/$S/tail_cc.1s"));
#endif
		concat(call, &LD_TAIL);
		if (! runvec(call, (char *) 0)) {
			cleanup(tmp_file);
			exit(RET_CODE);
		}
		init(call);
		append(call, expand_string(CV_NAME));
		append(call, tmp_file);
		append(call, o_FILE);
		runvec(call, (char *) 0);
		cleanup(tmp_file);
	}
	exit(RET_CODE);
}

needsprep(name)
	char *name;
{
	int file;
	char fc;

	file = open(name,0);
	if (file < 0) return 0;
	if (read(file, &fc, 1) != 1) fc = 0;
	close(file);
	return fc == '#';
}

char *
alloc(u)
	unsigned u;
{
	char *p = malloc(u);

	if (p == 0)
		panic("no space");
	return p;
}

char *
expand_string(s)
	char	*s;
{
	char	buf[1024];
	register char	*p = s;
	register char	*q = &buf[0];
	int expanded = 0;

	if (!p) return p;
	while (*p) {
		if (*p == '$') {
			p++;
			expanded = 1;
			switch(*p++) {
			case 'A':
				if (ansi_c) strcpy(q, "ac");
				else strcpy(q, cc);
				break;
			case 'H':
				strcpy(q, EM_DIR);
				break;
			case 'M':
				strcpy(q, MACHNAME);
				break;
			case 'S':
				strcpy(q, SYSNAME);
				break;
			default:
				panic("internal error");
				break;
			}
			while (*q) q++;
		}
		else *q++ = *p++;
	}
	if (! expanded) return s;
	*q++ = '\0';
	p = alloc((unsigned int) (q - buf));
	return strcpy(p, buf);
}

append(al, arg)
	register struct arglist *al;
	char *arg;
{
	if (!arg || !*arg) return;
	if (al->al_argc >= MAXARGC)
		panic("argument list overflow");
	al->al_argv[(al->al_argc)++] = arg;
}

expand(al)
	register struct arglist *al;
{
	register int i = al->al_argc;
	register char **p = &(al->al_argv[0]);

	while (i-- > 0) {
		*p = expand_string(*p);
		p++;
	}
}

concat(al1, al2)
	struct arglist *al1, *al2;
{
	register i = al2->al_argc;
	register char **p = &(al1->al_argv[al1->al_argc]);
	register char **q = &(al2->al_argv[0]);

	if ((al1->al_argc += i) >= MAXARGC)
		panic("argument list overflow");
	while (i-- > 0) {
		*p++ = *q++;
	}
}

/*VARARGS*/
char *
mkstr(va_alist)
	va_dcl
{
	va_list ap;
	char *dst;

	va_start(ap);
	{
		register char *p;
		register char *q;

		dst = q = va_arg(ap, char *);
		p = va_arg(ap, char *);

		while (p) {
			while (*q++ = *p++);
			q--;
			p = va_arg(ap, char *);
		}
	}
	va_end(ap);

	return dst;
}

basename(str, dst)
	char *str;
	register char *dst;
{
	register char *p1 = str;
	register char *p2 = p1;

	while (*p1)
		if (*p1++ == '/')
			p2 = p1;
	p1--;
	while (*p1 != '.' && p1 >= p2) p1--;
	if (p1 >= p2) {
		*p1 = '\0';
		while (*dst++ = *p2++);
		*p1 = '.';
	}
	else
		while (*dst++ = *p2++);
}

char *
extension(fn)
	char *fn;
{
	register char *c = fn;

	while (*c++) ;
	while (*--c != '.' && c >= fn) { }
	if (c++ < fn || !*c) return fn;
	return c;
}

runvec(vec, outp)
	struct arglist *vec;
	char *outp;
{
	int pid, status;

	if (v_flag) {
		pr_vec(vec);
		putc('\n', stderr);
	}
	if ((pid = fork()) == 0) {	/* start up the process */
		if (outp) {	/* redirect standard output	*/
			close(1);
			if (creat(outp, 0666) != 1)
				panic("cannot create output file");
		}
		ex_vec(vec);
	}
	if (pid == -1)
		panic("no more processes");
	kids = pid;
	wait(&status);
	if (status) switch(status & 0177) {
	case SIGHUP:
	case SIGINT:
	case SIGQUIT:
	case SIGTERM:
	case 0:
		break;
	default:
		error("%s died with signal %d\n", vec->al_argv[1], status&0177);
	}
	kids = -1;
	return status ? ((RET_CODE = 1), 0) : 1;
}

/*VARARGS1*/
error(str, s1, s2)
	char *str, *s1, *s2;
{
	fprintf(stderr, "%s: ", ProgCall);
	fprintf(stderr, str, s1, s2);
	putc('\n', stderr);
	ecount++;
}

/*VARARGS1*/
warning(str, s1, s2)
	char *str, *s1, *s2;
{
	fprintf(stderr, "%s: (warning) ", ProgCall);
	fprintf(stderr, str, s1, s2);
	putc('\n', stderr);
}

panic(str)
	char *str;
{
	error(str);
	trapcc(SIGINT);
}

pr_vec(vec)
	register struct arglist *vec;
{
	register char **ap = &vec->al_argv[1];

	vec->al_argv[vec->al_argc] = 0;
	fprintf(stderr, "%s", *ap);
	while (*++ap) {
		fprintf(stderr, " %s", *ap);
	}
}

extern int errno;

ex_vec(vec)
	register struct arglist *vec;
{
	if (noexec)
		exit(0);
	vec->al_argv[vec->al_argc] = 0;
	execv(vec->al_argv[1], &(vec->al_argv[1]));
	if (errno == ENOEXEC) { /* not an a.out, try it with the SHELL */
		vec->al_argv[0] = SHELL;
		execv(SHELL, &(vec->al_argv[0]));
	}
	if (access(vec->al_argv[1], 1) == 0) {
		/* File is executable. */
		error("cannot execute %s", vec->al_argv[1]);
	} else {
		error("%s is not executable", vec->al_argv[1]);
	}
	exit(1);
}
