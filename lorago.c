#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"

#define DEVICE "RP2040 LoRa USB Receiver"
#define VERSION "1.0"

#define PIN_MISO			16
#define PIN_CS				17
#define PIN_SCK				18
#define PIN_MOSI			19

#define LORA_DIO0          	22

#define LED_PIN 			25

    // Pins
// const uint cs_pin = 17;
// const uint sck_pin = 18;
// const uint mosi_pin = 19;
// const uint miso_pin = 16;

#define REG_FIFO                    0x00
#define REG_FIFO_ADDR_PTR           0x0D 
#define REG_FIFO_TX_BASE_AD         0x0E
#define REG_FIFO_RX_BASE_AD         0x0F
#define REG_RX_NB_BYTES             0x13
#define REG_OPMODE                  0x01
#define REG_FIFO_RX_CURRENT_ADDR    0x10
#define REG_IRQ_FLAGS               0x12
#define REG_PACKET_SNR              0x19
#define REG_PACKET_RSSI             0x1A
#define REG_RSSI_CURRENT            0x1B
#define REG_DIO_MAPPING_1           0x40
#define REG_DIO_MAPPING_2           0x41
#define REG_MODEM_CONFIG            0x1D
#define REG_MODEM_CONFIG2           0x1E
#define REG_MODEM_CONFIG3           0x26
#define REG_PAYLOAD_LENGTH          0x22
#define REG_IRQ_FLAGS_MASK          0x11
#define REG_HOP_PERIOD              0x24
#define REG_FREQ_ERROR              0x28
#define REG_DETECT_OPT              0x31
#define REG_DETECTION_THRESHOLD     0x37

// MODES
#define RF96_MODE_RX_CONTINUOUS     0x85
#define RF96_MODE_TX                0x83
#define RF96_MODE_SLEEP             0x80
#define RF96_MODE_STANDBY           0x81

#define PAYLOAD_LENGTH              80

// Modem Config 1
#define EXPLICIT_MODE               0x00
#define IMPLICIT_MODE               0x01

#define ERROR_CODING_4_5            0x02
#define ERROR_CODING_4_6            0x04
#define ERROR_CODING_4_7            0x06
#define ERROR_CODING_4_8            0x08

#define BANDWIDTH_7K8               0x00
#define BANDWIDTH_10K4              0x10
#define BANDWIDTH_15K6              0x20
#define BANDWIDTH_20K8              0x30
#define BANDWIDTH_31K25             0x40
#define BANDWIDTH_41K7              0x50
#define BANDWIDTH_62K5              0x60
#define BANDWIDTH_125K              0x70
#define BANDWIDTH_250K              0x80
#define BANDWIDTH_500K              0x90

// Modem Config 2

#define SPREADING_6                 0x60
#define SPREADING_7                 0x70
#define SPREADING_8                 0x80
#define SPREADING_9                 0x90
#define SPREADING_10                0xA0
#define SPREADING_11                0xB0
#define SPREADING_12                0xC0

#define CRC_OFF                     0x00
#define CRC_ON                      0x04


// POWER AMPLIFIER CONFIG
#define REG_PA_CONFIG               0x09
#define PA_MAX_BOOST                0x8F
#define PA_LOW_BOOST                0x81
#define PA_MED_BOOST                0x8A
#define PA_MAX_UK                   0x88
#define PA_OFF_BOOST                0x00
#define RFO_MIN                     0x00

// LOW NOISE AMPLIFIER
#define REG_LNA                     0x0C
#define LNA_MAX_GAIN                0x23  // 0010 0011
#define LNA_OFF_GAIN                0x00


struct TSettings
{
  double Frequency;
  int LoRaMode;
  int ImplicitOrExplicit;
  int ErrorCoding;
  int Bandwidth;
  int SpreadingFactor;
  int LowDataRateOptimize;
} Settings;

int Sending=0;

void SendToHosts(char *Line)
{
	printf("%s\r\n", Line);
}

void ReplyOK(void)
{
  SendToHosts("*");
}

