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
#include "lwipopts.h"
#ifdef __arm__
#include "xil_printf.h"
#include "FreeRTOS.h"
#include "task.h"
#endif

/* Header files needed to handle with switches */
#include "xparameters.h"
#include "xgpio.h"

u16_t rx_port = 10;

#define BUFF_SIZE  20

void print_ip();


/*
* Client thread, connects to other board through port 10 (u16_t rx_port)
*  
*/
void rx_data()
{
	struct ip_addr servaddr;
	int sock;
	struct sockaddr_in serv_addr;

	char rx_buf[BUFF_SIZE]; //buffer to transmit

	 XGpio push, leds;
	 int psb_check, led_value;

	 /* Configuration of LEDs and SWITCHES */
	 XGpio_Initialize(&push,  XPAR_BTNS_DEVICE_ID);
	 XGpio_SetDataDirection(&push, 1, 0xffffffff);

	 XGpio_Initialize(&leds, XPAR_LEDS_DEVICE_ID);
	 XGpio_SetDataDirection(&leds, 1, 0x00000000);

	/* Set here the host (Other board) IP address */
	IP4_ADDR(&servaddr,  192, 168,   1, 20);

	while(1)
	{

		if ((sock = lwip_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			xil_printf("error creating socket\r\n");
			vTaskDelete(NULL);
			return;
		}

		memset((void*)&serv_addr, 0, sizeof serv_addr);
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(rx_port);
		serv_addr.sin_addr.s_addr = servaddr.addr;

		print_ip("connect to  ", &servaddr);
		xil_printf("... ");

		if (lwip_connect(sock, (struct sockaddr *)&serv_addr, sizeof (serv_addr)) == 0)
			{
			xil_printf("Connected!!\r\n");
			break; /* If connected, this sentence forces to exit the while(1) and resumes at the point connected*/
			}

		close(sock); // If connection fails, close the socket before retrying

		xil_printf("Connection not established. Please, press a button to retry\r\n");
		psb_check = XGpio_DiscreteRead(&push, 1);
		while (!psb_check)
		{
			vTaskDelay(100);
			psb_check = XGpio_DiscreteRead(&push, 1);
		}


	}
	/* -------------------------------------- If Connected */
		print_ip("Connected to  ", &servaddr);
		xil_printf("Port %d", rx_port);

	while (1)
	{
		read(sock, rx_buf, BUFF_SIZE);
		led_value = (int)rx_buf[0];
		XGpio_DiscreteWrite(&leds, 1, led_value);
		xil_printf("Data Received %x\r\n", led_value);

	}

	close(sock);
	vTaskDelete(NULL);
	return;
}
