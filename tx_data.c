/*
 * Part of this code was provided by Xilinx Inc. in  
 * their application note XAPP1026.
 * Neither Xilinx nor UPC IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * This demo code is the starting point of Lab 3 of the ESDC course
 * UPC Telecom School, Barcelona
 * J. Altet/F. Moll, 2018
 *
 */

#include <stdio.h>
#include <string.h>

#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwipopts.h"

#include "config_apps.h"
#include "xil_printf.h"
#include "FreeRTOS.h"
#include "task.h"



/* Header files needed to handle with switches */
#include "xparameters.h"
#include "xgpio.h"
const int LSB_1 = 1;

XGpio dip; // global variable for switches

/* Port used for the transmission */
u16_t my_port = 10;
const int TWO_CHARS = 2;

void print_echo_app_header()
{
    printf("%20s %6d %s\r\n", "SERVER PORT",
    					my_port,
                        "$ SHOULD BE USED IN OTHER BOARD IN RX_DATA");

}

/* thread spawned for each connection */
void sw_to_net(void *p)
{
	int sd = (int)p;
	int BUFF_SIZE = 20;

    int dip_check_old, dip_check;


	char tx_buf[BUFF_SIZE]; //buffer to transmit


	// Read value of switches
	dip_check_old = XGpio_DiscreteRead(&dip, 1);
	xil_printf("Initial DIP status  %x\r\n", dip_check_old);


	while (1)
	{
		/* Reading from swithes. Sending information every time SW[0] changes value */
		dip_check = XGpio_DiscreteRead(&dip, 1);
		/* Checking if bit 0 has changed */
		if ( (dip_check & LSB_1) ^ (dip_check_old & LSB_1) )
				{
					dip_check_old = dip_check;
					xil_printf("New DIP status  %x\r\n", dip_check_old);

					tx_buf[0] = (char) dip_check;
					tx_buf[1] = '\0';
					if (( write(sd, tx_buf, TWO_CHARS)) < 0)
								{
									xil_printf("Error sending data\r\n");
									xil_printf("Closing socket %d\r\n", sd);
									break;
								}

				}
		/* changing the status of this task to not running for 10000 thick time units */
		vTaskDelay(100);
	}


	/* close connection */
	close(sd);
	vTaskDelete(NULL);
}


/*
* Server thread, accepts connections through port 10 (u16_t my_port)
* Launches new thread sw_to_net to transmit switches state. 
*/
void tx_data()
{
	int sock, new_sd;
	struct sockaddr_in address, remote;
	int size;
	int ncons; //  number of accepted connections
	
	if ((sock = lwip_socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return;

	address.sin_family = AF_INET;
	address.sin_port = htons(my_port);
	address.sin_addr.s_addr = INADDR_ANY;


	if (lwip_bind(sock, (struct sockaddr *)&address, sizeof (address)) < 0)
		return;

	lwip_listen(sock, 0);

	size = sizeof(remote);

	/* Switch inizialization */
	XGpio_Initialize(&dip, XPAR_SWTCH_DEVICE_ID);
	XGpio_SetDataDirection(&dip, 1, 0xffffffff);
	
	ncons=0;

	while (1)
	{
		// every new connection creates a processing thread
		if ((new_sd = lwip_accept(sock, (struct sockaddr *)&remote, (socklen_t *)&size)) > 0)
		{
			printf("Connection accepted: %d\n", ++ncons);
			sys_thread_new("sw_to_net", sw_to_net,
				(void*)new_sd,
				THREAD_STACKSIZE,
				DEFAULT_THREAD_PRIO);
		}
	}
}