void ReplyBad(void)
{
  SendToHosts("?");
}

void SendVersion(void)
{
	ReplyOK();
	printf("Version=%s\r\n", VERSION);
}

void SendDevice(void)
{
	ReplyOK();
	printf("Device=%s\r\n", DEVICE);
}

void LoadSettings(void)
{
  // int i;
  // unsigned char *ptr;

  // ptr = (unsigned char *)(&Settings);
  // for (i=0; i<sizeof(Settings); i++, ptr++)
  // {
    // *ptr = EEPROM.read(i+2);
  // }
}

void StoreSettings(void)
{
  // int i;
  // unsigned char *ptr;
  
  // Signature
  // EEPROM.write(0, 'D');
  // EEPROM.write(1, 'A');

  // Settings
  // ptr = (unsigned char *)(&Settings);
  // for (i=0; i<sizeof(Settings); i++, ptr++)
  // {
    // EEPROM.write(i+2, *ptr);
  // }

  // EEPROM.commit();
}

void cs_select() 
{
	asm volatile("nop \n nop \n nop");
	sleep_ms(1);
	gpio_put(PIN_CS, 0);
	sleep_ms(1);
	asm volatile("nop \n nop \n nop");
}

void cs_unselect() 
{
	sleep_ms(1);
	asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 1);
	sleep_ms(1);
	asm volatile("nop \n nop \n nop");
}

uint8_t readRegister(uint8_t addr)
{
	uint8_t reg[1], buf[1];	

	reg[0] = addr & 0x7F;	

	cs_select();
	spi_write_blocking(spi0, reg, 1);
	spi_read_blocking(spi0, 0, buf, 1);  
	cs_unselect();
	
	return buf[0];
}

void writeRegister(uint8_t reg, uint8_t value)
{
	uint8_t buf[2];

	buf[0] = reg | 0x80;
	buf[1] = value;
	
	cs_select();
	spi_write_blocking(spi0, buf, 2);
	cs_unselect();
}

double FrequencyReference(void)
{
  switch (Settings.Bandwidth)
  {
    case  BANDWIDTH_7K8:  return 7800;
    case  BANDWIDTH_10K4:   return 10400; 
    case  BANDWIDTH_15K6:   return 15600; 
    case  BANDWIDTH_20K8:   return 20800; 
    case  BANDWIDTH_31K25:  return 31250; 
    case  BANDWIDTH_41K7:   return 41700; 
    case  BANDWIDTH_62K5:   return 62500; 
    case  BANDWIDTH_125K:   return 125000; 
    case  BANDWIDTH_250K:   return 250000; 
    case  BANDWIDTH_500K:   return 500000; 
  }

  return 0;
}

double FrequencyError(void)
{
  int32_t Temp;
  double T;
  
  Temp = (int32_t)readRegister(REG_FREQ_ERROR) & 7;
  Temp <<= 8L;
  Temp += (int32_t)readRegister(REG_FREQ_ERROR+1);
  Temp <<= 8L;
  Temp += (int32_t)readRegister(REG_FREQ_ERROR+2);
  
  if (readRegister(REG_FREQ_ERROR) & 8)
  {
    Temp = Temp - 524288;
  }

  T = (double)Temp;
  T *=  (16777216.0 / 32000000.0);
  T *= (FrequencyReference() / 500000.0);

  return -T;
} 

int receiveMessage(unsigned char *message)
{
	int i, Bytes, currentAddr;

	int x = readRegister(REG_IRQ_FLAGS);
	Bytes = 0;

	printf("Message status = %02Xh\n", x);

	// clear the rxDone flag
	// writeRegister(REG_IRQ_FLAGS, 0x40); 
	writeRegister(REG_IRQ_FLAGS, 0xFF); 

	// check for payload crc issues (0x20 is the bit we are looking for
	if((x & 0x20) == 0x20)
	{
		printf("CRC Failure\n");
		// reset the crc flags
		writeRegister(REG_IRQ_FLAGS, 0x20); 
	}
	else
	{
		printf("Received\n");
		currentAddr = readRegister(REG_FIFO_RX_CURRENT_ADDR);
		Bytes = readRegister(REG_RX_NB_BYTES);
		printf ("%d bytes in packet\n", Bytes);

		writeRegister(REG_FIFO_ADDR_PTR, currentAddr);   
		// now loop over the fifo getting the data
		for(i = 0; i < Bytes; i++)
		{
		  message[i] = (unsigned char)readRegister(REG_FIFO);
		}
		message[Bytes] = '\0';

		// writeRegister(REG_FIFO_ADDR_PTR, 0);  // currentAddr);   
	} 
  
  return Bytes;
}


