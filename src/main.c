#include <stdio.h>
#include <unistd.h>
#include <modbus.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <asm/ioctls.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>

#include "futil.h"
#include "fconf.h"

static bool run = false;

static void signal_handler(int sig)
{
	run = false;
}


static void print_usage(const char *pname)
{
	printf("Usage:\n%s [options] <address>\n"
		"Options:\n"
		"-s, --set=value\n"
		"       The given value will be written to the address,"
		" which\n"
		"       is mandatory in this case.\n"
		"-n, --nbr=value\n"
		"       Number of 16-bit registers to read.\n"
		"\n", pname);
}


int main(int argc, char *argv[])
{
	int ret;
	modbus_t *ctx = NULL;
	int addr = -1;
	struct fconf conf;
	char fname[255];
	bool set = false;
	uint16_t setval;
	uint16_t nbr = 1;
	int err;

	memset(&conf, 0, sizeof(conf));
	sprintf(fname, "%s/.config/.modbusctl", getenv("HOME"));
	err = read_fconfig(&conf, fname);
	if (err)
		return err;

	while (1) {
		int c;
		static struct option long_options[] = {
			{"set",       1, 0, 's'},
			{"nbr",       1, 0, 'n'},
			{0,           0, 0,   0}
		};

		c = getopt_long(argc, argv, "s:n:", long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 's':
				set = true;
				setval = (uint16_t) atoi(optarg);
				break;

			case 'n':
				nbr = (uint16_t) atoi(optarg);
				if (nbr > 20)
					nbr = 20;
				break;

			default:
				print_usage(argv[0]);
				return EINVAL;
		}
	}

	if (optind < argc) {
		addr = atoi(argv[optind]);
	}
	else {
		print_usage(argv[0]);
		exit(EINVAL);
	}

	printf("Connecting to %s modbus slave %d baud %d parity %c\n",
	       conf.ttydev, conf.slaveid, conf.baud, conf.parity);

	ctx = modbus_new_rtu(conf.ttydev, conf.baud, conf.parity, 8,
			     conf.stopbits);
	if (ctx == NULL) {
		perror("Unable to create the libmodbus context\n");
		goto out;
	}

	/*        ret = modbus_set_debug(ctx, TRUE);*/

	ret = modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS232);
	if(ret < 0){
		perror("modbus_rtu_set_serial_mode error\n");
		goto out;
	}

	ret = modbus_connect(ctx);
	if(ret < 0){
		perror("modbus_connect error\n");
		goto out;
	}

	ret = modbus_set_slave(ctx, conf.slaveid);
	if(ret < 0) {
		perror("modbus_set_slave error\n");
		goto out;
	}

	modbus_set_debug(ctx, 1);

	(void)signal(SIGINT, signal_handler);
	(void)signal(SIGALRM, signal_handler);
	(void)signal(SIGTERM, signal_handler);

	if (set && addr >= 0) {
		ret = modbus_write_registers(ctx, addr, 1, &setval);
	
	}
	else if (addr >= 0) {
		int mret;
		uint16_t *val = malloc(2*nbr);
		printf("Reading %u registers\n", nbr);
		mret = modbus_read_input_registers(ctx, addr, nbr, val);
		if (ctx && mret == -1) {
			printf("ERR - modbus read error\n");
			printf("      (%s)\n", modbus_strerror(errno));
			err = EPROTO;
			free(val);
			goto out;
		}

		printf("%04x :", addr);
		for (uint16_t i=0; i<nbr; i++)
			printf(" %04x", val[i]);

		printf("\n");
		free(val);
	}

out:
	modbus_close(ctx);
	modbus_free(ctx);

	free_fconfig(&conf);

	if (!err && ret < 0)
		err = EIO;

	return err;
}
