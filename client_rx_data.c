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
 * Added state machine description March 2019
 *
 * Modifiyed in order to create the ampliable for the Lab3 by
 * Guillem Llados Gomez
 * Sebastien Kanj Bongard
 * May 2019
 */

#include <stdio.h>
#include <string.h>

#include "lwip/sockets.h"
#include "netif/xadapter.h"
#include "lwipopts.h"
#include "xil_printf.h"
#include "FreeRTOS.h"
#include "task.h"

/* Header files needed to handle with switches */
#include "xparameters.h"
#include "xgpio.h"

u16_t rx_port = 10;

struct AMessage
 {
    int buttonValue;
    int swsValue;
 } xMessage;

#define BUFF_SIZE  20

void print_ip();

/*
* Client thread, connects to other board through port 10 (u16_t rx_port)
*
*/
void rx_data(QueueHandle_t queue)
{
	static int state = 0;
	struct ip_addr servaddr;
	int sock;
	struct sockaddr_in serv_addr;
	int readflag;

	char rx_buf[BUFF_SIZE]; //buffer to transmit

	 XGpio push, leds, squerestate, color;
	 int psb_check, sws_value_rx, btn_value_rx, square_state_old;
	 square_state_old = 0;

	 /* Configuration of LEDs and SWITCHES */
	 //XGpio_Initialize(&push,  XPAR_BUTTONS_DEVICE_ID);
	 //XGpio_SetDataDirection(&push, 1, 0xffffffff);

	 XGpio_Initialize(&leds, XPAR_LEDS_DEVICE_ID);
	 XGpio_SetDataDirection(&leds, 1, 0x00000000);

	 XGpio_DiscreteWrite(&squerestate, 1, square_state_old);

	 XGpio_Initialize(&color, XPAR_COLOR_DEVICE_ID);
	 XGpio_SetDataDirection(&color, 1, 0x00000000);

	 XGpio_DiscreteWrite(&color, 1, 3);

	/* Set here the host (Other board) IP address */
	IP4_ADDR(&servaddr, 147, 83,   81, 223);

	while(1)
	{
		switch (state)
		{
			case 0:
				if ((sock = lwip_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
					xil_printf("error creating socket\r\n");
				}
				else
					state = 1;
				break;

			case 1:
				memset((void*)&serv_addr, 0, sizeof serv_addr);
				serv_addr.sin_family = AF_INET;
				serv_addr.sin_port = htons(rx_port);
				serv_addr.sin_addr.s_addr = servaddr.addr;

				print_ip("connect to  ", &servaddr);
				xil_printf("... ");

				if (lwip_connect(sock, (struct sockaddr *)&serv_addr, sizeof (serv_addr)) == 0)
				{
					xil_printf("Connected!!\r\n");
					state = 2;
				}
				else
					state = 3;
				break;

			case 2:
				if ((readflag=read(sock, rx_buf, BUFF_SIZE)) > 0){
					sws_value_rx = (int)rx_buf[0];
					btn_value_rx = (int)rx_buf[1];
					XGpio_DiscreteWrite(&leds, 1, sws_value_rx);
					xil_printf("Data Received %x %x\r\n", sws_value_rx, btn_value_rx);

					struct AMessage *pxMessage;
					xMessage.buttonValue = btn_value_rx;
			    xMessage.swsValue = sws_value_rx;
					pxMessage = & xMessage;
    			xQueueSend( queue, ( void * ) &pxMessage, ( TickType_t ) 0 );

				}
				else {
					xil_printf("Error reading data (%d)\r\n",readflag);
					state = 4;
				}
				break;

			case 3:
				xil_printf("Connection not established. Please, press a button to retry\r\n");
				psb_check = XGpio_DiscreteRead(&push, 1);
				while (!psb_check) {
					vTaskDelay(1000);
					psb_check = 1;
				}
				state = 4;
				break;


			case 4:
				xil_printf("Closing socket %d\r\n", sock);
				close(sock);
				state = 0;
				break;

		}

	}
	close(sock);
	vTaskDelete(NULL);
	return;
}
