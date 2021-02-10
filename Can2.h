//----------------------------------------------------------------------------------------
//  RLG
//  ACAN2515 SETUP as second CAN network Can2 via SPI
//  
//  https://github.com/pierremolinaro/acan2515/blob/master/src/CANMessage.h
//
//  MCP2515 connections:
//    - standard SPI pins for SCK, MOSI and MISO
//    - a digital output for CS
//    - interrupt input pin for INT
//----------------------------------------------------------------------------------------


#include <ACAN2515.h> //mcp2515 SPI CAN

static const byte MCP2515_CS  = 10 ; // CS input of MCP2515 (adapt to your design) 
static const byte MCP2515_INT =  9 ; // INT output of MCP2515 (adapt to your design)


//----------------------------------------------------------------------------------------
//  MCP2515 Driver object
//----------------------------------------------------------------------------------------


ACAN2515 Can2 (MCP2515_CS, SPI, MCP2515_INT) ;


//----------------------------------------------------------------------------------------
//  MCP2515 Quartz frequency
//----------------------------------------------------------------------------------------


static const uint32_t QUARTZ_FREQUENCY = 8 * 1000 * 1000 ; // 8 MHz


//----------------------------------------------------------------------------------------
//  Can2 input variables needed to avoid conflict with FlexCAN definitions...
//----------------------------------------------------------------------------------------


struct can2Frame
{
  uint32_t Can2_id      = 0 ;     // Frame identifier
  bool     Can2_ext     = false ; // false -> standard frame, true -> extended frame
  bool     Can2_rtr     = false ; // false -> data frame, true -> remote frame
  uint8_t  Can2_len     = 0 ;     // Length of data (0 ... 8)
  uint8_t  Can2_d[8]    = {0, 0, 0, 0, 0, 0, 0, 0} ; //data
};



//----------------------------------------------------------------------------------------
// Set up the ACAN2515 module
//----------------------------------------------------------------------------------------


void Can2Setup()
{
  //--- Begin SPI
  SPI.begin () ;
  //--- Configure ACAN2515 CAN bit rate 500 kb/s:
  Serial.println ("Configure ACAN2515") ;
  
  ACAN2515Settings settings (QUARTZ_FREQUENCY, 500 * 1000) ;

  settings.mRequestedMode = ACAN2515Settings::NormalMode ; // Select normal mode
  const uint16_t errorCode = Can2.begin (settings, [] { Can2.isr () ; }) ;

  if (errorCode == 0) {
    Serial.print ("Bit Rate prescaler: ") ;
    Serial.println (settings.mBitRatePrescaler) ;
    Serial.print ("Propagation Segment: ") ;
    Serial.println (settings.mPropagationSegment) ;
    Serial.print ("Phase segment 1: ") ;
    Serial.println (settings.mPhaseSegment1) ;
    Serial.print ("Phase segment 2: ") ;
    Serial.println (settings.mPhaseSegment2) ;
    Serial.print ("SJW: ") ;
    Serial.println (settings.mSJW) ;
    Serial.print ("Triple Sampling: ") ;
    Serial.println (settings.mTripleSampling ? "yes" : "no") ;
    Serial.print ("Actual bit rate: ") ;
    Serial.print (settings.actualBitRate ()) ;
    Serial.println (" bit/s") ;
    Serial.print ("Exact bit rate ? ") ;
    Serial.println (settings.exactBitRate () ? "yes" : "no") ;
    Serial.print ("Sample point: ") ;
    Serial.print (settings.samplePointFromBitStart ()) ;
    Serial.println ("%") ;
  }else{
    Serial.print ("Configuration error 0x") ;
    Serial.println (errorCode, HEX) ;
  }
}

//——————————————————————————————————————————————————————————————————————————————
// function to send data on Can2 sent from SimpBMS main ino
// This approach needed to avoid conflic with FlexCan construts...
//——————————————————————————————————————————————————————————————————————————————
bool Can2Send(can2Frame in)
{
  CANMessage frame ;
  frame.ext     = in.Can2_ext;
  frame.rtr     = in.Can2_rtr;
  frame.id      = in.Can2_id;
  frame.len     = in.Can2_len;
  frame.data[0] = in.Can2_d[0];
  frame.data[1] = in.Can2_d[1];
  frame.data[2] = in.Can2_d[2];
  frame.data[3] = in.Can2_d[3];
  frame.data[4] = in.Can2_d[4];
  frame.data[5] = in.Can2_d[5];
  frame.data[6] = in.Can2_d[6];
  frame.data[7] = in.Can2_d[7];
  
  return Can2.tryToSend (frame);
  //
}
