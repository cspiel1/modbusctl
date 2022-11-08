#ifndef FCONF_H
#define FCONF_H

#include <stdint.h>


struct fconf {
	char *ttydev;
	int baud;
	int stopbits;
	int slaveid;
	char parity;
};


int read_fconfig(struct fconf *conf, const char *fname);
int write_conf_file(struct fconf *conf, const char *fname);
void free_fconfig(struct fconf *conf);

#endif
