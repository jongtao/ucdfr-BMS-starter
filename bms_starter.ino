// -------------------------------------------------------------
// CANtest for Teensy 3.1
// by teachop
//
// This test is talking to a single other echo-node on the bus.
// 6 frames are transmitted and rx frames are counted.
// Tx and rx are done in a way to force some driver buffering.
// Serial is used to print the ongoing status.

#include <Metro.h>
#include <FlexCAN.h>
#include <stdint.h>

#define LED 13
#define CAN_LENGTH 8
#define USART_SEND_LENGTH 14

#define BMS_CMD_ID 0x506
#define BMS_START {0x08, 0x5A, 0x40, 0x00, 0x0D, 0x00, 0x00, 0x00}

#define PACK_STATUS 0x188
#define PACK_CONFIG 0x288
#define PACK_STATS 0x308
#define CELL_VOLTAGE 0x388
#define PACK_ACTIVE_DATA 0x408
#define PACK_TEMP_DATA 0x488
#define PACK_TIME 0x508

#define ZERO_BMS_STATUS_CODE_OKAY              	0x0000
#define ZERO_BMS_STATUS_CODE_CHARGER           	0x0001
#define ZERO_BMS_STATUS_CODE_BATT_TEMP_TOO_HIGH	0x0002
#define ZERO_BMS_STATUS_CODE_BATT_TEMP_HIGH    	0x0004
#define ZERO_BMS_STATUS_CODE_BATT_TEMP_TOO_LOW 	0x0008
#define ZERO_BMS_STATUS_CODE_LOW_BATT          	0x0010
#define ZERO_BMS_STATUS_CODE_CRITICAL_BATT     	0x0020
#define ZERO_BMS_STATUS_CODE_IMBALANCE         	0x0040
#define ZERO_BMS_STATUS_CODE_INTERNAL_FAULT     0x0080
#define ZERO_BMS_STATUS_CODE_FETS_CLOSED        0x0100
#define ZERO_BMS_STATUS_CODE_CONTACTOR_CLOSED   0x0200
#define ZERO_CAN_BMS_STATUS_CODE_ISOLATION_FAULT	0x0400
#define ZERO_CAN_BMS_STATUS_CODE_CELL_TOO_HIGH	0x0800
#define ZERO_CAN_BMS_STATUS_CODE_CELL_TOO_LOW		0x1000
#define ZERO_CAN_BMS_STATUS_CODE_CHARGE_HALT		0x2000
#define ZERO_CAN_BMS_STATUS_CODE_FULL						0x4000
#define ZERO_CAN_BMS_STATUS_CODE_INTERNAL_DISABLE	0x8000



typedef struct Pack
{
	uint8_t state_of_charge;
	uint16_t status;
	uint16_t balance_mV;
	//uint16_t capacity_AH;
	uint16_t remaining_AH;
	uint16_t current_A;
	uint8_t temp_C;
	uint32_t voltage_mV;
}; // struct Pack



void get_rx();
void print_pack();
void put_usart();
void inject_test_data();



/* Globals */
static const uint8_t bms_startup[CAN_LENGTH] = BMS_START;
Metro sysTimer = Metro(1);	// milliseconds
FlexCAN CANbus(500000);
static CAN_message_t msg, rxmsg;

int txCount, rxCount;
unsigned int txTimer, rxTimer;

Pack PackData =
{
	.state_of_charge = 0,
	.status = 0,
	.balance_mV = 0,
	//.capacity_AH = 0,
	.remaining_AH = 0,
	.current_A = 0,
	.temp_C = 0,
	.voltage_mV = 0,
};



Pack TestData =
{
	.state_of_charge = 88,
	.status = 0x00cd,
	.balance_mV = 6,
	//.capacity_AH = 4,
	.remaining_AH = 456,
	.current_A = -69,
	.temp_C = 29,
	.voltage_mV = 11208//112082
};



