/*
 * freopen.c - open a file and associate a stream with it
 */
/* $Id$ */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#if ACKCONF_WANT_STDIO && ACKCONF_WANT_EMULATED_FILE

#define PMODE 0666

/* Do not "optimize" this file to use the open with O_CREAT if the file
 * does not exist. The reason is given in fopen.c.
 */

FILE* freopen(const char* name, const char* mode, FILE* stream)
{
	register int i;
	int rwmode = 0, rwflags = 0;
	int fd, flags = stream->_flags & (_IONBF | _IOFBF | _IOLBF | _IOMYBUF);

	(void)fflush(stream); /* ignore errors */
	(void)close(fileno(stream));

	switch (*mode++)
	{
		case 'r':
			flags |= _IOREAD;
			rwmode = O_RDONLY;
			break;
		case 'w':
			flags |= _IOWRITE;
			rwmode = O_WRONLY;
			rwflags = O_CREAT | O_TRUNC;
			break;
		case 'a':
			flags |= _IOWRITE | _IOAPPEND;
			rwmode = O_WRONLY;
			rwflags |= O_APPEND | O_CREAT;
			break;
		default:
			return (FILE*)NULL;
	}

	while (*mode)
	{
		switch (*mode++)
		{
			case 'b':
#if ACKCONF_WANT_O_TEXT_O_BINARY
				flags |= _IOBINARY;
#endif
				continue;
			case '+':
				rwmode = O_RDWR;
				flags |= _IOREAD | _IOWRITE;
				continue;
			/* The sequence may be followed by aditional characters */
			default:
				break;
		}
		break;
	}

#if !ACKCONF_WANT_O_TEXT_O_BINARY
	if ((rwflags & O_TRUNC)
	    || (((fd = open(name, rwmode)) < 0)
	           && (rwflags & O_CREAT)))
	{
		if (((fd = creat(name, PMODE)) >= 0) && (flags & _IOREAD))
		{
			(void)close(fd);
			fd = open(name, rwmode);
		}
	}
#else
	rwflags |= (flags & _IOBINARY) ? O_BINARY : O_TEXT;
	fd = open(name, rwmode | rwflags, PMODE);
#endif

	if (fd < 0)
	{
		for (i = 0; i < FOPEN_MAX; i++)
		{
			if (stream == __iotab[i])
			{
				__iotab[i] = 0;
				break;
			}
		}
		if (stream != stdin && stream != stdout && stream != stderr)
			free((void*)stream);
		return (FILE*)NULL;
	}

	stream->_count = 0;
	stream->_fd = fd;
	stream->_flags = flags;
	return stream;
}

#endif