void CheckRx()
{
#ifdef LED
  if ((LEDOff > 0) && (millis() >= LEDOff))
  {
    gpio_put(LED, 00);
    LEDOff = 0;
  }
#endif
  
	if (gpio_get(LORA_DIO0))
	{
		unsigned char Message[256];
		char Line[20];
		int Bytes, SNR, RSSI, i;
		long Altitude;

		#ifdef LED
			gpio_put(LED, 1);
			LEDOff = millis() + 1000;
		#endif
		
		Bytes = receiveMessage(Message);
		
		sprintf(Line, "FreqErr=%.1lf", FrequencyError()/1000.0);
		SendToHosts(Line);

		SNR = readRegister(REG_PACKET_SNR);
		SNR /= 4;
		
		if (Settings.Frequency > 525)
		{
		  RSSI = readRegister(REG_PACKET_RSSI) - 157;
		}
		else
		{
		  RSSI = readRegister(REG_PACKET_RSSI) - 164;
		}
		
		if (SNR < 0)
		{
		  RSSI += SNR;
		}
		
	//    Serial.print("PacketRSSI="); Serial.println(RSSI);
	//    Serial.print("PacketSNR="); Serial.println(SNR);
	//    SerialBT.print("PacketRSSI="); SerialBT.println(RSSI);
	//    SerialBT.print("PacketSNR="); SerialBT.println(SNR);
		sprintf(Line, "PacketRSSI=%d", RSSI);
		SendToHosts(Line);
		sprintf(Line, "PacketSNR=%d", SNR);
		SendToHosts(Line);
		
		// Serial.print("Packet size = "); Serial.println(Bytes);

		// Telemetry='$$LORA1,108,20:30:39,51.95027,-2.54445,00141,0,0,11*9B74

		if (Bytes == 0)
		{
		  // CRC error
		}
		else if (Message[0] == '$')
		{
		   char ShortMessage[21];
		   char Line[256];
		   
		  // Remove LF
		  Message[strlen((char *)Message)-1] = '\0';
		  
		  sprintf(Line, "Message=%s", Message);
		  SendToHosts(Line);
			  
		}
		else if (Message[0] == '%')
		{
		  char *ptr, *ptr2;

		  // Message[0] = '$';
		  
		  ptr = (char *)Message;
		  do
		  {
			if ((ptr2 = strchr(ptr, '\n')) != NULL)
			{
			  *ptr2 = '\0';
			  
			  sprintf(Line, "Message=%s\r\n", Message);
			  SendToHosts(Line);
			  
			  ptr = ptr2 + 1;
			}
		  } while (ptr2 != NULL);
		}
		else
		{
		  printf("Hex=");

		  for (i=0; i<Bytes; i++)
		  {
			printf("%02X", Message[i]);
		  }
		  printf("\n");
		}
	}
}