/* Program */
void get_rx()
{
	while (CANbus.read(rxmsg))
	{
		//hexDump( sizeof(rxmsg), (uint8_t *)&rxmsg );
		/*
			 for(int i=0; i<8; i++)
			 Serial.write(rxmsg.buf[i]);
			 Serial.write('\n');
		 */
		//Serial.write(rxmsg.buf[0]);

		switch(rxmsg.id)
		{
			case PACK_STATUS:
				//Serial.print("PACK_STATUS");
				PackData.state_of_charge = rxmsg.buf[0];
				PackData.status =
					(uint16_t)rxmsg.buf[1] | ((uint16_t)rxmsg.buf[2] << 8);
				PackData.balance_mV =
					(uint16_t)rxmsg.buf[5] | ((uint16_t)rxmsg.buf[6] << 8);
				break;
			case PACK_CONFIG:
				//Serial.print("PACK_CONFIG");
				/*
				PackData.capacity_AH = 
					(uint16_t)rxmsg.buf[5] | ((uint16_t)rxmsg.buf[6] << 8);
					*/
				break;
			case PACK_STATS:
				//Serial.print("PACK_STATS");
				break;
			case CELL_VOLTAGE:
				//Serial.print("CELL_VOLTAGE");
				PackData.voltage_mV =
					(uint16_t)rxmsg.buf[3] |
					((uint16_t)rxmsg.buf[4] << 8) |
					((uint16_t)rxmsg.buf[5] << 16) |
					((uint16_t)rxmsg.buf[6] << 32);
				//Serial.print(PackData.voltage_mV);
				break;
			case PACK_ACTIVE_DATA:
				//Serial.print("PACK_ACTIVE_DATA");
				PackData.temp_C = rxmsg.buf[1];
				PackData.current_A = 
					(uint16_t)rxmsg.buf[3] | ((uint16_t)rxmsg.buf[4] << 8);
				PackData.remaining_AH =
					(uint16_t)rxmsg.buf[5] | ((uint16_t)rxmsg.buf[6] << 8);
				break;
			case PACK_TEMP_DATA:
				//Serial.print("PACK_TEMP_DATA");
				break;
			case PACK_TIME:
				//Serial.print("PACK_TIME");
				break;
		}; // Rx message ID

		rxCount++;
	} // while CANbus to be read

	//Serial.write('\n');
} // get_rx()



void print_pack() // print to usb for debug
{
	Serial.print("SOC: ");
	Serial.println(PackData.state_of_charge);
	Serial.print("Status: ");
	Serial.println(PackData.status);
	Serial.print("Balance: ");
	Serial.println(PackData.balance_mV);
	Serial.print("Capacity: ");
	/*Serial.println(PackData.capacity_AH);
	Serial.print("Remaining: ");*/
	Serial.println(PackData.remaining_AH);
	Serial.print("Current: ");
	Serial.println(PackData.current_A);
	Serial.print("Temp: ");
	Serial.println(PackData.temp_C);
	Serial.print("Voltage: ");
	Serial.println(PackData.voltage_mV);
} // print_pack()



void put_usart()
{
	unsigned int i = 0;
	char string[USART_SEND_LENGTH];
	// Put data into string
	string[i++] = PackData.state_of_charge;

	string[i++] = (uint8_t)(PackData.status >> 8);
	string[i++]	= (uint8_t)PackData.status;

	string[i++] = PackData.balance_mV >> 8;
	string[i++] = PackData.balance_mV;

	string[i++] = PackData.remaining_AH >> 8;
	string[i++] = PackData.remaining_AH;

	string[i++] = PackData.current_A >> 8;
	string[i++] = PackData.current_A;

	string[i++] = PackData.temp_C;

	string[i++] = (uint8_t)(PackData.voltage_mV >> 24);
	string[i++] = (uint8_t)(PackData.voltage_mV >> 16);
	string[i++] = (uint8_t)(PackData.voltage_mV >> 8);
	string[i] = (uint8_t)(PackData.voltage_mV);

	// Encode and send
	Serial1.write(0x7E);						// send begin flag

	for(i=0; i<USART_SEND_LENGTH; i++)
	{
		if(string[i] == 0x7E || string[i] == 0x7D)
		{
			Serial1.write(0x7D);							// send escape flag
			Serial1.write(string[i] ^ 0x20);	// send encoded byte
		} // if need to escape
		else
			Serial1.write(string[i]); // Copy buffer to string
	} // for i
} // put_usart()



void inject_usart_test_data()
{
	PackData = TestData;
} // inject_usart_test_data()



void setup(void)
{
	Serial1.begin(9600);
	CANbus.begin();
	pinMode(LED, OUTPUT);
	digitalWrite(LED, 1);
	delay(1000);
	sysTimer.reset();
}



void loop(void)
{
	uint8_t idx = 0;

	if(sysTimer.check())
	{
		if(txTimer)
			--txTimer;

		if(rxTimer)
			--rxTimer;
	} // service software timers based on Metro tick

	if(!rxTimer)
	{
		get_rx();
		//inject_usart_test_data();
		put_usart();
		print_pack();	
	} // if not time-delayed, read CAN messages and print 1st byte

	if (!txTimer) // insert a time delay between transmissions
	{
		digitalWrite(LED, 1);
		txTimer = 100;	//milliseconds
		rxTimer = 3;	// 3 millisecond time delay to force some rx data queue use
		txCount = 6;	// send 6 at a time to force tx buffering
		msg.len = CAN_LENGTH;
		msg.id = BMS_CMD_ID;

		for(idx=0; idx<CAN_LENGTH; idx++)
			msg.buf[idx] = bms_startup[idx];

		while(txCount--)
			CANbus.write(msg);

		digitalWrite(LED, 0);
	} // if !txTimer
} // loop
