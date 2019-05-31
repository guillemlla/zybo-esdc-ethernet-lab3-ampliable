/*
 * version3.c demo code
 * The main code activates two state machines, that run concurrently. There is no scheduller.
 * State machines have to
 *
 *  Created on: 12/2/2019
 *      Author: Josep Altet
 *
 * Modifiyed in order to create the ampliable for the Lab3 by
 * Guillem Llados Gomez
 * Sebastien Kanj Bongard
 * May 2019
*/

#include "xparameters.h"
#include "xgpio.h"

#define BACKGROUND 0
#define SQUARE 4
#define Y_ADDRESS 64

struct AMessage
 {
    int buttonValue;
    int swsValue;
 };

// Pointer and variable declaration
    XGpio rgb, push, adr, sws, vsync, leds;

// Variable global to comunicate both state machines.
// It should be a queue.
    int run,color,colorBackground;
#define TRUE 1
#define FALSE 0

void init_gpios()
{
// Check out your own XPAR ID symbol name declared in xparameters.h
// The format is always XPAR_<NAME_IN_VIVADO>_DEVICE_ID
	XGpio_Initialize(&vsync, XPAR_V_SYNC_DEVICE_ID);
	XGpio_SetDataDirection(&vsync, 1, 0xffffffff); //input

	XGpio_Initialize(&adr, XPAR_ADDRESS_DEVICE_ID);
	XGpio_SetDataDirection(&adr, 1, 0x00000000); //output

	XGpio_Initialize(&leds, XPAR_LEDS_DEVICE_ID);
	XGpio_SetDataDirection(&leds, 1, 0x00000000); //output

}
/* Routine that writes the memory cells to represent a square */
void write_square(int xad, int yad, int color)
{
	int i, j;
	int add;

	add = ((yad >> 1 )<< 9) | (xad >> 1);
	for(j=0; j<8; j++)
	{
		for(i=0; i<8; i++)
		{
			XGpio_DiscreteWrite(&adr, 1, add);
			XGpio_DiscreteWrite(&rgb, 1, color);
			add++;
		}
		yad = yad +2;
		add = ((yad >> 1 )<< 9) | (xad >> 1);
	}
}
//====================================================
/* Software State Machine */

void control_state_machine(int buttonValue, int swsValue)
{
	static int state = 0;
	int v_sync;
	static int x_new = 0;
	static int y_new = Y_ADDRESS;
	static int y_old = Y_ADDRESS;
	static int x_old = 0;

	switch (swsValue) {
		case 2:
		case 3:
						if(run){
							run = 0;
						}else{
							run = 1;
						}
						break;
		case 4:
		case 5:
						color = buttonValue;
						break;
		case 8:
		case 9:
						colorBackground = buttonValue;
						break;
	}

	switch (state)
	{
		case 0:
			// ACTIVATED IF RUN = TRUE
			if(run)
				  {
					  state = 1;
					 /* xil_printf("-- Button Pressed --\r\n");*/
				  }
			break;
		case 1:
			v_sync = XGpio_DiscreteRead(&vsync, 1);
			if (!v_sync)
			{
				state = 2;
				 write_square(x_old, y_old, BACKGROUND);
				 write_square(x_new, y_new, color);
				 x_old = x_new;
				 y_old = y_new;
				 x_new = (x_new >= 624) ? 0 : x_new+2;
			}
		case 2:
			v_sync = XGpio_DiscreteRead(&vsync, 1);
			if (v_sync) state = 0;

	}
}

void draw_square(QueueHandle_t queue)
{

  xil_printf("-- Start of the Program --\r\n");
	init_gpios();
	run = 0;
	color = SQUARE;
	colorBackground = BACKGROUND;

	while (1)
	{
		int buttonValue = -1;
		int swsValue = -1;
		struct AMessage *pxRxedMessage;
		if( xQueueReceive( queue, &( pxRxedMessage ), ( TickType_t ) 10 ) )
    {
            buttonValue = pxRxedMessage.buttonValue;
						swsValue = pxRxedMessage.swsValue;
    }
		control_state_machine(buttonValue,swsValue);
	}
}