/////////////////////////////////////
//    Method:   Change the mode
//////////////////////////////////////
void SetLoRaMode(uint8_t newMode)
{
	static uint8_t currentMode = 0x81;
	
  if(newMode == currentMode)
    return;  
  
  switch (newMode) 
  {
    case RF96_MODE_TX:
      writeRegister(REG_LNA, LNA_OFF_GAIN);  // TURN LNA OFF FOR TRANSMITT
      writeRegister(REG_PA_CONFIG, PA_MAX_UK);
      writeRegister(REG_OPMODE, newMode);
      currentMode = newMode; 
      
      break;
          
    case RF96_MODE_RX_CONTINUOUS:
      writeRegister(REG_PA_CONFIG, PA_OFF_BOOST);  // TURN PA OFF FOR RECIEVE??
      writeRegister(REG_LNA, LNA_MAX_GAIN);  // LNA_MAX_GAIN);  // MAX GAIN FOR RECIEVE
      writeRegister(REG_OPMODE, newMode);
      currentMode = newMode; 
      break;
      
      break;
    case RF96_MODE_SLEEP:
      writeRegister(REG_OPMODE, newMode);
      currentMode = newMode; 
      break;
    case RF96_MODE_STANDBY:
      writeRegister(REG_OPMODE, newMode);
      currentMode = newMode; 
      break;
    default: return;
  } 
  
  if(newMode != RF96_MODE_SLEEP)
  {
    sleep_ms(10);
  }
   
  return;
}

void SetLoRaFrequency()
{
  unsigned long FrequencyValue;
  double Temp;
  
  Temp = Settings.Frequency * 7110656 / 434;
  FrequencyValue = (unsigned long)(Temp);

//  Serial.print("FrequencyValue is ");
//  Serial.println(FrequencyValue);

#ifdef OLED
  display.setCursor(0,10);
  display.print("Freq: ");
  display.print(Settings.Frequency, 4);
  display.print(" MHz");
  display.display();
#endif

  writeRegister(0x06, (FrequencyValue >> 16) & 0xFF);    // Set frequency
  writeRegister(0x07, (FrequencyValue >> 8) & 0xFF);
  writeRegister(0x08, FrequencyValue & 0xFF);
}

void SetLoRaParameters()
{
  writeRegister(REG_MODEM_CONFIG, Settings.ImplicitOrExplicit | Settings.ErrorCoding | Settings.Bandwidth);
  writeRegister(REG_MODEM_CONFIG2, Settings.SpreadingFactor | CRC_ON);
  writeRegister(REG_MODEM_CONFIG3, 0x04 | Settings.LowDataRateOptimize);                  // 0x04: AGC sets LNA gain
  writeRegister(REG_DETECT_OPT, (readRegister(REG_DETECT_OPT) & 0xF8) | ((Settings.SpreadingFactor == SPREADING_6) ? 0x05 : 0x03));  // 0x05 For SF6; 0x03 otherwise
  writeRegister(REG_DETECTION_THRESHOLD, (Settings.SpreadingFactor == SPREADING_6) ? 0x0C : 0x0A);    // 0x0C for SF6, 0x0A otherwise

}

void startReceiving(void)
{
  SetLoRaMode(RF96_MODE_SLEEP);
  writeRegister(REG_OPMODE,0x80);  
  SetLoRaMode(RF96_MODE_SLEEP);

  SetLoRaFrequency();
  
  SetLoRaParameters();

  writeRegister(REG_DIO_MAPPING_1, 0x00 );  // 00 00 00 00 maps DIO0 to RxDone

  writeRegister(REG_PAYLOAD_LENGTH, 255);
  writeRegister(REG_RX_NB_BYTES, 255);
  
  writeRegister(REG_FIFO_RX_BASE_AD, 0);
  writeRegister(REG_FIFO_ADDR_PTR, 0);
  
  // Setup Receive Continous Mode
  SetLoRaMode(RF96_MODE_RX_CONTINUOUS);
}

void setupRFM98(void)
{
    // Initialize CS pin high
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
	
    // gpio_init(LORA_DIO0);
    gpio_set_dir(LORA_DIO0, GPIO_IN);

	// Initialize SPI port at 0.5 MHz
    spi_init(spi0, 500 * 1000);	
	
	gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
	gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
	gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
	gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
	
    // Set SPI format
    spi_set_format( spi0,   // SPI instance
                    8,      // Number of bits per transfer
                    0,      // Polarity (CPOL)
                    0,      // Phase (CPHA)
                    SPI_MSB_FIRST);	
					
    // Workaround: perform throw-away read to make SCK idle high
    // reg_read(spi, cs_pin, REG_DEVID, data, 1);

	startReceiving();
}


