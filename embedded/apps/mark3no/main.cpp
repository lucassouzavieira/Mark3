/*
 * Mark3no_Bringup.cpp
 *
 * Created: 6/12/2016 7:56:41 PM
 * Author : moslevin
 */ 

#include <avr/io.h>
#include "mark3.h"
#include "drvUART.h"
#include "rtc.h"
#include "memutil.h"

static Thread clApp1;
static uint8_t au8Stack1[320];

static Thread clApp2;
static uint8_t au8Stack2[320];

static Thread clApp3;
static uint8_t au8Stack3[320];

static Thread clApp4;
static uint8_t au8Stack4[320];

static Thread clApp5;
static uint8_t au8Stack5[320];

static ATMegaUART clUART0;
static uint8_t au8RX0[32];
static uint8_t au8TX0[32];

static ATMegaUART clUART1;
static uint8_t au8RX1[32];
static uint8_t au8TX1[32];

static Semaphore clRTCSem;
static RTC clRTC;

static void App1(void* param)
{
	while(1) {
		PORTA ^= 0x01;
		Thread::Sleep(100);
	}
}

static void App2(void* param)
{
	while(1) {
		PORTA ^= 0x02;
		Thread::Sleep(250);
	}
}

static void WriteString(Driver* pclDriver_, const char* szData_)
{
    uint16_t u16Written = 0;
    uint8_t* src = (uint8_t*)szData_;

    while (*src != '\0')
    {
        u16Written = pclDriver_->Write(1, src++);

        if (!u16Written)
        {
            Thread::Sleep(5);
        }
    }
}

static void App3(void* param)
{
    {
        clRTC.Init(1);
        calendar_t cal = {0};
        cal.u8Day = 27;
        cal.eMonth = MONTH_JUNE;
        cal.u16Year = 2016;
        cal.u8Hour = 20;
        cal.u8Minute = 50;
        cal.u8Second = 0;

        clRTC.SetDateTime(&cal);
    }

    Driver* pclUART = DriverList::FindByPath("/dev/tty0");

	while(1) {
        PORTA ^= 0x04;
        calendar_t myCal;

		clRTCSem.Pend();
        clRTC.AddTime(1);

        clRTC.GetDateTime(&myCal);
        char szBuf[12];

        MemUtil::DecimalToString(myCal.u8Hour, szBuf);
        WriteString(pclUART, szBuf);
        WriteString(pclUART, ":");

        MemUtil::DecimalToString(myCal.u8Minute, szBuf);
        WriteString(pclUART, szBuf);
        WriteString(pclUART, ":");

        MemUtil::DecimalToString(myCal.u8Second, szBuf);
        WriteString(pclUART, szBuf);
        WriteString(pclUART, " - ");

        const char* szDay   = clRTC.GetDayOfWeek();
        WriteString(pclUART, szDay);
        WriteString(pclUART, ", ");

        const char* szMonth = clRTC.GetMonthName();
        WriteString(pclUART, szMonth);
        WriteString(pclUART, " ");

        MemUtil::DecimalToString(myCal.u8Day, szBuf);
        WriteString(pclUART, szBuf);
        WriteString(pclUART, ", ");

        MemUtil::DecimalToString(myCal.u16Year, szBuf);
        WriteString(pclUART, szBuf);
        WriteString(pclUART, "\n");
	}
}

static void App4(void* param)
{
	Driver* pclUART = DriverList::FindByPath("/dev/tty0");
	
	while(1) {
		const char* szStr = "Tommy can you hear me?\r\n"; // 24 chars?
        WriteString(pclUART, szStr);
        Thread::Sleep(300);
	}
}

static void App5(void* param)
{
	Driver* pclUART = DriverList::FindByPath("/dev/tty1");
	
	while(1) {
		const char* szStr = "Ground control to major Tom\r\n"; // 29 chars?
        WriteString(pclUART, szStr);
		Thread::Sleep(750);
	}
}

ISR(TIMER2_OVF_vect)
{
	clRTCSem.Post();
	// TIFR2's TOV2 flag is cleared on interrupt.	
}

static void RTC_Init(void)
{
	// Set up an RTC that expires every second.
	TCCR2A = 0;	 // Normal mode -- count to top @ 0xFF
	TCNT2 = 0;
	
	// Enable overflow interrupt
	TIFR2 = 0;
	TIMSK2 = (1 << TOIE2);

	// Enable asynchronous clock from 32kHz source
	ASSR = (1 << AS2);

	// 1-second expiry (32kHz / 128) = 256 (rollover)
	TCCR2B = (1 << CS22) | (1 << CS20); 		
}

int main(void)
{
	Kernel::Init();
	
	clApp1.Init(au8Stack1, 320, 1, App1, 0);	
	clApp2.Init(au8Stack2, 320, 1, App2, 0);
	clApp3.Init(au8Stack3, 320, 1, App3, 0);
	clApp4.Init(au8Stack4, 320, 1, App4, 0);
	clApp5.Init(au8Stack5, 320, 1, App5, 0);
	
	clApp1.Start();
	clApp2.Start();
	clApp3.Start();
	clApp4.Start();
	clApp5.Start();
	
	DDRA |= 0x07;
	
	// Initialize UART0
	clUART0.Init();
	clUART0.SetName("/dev/tty0");	
	uint8_t u8Identity = 0 ;
	clUART0.Control(CMD_SET_IDENTITY, &u8Identity, 0, 0, 0);
	uint32_t u32Baud = 57600;
	clUART0.Control(CMD_SET_BAUDRATE, &u32Baud, 0, 0, 0);
	clUART0.Control(CMD_SET_BUFFERS, au8RX0, 32, au8TX0, 32);	
	clUART0.Open();
	
	// Initialize UART1
	clUART1.Init();
	clUART1.SetName("/dev/tty1");
	u8Identity = 1 ;
	clUART1.Control(CMD_SET_IDENTITY, &u8Identity, 0, 0, 0);
	clUART1.Control(CMD_SET_BAUDRATE, &u32Baud, 0, 0, 0);
	clUART1.Control(CMD_SET_BUFFERS, au8RX1, 32, au8TX1, 32);		
	clUART1.Open();
	
	DriverList::Add(&clUART0);
	DriverList::Add(&clUART1);
	
	// Initialize the RTC and a semaphore used to signal 1s expiry
	RTC_Init();
	clRTCSem.Init(0, 1);
	
	Kernel::Start();
	
}

