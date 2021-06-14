// Echoes serial communication over UART and vice versa.
// Koichi approved.
// Designed to work with the RFM95 Feather M0 board.

#include <SPI.h>
#include <RH_RF95.h>
#include <RHReliableDatagram.h>

#define USB_SERIAL_BAUD  115200

#define RFM95_CS  8
#define RFM95_RST 4
#define RFM95_INT 3

// The default transmitter power is 13dBm, using PA_BOOST.
// If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
// you can set transmitter powers from 5 to 23 dBm:
#define RFM95_DFLT_FREQ_MHz     902.5
#define RFM95_DFLT_SF           7
#define RFM95_DFLT_TX_POWER_dBm 15
#define RFM95_DFLT_BW_Hz        500000

#define RH_RF95_MAX_MESSAGE_LEN 128
#define RH_RELIABLE_DATAGRAM_ADDR 0xBB

#define USE_RH_RELIABLE_DATAGRAM true

//------------------------
// Structure Declarations
//------------------------

enum msgType_t
{
  msgType_dataRequest,
  msgType_linkChangeRequest,
  msgType_wakeRequest,
  msgType_sleepRequest
};

enum spreadingFactor_t
{
  spreadingFactor_sf7,
  spreadingFactor_sf8,
  spreadingFactor_sf9,
  spreadingFactor_sf10,
  spreadingFactor_sf11,
  spreadingFactor_sf12
};

uint8_t signalBandwidthTable = [7, 8, 9, 10, 11, 12];

enum signalBandwidth_t
{
  signalBandwidth_125kHz,
  signalBandwidth_250kHz,
  signalBandwidth_500kHz,
  signalBandwidth_625kHz
};

uint32_t signalBandwidthTable = [125000, 250000, 500000, 625000];

enum frequencyChannel_t
{
  frequencyChannel_500kHz_Uplink_0,
  frequencyChannel_500kHz_Uplink_1,
  frequencyChannel_500kHz_Uplink_2,
  frequencyChannel_500kHz_Uplink_3,
  frequencyChannel_500kHz_Uplink_4,
  frequencyChannel_500kHz_Uplink_5,
  frequencyChannel_500kHz_Uplink_6,
  frequencyChannel_500kHz_Uplink_7,
  frequencyChannel_500kHz_Downlink_0,
  frequencyChannel_500kHz_Downlink_1,
  frequencyChannel_500kHz_Downlink_2,
  frequencyChannel_500kHz_Downlink_3,
  frequencyChannel_500kHz_Downlink_4,
  frequencyChannel_500kHz_Downlink_5,
  frequencyChannel_500kHz_Downlink_6,
  frequencyChannel_500kHz_Downlink_7
};

uint16_t frequencyChannelTable [] = {9030, 9046, 9062, 9078, 9094, 9110, 9126, 9142, 9233, 9239, 9245, 9251, 9257, 9263, 9269, 9275};

//-----------------------
// Variable Declarations
//-----------------------

RH_RF95 rf95(RFM95_CS, RFM95_INT);
RHReliableDatagram rhReliableDatagram(rf95, RH_RELIABLE_DATAGRAM_ADDR);

uint8_t txBuf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t txBufIdx = 0;
uint8_t txSrcAddr;
uint8_t txDestAddr;
uint8_t txId;
uint8_t txFlags;
msgType_t txMsgType;

uint8_t rxBuf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t rxBufLen = RH_RF95_MAX_MESSAGE_LEN;
uint8_t rxSrcAddr;
uint8_t rxDestAddr;
uint8_t rxId;
uint8_t rxFlags;
msgType_t rxMsgType;

spreadingFactor_t rf95 SpreadingFactor

char inputChar = 0;

//-----------------------
// Function Declarations
//-----------------------

void forceRadioReset();
void setTxPower         (int8_t TxPower);
void setSpreadingFactor (uint8_t spreadingFactor);
void setBandwidth       (uint32_t bandwidth); 

template <class dataPort_t, class debugPort_t>
uint8_t buildStringFromSerial (dataPort_t* dataPort, debugPort_t* debugPort);
uint8_t buildStringFromSerialInner ();

template <class debugPort_t>
void serviceTx (debugPort_t* debugPort);

template <class debugPort_t>
void serviceRx (debugPort_t* debugPort);

//------------
// Main Setup
//------------