void SetParametersFromLoRaMode(int LoRaMode)
{
  Settings.LowDataRateOptimize = 0;

// #ifdef OLED
  // display.setCursor(0,20);
  // display.print("Mode: ");
  // display.print(LoRaMode);
  // display.display();
// #endif
  
  Settings.LoRaMode = LoRaMode;
  
  if (LoRaMode == 7)
  {
    Settings.ImplicitOrExplicit = EXPLICIT_MODE;
    Settings.ErrorCoding = ERROR_CODING_4_5;
    Settings.Bandwidth = BANDWIDTH_20K8;
    Settings.SpreadingFactor = SPREADING_7;
  }
  else if (LoRaMode == 6)
  {
    Settings.ImplicitOrExplicit = IMPLICIT_MODE;
    Settings.ErrorCoding = ERROR_CODING_4_5;
    Settings.Bandwidth = BANDWIDTH_41K7;
    Settings.SpreadingFactor = SPREADING_6;
  }
  else if (LoRaMode == 5)
  {
    Settings.ImplicitOrExplicit = EXPLICIT_MODE;
    Settings.ErrorCoding = ERROR_CODING_4_8;
    Settings.Bandwidth = BANDWIDTH_41K7;
    Settings.SpreadingFactor = SPREADING_11;
  }
  else if (LoRaMode == 4)
  {
    Settings.ImplicitOrExplicit = IMPLICIT_MODE;
    Settings.ErrorCoding = ERROR_CODING_4_5;
    Settings.Bandwidth = BANDWIDTH_250K;
    Settings.SpreadingFactor = SPREADING_6;
  }
  else if (LoRaMode == 3)
  {
    Settings.ImplicitOrExplicit = EXPLICIT_MODE;
    Settings.ErrorCoding = ERROR_CODING_4_6;
    Settings.Bandwidth = BANDWIDTH_250K;
    Settings.SpreadingFactor = SPREADING_7;
  }
  else if (LoRaMode == 2)
  {
    Settings.ImplicitOrExplicit = EXPLICIT_MODE;
    Settings.ErrorCoding = ERROR_CODING_4_8;
    Settings.Bandwidth = BANDWIDTH_62K5;
    Settings.SpreadingFactor = SPREADING_8;

  }
  else if (LoRaMode == 1)
  {
    Settings.ImplicitOrExplicit = IMPLICIT_MODE;
    Settings.ErrorCoding = ERROR_CODING_4_5;
    Settings.Bandwidth = BANDWIDTH_20K8;
    Settings.SpreadingFactor = SPREADING_6;

  }
  else if (LoRaMode == 0)
  {
    Settings.ImplicitOrExplicit = EXPLICIT_MODE;
    Settings.ErrorCoding = ERROR_CODING_4_8;
    Settings.Bandwidth = BANDWIDTH_20K8;
    Settings.SpreadingFactor = SPREADING_11;
    Settings.LowDataRateOptimize = 0x08;
  }
}

void SetFrequency(char *Line)
{
  double Freq;

  Freq = atof(Line);

  if (Freq > 0)
  {
    char Line[20];

    Settings.Frequency = Freq;
    
    StoreSettings();
    
    ReplyOK();

    sprintf(Line, "Frequency=%.3lf", Settings.Frequency);
    SendToHosts(Line);
    
    startReceiving();
  }
  else
  {
    ReplyBad();
  }
}


void SetMode(char *Line)
{
  int Mode;

  Mode = atoi(Line);

  if ((Mode >= 0) && (Mode <= 7))
  {
    char Line[20];
    
    Settings.LoRaMode = Mode;
    
    SetParametersFromLoRaMode(Mode);
    
    StoreSettings();
    
    ReplyOK();

    sprintf(Line, "Mode=%d", Mode);
    SendToHosts(Line);

    startReceiving();
    
  }
  else
  {
    ReplyBad();
  }
}

