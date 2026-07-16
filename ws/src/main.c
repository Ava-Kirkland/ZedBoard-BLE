/*
Name: Ava Kirkland
Date: 6/11/2026
Company: Nspired Engineering
Description: Bidirectional UART bridge with polling. 
Before runnng have... 
- ZedBoard in JTAG boot and configuration
- Pmod BLE connected to the ZedBoard via JE
- Pmod BLE connected UART 0
- Tera Term connected to the ZedBoard UART COM#
- Tera Term set to Baud Rate: 115200 
*/

#include <stdio.h>
#include <xstatus.h>
#include <xuartps_hw.h>
#include "xuartps.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "sleep.h"

// UART data structure instances
XUartPs Uart0;   // BLE UART
XUartPs Uart1;   // Terminal UART

void clear_uart(){
    //Flush UART 0 RX buffer
    while(XUartPs_IsReceiveData(Uart0.Config.BaseAddress)){
        u8 garabage;
        XUartPs_Recv(&Uart0, &garabage, 1);
    }

    //Flush UART 1 RX buffer
    while(XUartPs_IsReceiveData(Uart1.Config.BaseAddress)){
        u8 garabage;
        XUartPs_Recv(&Uart1, &garabage, 1);
    }
    // 1 second of quiet
    msleep(1);
}


int init_uart()
{
    XUartPs_Config *cfg;
    int status;

    // -------------------------
    // Initialize UART1 (Terminal)
    // -------------------------

    //Does the device have a UART structure
    cfg = XUartPs_LookupConfig(XPAR_XUARTPS_1_BASEADDR);
    if (cfg == NULL) {
        xil_printf("UART1 LookupConfig FAILED\r\n");
        return XST_FAILURE;
    }
    //Link the UART structure of the device to the UART struct instance
    status = XUartPs_CfgInitialize(&Uart1, cfg, cfg->BaseAddress);
    if (status != XST_SUCCESS) {
        xil_printf("UART1 CfgInitialize FAILED\r\n");
        return status;
    }
    //Set Baud Rate
    XUartPs_SetBaudRate(&Uart1, 115200);
    xil_printf("UART1 initialized (Terminal UART) @115200\r\n");

    // -------------------------
    // Initialize UART0 (BLE)
    // -------------------------
    cfg = XUartPs_LookupConfig(XPAR_XUARTPS_0_BASEADDR);
    if (cfg == NULL) {
        xil_printf("UART0 LookupConfig FAILED\r\n");
        return XST_FAILURE;
    }

    status = XUartPs_CfgInitialize(&Uart0, cfg, cfg->BaseAddress);
    if (status != XST_SUCCESS) {
        xil_printf("UART0 CfgInitialize FAILED\r\n");
        return status;
    }

    XUartPs_SetBaudRate(&Uart0, 115200);
    xil_printf("UART0 initialized (BLE UART) @115200\r\n");


    xil_printf("Both UARTs ready.\r\n\r\n");
    //clear_uart();
    return XST_SUCCESS;
}



int main()
{
    u8 newline[2] = {'\r', '\n'};
    int status;
    status = init_uart();
    if (status != XST_SUCCESS){
        xil_printf("ERROR: UART init");
        return XST_FAILURE;
    }else{
        
        xil_printf("UART Bridge Running... \r\n");
//maybe eror with c8 comnvfigured
        while (1) {
            u8 c;
            u32 received;
            // Terminal → BLE
            if (XUartPs_IsReceiveData(Uart1.Config.BaseAddress)) {

                    //Receives 1 character
                    received = XUartPs_Recv(&Uart1, &c, 1);
                if (received == 1){
                    //Echo to Terminal
                    if ( c == '\r'){
                    //while (XUartPs_IsTransmitFull(Uart1.Config.BaseAddress));       
                    XUartPs_Send(&Uart1, newline, 2);
                    
                    }else{
                        
                    //Check that there is space for the character in the FIFO pipe
                    //while (XUartPs_IsTransmitFull(Uart1.Config.BaseAddress));
                    XUartPs_Send(&Uart1, &c, 1);                        
                    }

                    //Sent to BLE
                    //while (XUartPs_IsTransmitFull(Uart0.Config.BaseAddress));
                    XUartPs_Send(&Uart0, &c, 1);
                }
            }

            if (XUartPs_IsReceiveData(Uart0.Config.BaseAddress)) {
                
                    received = XUartPs_Recv(&Uart0, &c, 1);
                if (received == 1){
                    //while (XUartPs_IsTransmitFull(Uart1.Config.BaseAddress));
                    XUartPs_Send(&Uart1, &c, 1);
                }
            }
            //usleep(4);
        }
    }
}