void setup()
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  Serial.begin(USB_SERIAL_BAUD);
  while (!Serial)
  {
    delay(1);
  }
  delay(100);
  Serial.println("Feather LoRa Range Test - Base!");

  forceRadioReset();

  while (!rf95.init())
  {
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  #if (USE_RH_RELIABLE_DATAGRAM > 0)
  while (!rhReliableDatagram.init())
  {
    Serial.println("Manager init failed");
    while (1);
  }
  Serial.println("Manager init OK!");
  #endif // USE_RH_RELIABLE_DATAGRAM

  rf95.setSpreadingFactor(RFM95_DFLT_SF);
  rf95.setSignalBandwidth(RFM95_DFLT_BW_Hz);
  rf95.setTxPower(RFM95_DFLT_TX_POWER_dBm, false);
}

//-----------
// Main Loop
//-----------

void loop() {
  // Transmit a string!
  if (buildStringFromSerial<Serial_, Serial_>(&Serial, &Serial))
  {
    serviceTx<Serial_>(&Serial);
  }
  serviceRx<Serial_>(&Serial);
}

//----------------------
// Function Definitions
//----------------------

void forceRadioReset ()
{
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
}

void setSpreadingFactor (spreadingFactor_t spreadingFactor)
{
  
}

void setBandwidth (signalBandwidth_t bandwidth)
{
  rf95.setSignalBandwidth(signalBandwidthTable[bandwidth]  )
}

void setFrequencyChannel (frequencyChannel_t frequencyChannel)
{
  if (frequencyChannel > 15)
  {
    frequencyChannel = 0;
  }
  if (!rf95.setFrequency(((float)(frequencyChannelTable[frequencyChannel]))/10))
  {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: ");
  Serial.println(RFM95_DFLT_FREQ_MHz);
}

template <class dataPort_t, class debugPort_t>
uint8_t buildStringFromSerial (dataPort_t* dataPort, debugPort_t* debugPort)
{
  uint8_t retval;
  while (dataPort->available())
  {
    inputChar = dataPort->read();
    debugPort->print(char(inputChar));
    retval = buildStringFromSerialInner();
  }
  return retval;
}

uint8_t buildStringFromSerialInner ()
{
  // Add chars to ignore to this.
  if ((inputChar != '\n'
       && inputChar != '\r')
      && txBufIdx < RH_RF95_MAX_MESSAGE_LEN)
  {
    txBuf[txBufIdx] = inputChar;
    txBufIdx++;
  }
  // Special start key
  if (inputChar == '$')
  {
    txBuf[txBufIdx] = 27;
    txBufIdx++;
  }
  // \n triggers sending
  if (inputChar == '\n')
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

template <class debugPort_t>
void serviceTx (debugPort_t* debugPort)
{
  if (txBufIdx > 0)
  {
    debugPort->print("Attempting to transmit: \"");
    for (uint8_t i = 0; i < txBufIdx; i++)
    {
      debugPort->print(char(txBuf[i]));
    }
    debugPort->println('\"');
    rf95.waitCAD();
    #if (USE_RH_RELIABLE_DATAGRAM > 0)
    if (rhReliableDatagram.sendtoWait(txBuf, txBufIdx, 0xEE) == true)
    {
      debugPort->println("Sent successfully & acknowleged!");
    }
    txBufIdx = 0;
    #else // USE_RH_RELIABLE_DATAGRAM
    rf95.send(txBuf, txBufIdx);
    txBufIdx = 0;
    rf95.waitPacketSent();
    #endif  // USE_RH_RELIABLE_DATAGRAM
    debugPort->println("Sent successfully!");
  }
  else
  {
    debugPort->print("Nothing to transmit: TX buffer empty.");
  }
}

template <class debugPort_t>
void serviceRx (debugPort_t* debugPort)
{
  if (
      #if (USE_RH_RELIABLE_DATAGRAM > 0)
      rhReliableDatagram.recvfromAckTimeout(rxBuf,
                                     &rxBufLen,
                                     500,
                                     &rxSrcAddr,
                                     &rxDestAddr,
                                     &rxId,
                                     &rxFlags)
      #else // USE_RH_RELIABLE_DATAGRAM
      rf95.recv(rxBuf, &rxBufLen)
      #endif  // USE_RH_RELIABLE_DATAGRAM
     )
  {
    switch rxBuf[0]
    {
      
      default:
      debugPort->print("Received: \"");
      for (uint8_t i = 0; i < rxBufLen; i++)
      {
        debugPort->print(char(rxBuf[i]));
      }
      debugPort->println("\"");
      rxBufLen = RH_RF95_MAX_MESSAGE_LEN;
      break;
    }
  }
}