void SetBandwidth(char *Line)
{
  if (strcmp(Line, "7K8") == 0)
  {
    Settings.Bandwidth = BANDWIDTH_7K8;
    StoreSettings();
    ReplyOK();
    startReceiving();
  }
  else if (strcmp(Line, "10K4") == 0)
  {
    Settings.Bandwidth = BANDWIDTH_10K4;
    StoreSettings();
    ReplyOK();
    startReceiving();
  }
  else if (strcmp(Line, "15K6") == 0)
  {
    Settings.Bandwidth = BANDWIDTH_15K6;
    StoreSettings();
    ReplyOK();
    startReceiving();
  }
  else if (strcmp(Line, "20K8") == 0)
  {
    Settings.Bandwidth = BANDWIDTH_20K8;
    StoreSettings();
    ReplyOK();
    startReceiving();
  }
  else if (strcmp(Line, "31K25") == 0)
  {
    Settings.Bandwidth = BANDWIDTH_31K25;
    StoreSettings();
    ReplyOK();
    startReceiving();
  }
  else if (strcmp(Line, "41K7") == 0)
  {
    Settings.Bandwidth = BANDWIDTH_41K7;
    StoreSettings();
    ReplyOK();
    startReceiving();
  }
  else if (strcmp(Line, "62K5") == 0)
  {
    Settings.Bandwidth = BANDWIDTH_62K5;
    StoreSettings();
    ReplyOK();
    startReceiving();
  }
  else if (strcmp(Line, "125K") == 0)
  {
    Settings.Bandwidth = BANDWIDTH_125K;
    StoreSettings();
    ReplyOK();
    startReceiving();
  }
  else if (strcmp(Line, "250K") == 0)
  {
    Settings.Bandwidth = BANDWIDTH_250K;
    StoreSettings();
    ReplyOK();
    startReceiving();
  }
  else if (strcmp(Line, "500K") == 0)
  {
    Settings.Bandwidth = BANDWIDTH_500K;
    StoreSettings();
    ReplyOK();
    startReceiving();
  }
  else
  {
    ReplyBad();
  }
}

void SetErrorCoding(char *Line)
{
  int Coding;

  Coding = atoi(Line);

  if ((Coding >= 5) && (Coding <= 8))
  {
    Settings.ErrorCoding = (Coding-4) << 1;
    StoreSettings();
    ReplyOK();
    startReceiving();
  }
  else
  {
    ReplyBad();
  }
}

void SetSpreadingFactor(char *Line)
{
  int Spread;

  Spread = atoi(Line);

  if ((Spread >= 6) && (Spread <= 12))
  {
    Settings.SpreadingFactor = Spread << 4;
    StoreSettings();
    ReplyOK();
    startReceiving();
  }
  else
  {
    ReplyBad();
  }
}

void SetImplicit(char *Line)
{
  int Implicit;

  Implicit = atoi(Line);

  Settings.ImplicitOrExplicit = Implicit ? IMPLICIT_MODE : EXPLICIT_MODE;

  StoreSettings();
  
  ReplyOK();
  
  startReceiving();
}

void SetLowOpt(char *Line)
{
  int LowOpt;

  LowOpt = atoi(Line);

  Settings.ImplicitOrExplicit = LowOpt ? 0x08 : 0;

  StoreSettings();

  ReplyOK();
  
  startReceiving();
}

void SendLoRaPacket(char *buffer)
{
	int i, Length;
	uint8_t buf[1];

	#ifdef LED
		digitalWrite(LED, HIGH);
		LEDOff = 0;
	#endif
	Length = strlen(buffer) + 1;

	// Serial.print("Sending "); Serial.print(Length);Serial.println(" bytes");
	SendToHosts("Tx=ON");

	SetLoRaMode(RF96_MODE_STANDBY);

	writeRegister(REG_DIO_MAPPING_1, 0x40);    // 01 00 00 00 maps DIO0 to TxDone
	writeRegister(REG_FIFO_TX_BASE_AD, 0x00);  // Update the address ptr to the current tx base address
	writeRegister(REG_FIFO_ADDR_PTR, 0x00); 
	if (Settings.ImplicitOrExplicit == EXPLICIT_MODE)
	{
		writeRegister(REG_PAYLOAD_LENGTH, Length);
	}
  
	buf[0] = REG_FIFO | 0x80;
	
	cs_select();
	spi_write_blocking(spi0, buf, 1);
	spi_write_blocking(spi0, buffer, Length);
	cs_unselect();

	// go into transmit mode
	SetLoRaMode(RF96_MODE_TX);

	#ifdef OLED
		display.setCursor(0,52);
		display.print("<<< TRANSMITTING >>>");
		display.display();          
	#endif    
  
	Sending = 1;
}

void CheckTx(void)
{
	if (gpio_get(LORA_DIO0))
	{
		Sending = 0;

		#ifdef LED
			digitalWrite(LED, LOW);
			LEDOff = 0;
		#endif

		#ifdef OLED
			  display.setCursor(0,52);
			  display.print("                    ");
			  display.display();          
		#endif    

		SendToHosts("Tx=OFF");

		setupRFM98();

		startReceiving();
	}
}

void TransmitMessage(char *Line)
{
  ReplyOK();

  SendLoRaPacket(Line);
}

void ProcessCommand(char *Line)
{
  char Command;

  Command = Line[1];
  Line += 2;
       
  if (Command == 'F')
  {
    SetFrequency(Line);
  }
  else if (Command == 'M')
  {
    SetMode(Line);
  }
  else if (Command == 'B')
  {
    SetBandwidth(Line);
  }
  else if (Command == 'E')
  {
    SetErrorCoding(Line);
  }
  else if (Command == 'S')
  {
    SetSpreadingFactor(Line);
  }
  else if (Command == 'I')
  {
    SetImplicit(Line);
  }
  else if (Command == 'L')
  {
    SetLowOpt(Line);
  }
  else if (Command == 'D')
  {
    SendDevice();
  }
  else if (Command == 'V')
  {
    SendVersion();
  }
  else if (Command == 'T')
  {
    TransmitMessage(Line);
  }
  else
  {
    ReplyBad();
  }
}


void CheckPC(void)
{
	static char Line[100];
	static int Length=0;
	char Character;

	do
	{
		Character = getchar_timeout_us(0);
		
		if (Character == 0xFF)
		{
		}
		else if (Character == '~')
		{
			Line[0] = Character;
			Length = 1;
		}
		else if (Length >= sizeof(Line))
		{
			Length = 0;
		}
		else if (Length > 0)
		{
			if (Character == '\r')
			{
				Line[Length] = '\0';
				ProcessCommand(Line);
				Length = 0;
			}
			else
			{
				Line[Length++] = Character;
			}
		}
	} while (Character != 0xFF);
}

void UpdateClient(void)
{
	static absolute_time_t UpdateClientAt=0;
	
	if (get_absolute_time() >= UpdateClientAt)
	{
		int CurrentRSSI;
		char Line[20];

		if (Settings.Frequency > 525)
		{
			CurrentRSSI = readRegister(REG_RSSI_CURRENT) - 157;
		}
		else
		{
			CurrentRSSI = readRegister(REG_RSSI_CURRENT) - 164;
		}

		sprintf(Line, "CurrentRSSI=%d", CurrentRSSI);
		SendToHosts(Line);

		UpdateClientAt = make_timeout_time_ms(1000);
	}
}


int main()
{
	bi_decl(bi_program_description("LoRaGo-2040"));
	// bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));

	stdio_init_all();

	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	Settings.Frequency = 434.325;
	Settings.LoRaMode = 1;
	
	SetParametersFromLoRaMode(Settings.LoRaMode);

	setupRFM98();
	
	SendDevice();
	SendVersion();

	while (true)
	{
		CheckPC();

		if (Sending)
		{
			CheckTx();
		}
		else
		{
			CheckRx();

			UpdateClient();
		}
		
		CheckRx();
	}
}
