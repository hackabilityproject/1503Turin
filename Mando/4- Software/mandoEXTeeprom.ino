
#include <IRremote.h> //IR
#include <RCSwitch.h> //RF
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>//LED RGB
#include <avr/power.h>// LED RGB
#include <Wire.h> // to handle external EEPROM
 

// DEFINE 
#define DEBUG 1



#define NUMPIXELS   1  // How many NeoPixels are attached to the Arduino?


// EEPROM
#define totalBytesEEPROM 32000
#define EXT_EEPROM_ADDR 	0x50

#define mask4StatusBlk  0xF0
#define mask4CodeType 0x0F


//flag status of the block
#define INIT_EMPTY	0x0C // 1100
#define INIT_USED 	0x0D // 1101

// flag code type
#define RF_TYPE		0x0A // 1010 10
#define IR_TYPE 	0x05 // 0101  5


// return values of ManageButton
#define COMMAND_SENT            1
#define ERROR_ON_SENDING        2
#define BUTTON_NOT_ASSOCIATED   3


#define PRESSED		LOW // definetely
#define RELEASED 	HIGH
#define DEBOUNCE_TIME	50
#define HOLDON_TIME		5000
#define MAX_WAITING_TIME        300000

#define IR_RAW_DATA 68   
#define IR_PADDING  16	
#define RF_PADDING  68
#define UNION_SIZE  84


// Constants
#define BUTT_RECORD_PIN     12
#define BUTT_PAGE_PIN       17
#define IR_RECV_PIN  11 //ricevtiore IR
#define LED_PIN_ADD 5 // in caso di led addressable rgb, insieme la pin 4
const char myPINs4Button[] = {4,6,7,8,9,14,15,16};
#define NUM_BUTTONS      sizeof(myPINs4Button)

// Pages
#define NUM_PAGES		3

RCSwitch myRcSwitch = RCSwitch();
IRsend myIRsend;//IR
static int indexPage = 0;

typedef struct RFData_t{
	unsigned long codiceRF;
	unsigned int nPulseLength;
        unsigned int len;		
	unsigned int nRepeatTransmit;
	unsigned int nProtocol;			
	byte padding[RF_PADDING];
};

typedef struct IRData_t{
  int count;
  byte rawbuff[IR_RAW_DATA];
  byte padding[IR_PADDING]; 
};


typedef union Data2Send_t{
	RFData_t RFdata;
	IRData_t IRdata;
	byte myByteArray[UNION_SIZE];
};

class handleRemoteButton{
	public:
		handleRemoteButton(int buttonPin,int index)
		{
                        this->buttonPin = buttonPin;
			this->indexButton = index;

		        this->buttonState = RELEASED;         // current state of the button
		        this->lastButtonState = RELEASED;     // previous state of the button
		        this->lastDebounceTime = 0;  // the last time the output pin was toggled
		
		        this->isAssociated = false;
		        this->modeType = 0;
		        this->memStart = 0;	
				

		}

		handleRemoteButton()
		{
		        this->buttonState = RELEASED;         // current state of the button
		        this->lastButtonState = RELEASED;     // previous state of the button
		        this->lastDebounceTime = 0;  // the last time the output pin was toggled
		        this->buttonPin = 0;	
		
		        this->isAssociated = false;
		        this->modeType = 0;
		        this->indexButton = 0;
		        this->memStart = 0;	

		}
		
		boolean isAssociatedWith()
		{
			return this->isAssociated;
		}

		int getIndex()
		{
			return this->indexButton;
		}
		
		void setModeType(char modeType)
		{
			this->modeType = modeType;
		}
		void setCurrPage(char page)
		{
			this->currPage = page;
		}
		char getCurrPage()
		{
			return(this->currPage);
		}
		
		void setMemStart(unsigned long memStart)
		{
		  this->memStart = memStart;
		}
		
		void setAssociatedWith(boolean val)
		{
		  this->isAssociated = val;
		}
		                
                void setPIN(int buttonPin)		
		{
                        this->buttonPin = buttonPin;
                }

                void setIndex(int index)
                {
                        this->indexButton = index;
                }                
		boolean IsPressed()
		{
			boolean isPressed = false;
			
			// read the state of the switch into a local variable:
			int reading = digitalRead(this->buttonPin);
                        //Serial.println("IsPressed");
			// check to see if you just pressed the button 
			// (i.e. the input went from LOW to HIGH),  and you've waited 
			// long enough since the last press to ignore any noise:  

			// If the switch changed, due to noise or pressing:
			if (reading != lastButtonState) {
				// reset the debouncing timer
				lastDebounceTime = millis();
			} 

			if ((millis() - lastDebounceTime) > DEBOUNCE_TIME) {
				// whatever the reading is at, it's been there for longer
				// than the debounce delay, so take it as the actual current state:

				// if the button state has changed:
				if (reading != buttonState) {
					buttonState = reading;

					// only TRUEif the new button state is HIGH
					if (buttonState == PRESSED) {
						return(true);
					}
				}
			}

			// save the reading.  Next time through the loop,
			// it'll be the lastButtonState:
			lastButtonState = reading;
			return isPressed;
		}
		
		boolean IsHeldOn()
		{			
			boolean isPressed = false;

			
			long now = (long)millis();      // get current time
			// read the state of the switch into a local variable:
			int reading = digitalRead(this->buttonPin);
 
			// If the switch changed, due to noise or pressing:
			if (reading != lastButtonState) {
				// reset the debouncing timer
				lastDebounceTime = millis();
			} 


			// debounce the button (Check if a stable, changed state has occured)
			if (now - lastDebounceTime  > DEBOUNCE_TIME && reading != buttonState)
			{
				buttonState = reading;
			}


			// Check for "long click"
			if ((buttonState== PRESSED) && (now - lastDebounceTime > HOLDON_TIME))
			{
				return true;
			}

			lastButtonState = reading;
			return isPressed;
		}
		
		boolean IsLongPressed()
		{
			boolean isPressed = false;
			
			// read the state of the switch into a local variable:
			int reading = digitalRead(this->buttonPin);
                        //Serial.println("IsPressed");
			// check to see if you just pressed the button 
			// (i.e. the input went from LOW to HIGH),  and you've waited 
			// long enough since the last press to ignore any noise:  

			// If the switch changed, due to noise or pressing:
			if (reading != lastButtonState) {
				// reset the debouncing timer
				lastDebounceTime = millis();
			} 

			if ((millis() - lastDebounceTime) > DEBOUNCE_TIME) {
				// whatever the reading is at, it's been there for longer
				// than the debounce delay, so take it as the actual current state:

				// only TRUEif the new button state is HIGH
				if (reading == PRESSED) {
					return(true);
				}
			}

			// save the reading.  Next time through the loop,
			// it'll be the lastButtonState:
			lastButtonState = reading;
			return isPressed;
		}
		
		
		boolean SendCommand()
		{
			Data2Send_t myData2Send;
			int memStart = this->memStart + 1;
			// get data from EEPROM and store bytes in array
		Serial.println("SendCommand:EEPROM ");	
for(int i= 0;i<UNION_SIZE; i++)
			{
			  myData2Send.myByteArray[i] =eeprom_read_byte(memStart + i);
                          Serial.println(myData2Send.myByteArray[i],HEX);
                          Serial.println(i);
                          
			}
                        ledOn(getColor4ButtIndex(indexPage)); 
			switch(this->modeType)
			{
				case RF_TYPE:
					// Transmitter is connected to Arduino Pin #10  
					myRcSwitch.enableTransmit(10);
					// Optional set pulse length.
					myRcSwitch.setPulseLength(myData2Send.RFdata.nPulseLength);

					// Optional set protocol (default is 1, will work for most outlets)
					myRcSwitch.setProtocol(myData2Send.RFdata.nProtocol);

					// Optional set number of transmission repetitions.
					myRcSwitch.setRepeatTransmit(myData2Send.RFdata.nRepeatTransmit);

					myRcSwitch.send(myData2Send.RFdata.codiceRF, myData2Send.RFdata.len);
					delay(1000);  
					myRcSwitch.send(myData2Send.RFdata.codiceRF, myData2Send.RFdata.len);
					#if defined DEBUG
					Serial.println("SendCommand:RF");
					Serial.println("SendCommand:RF PulseLength");
					Serial.println(myData2Send.RFdata.nPulseLength,HEX);
					Serial.println("SendCommand:RF nProtocol");
					Serial.println(myData2Send.RFdata.nProtocol);
					Serial.println("SendCommand:RF nRepeatTransmit");
					Serial.println(myData2Send.RFdata.nRepeatTransmit);
					Serial.println("SendCommand:RF codiceRF");
					Serial.println(myData2Send.RFdata.codiceRF);
					Serial.println("SendCommand:RF len");
					Serial.println(myData2Send.RFdata.len);
					delay(5000);

                                        #endif
                                        return true;
 				case IR_TYPE:
					// regenerate the code
					unsigned int rawData2Send[IR_RAW_DATA];
					for(int i=0;i<myData2Send.IRdata.count;i++)
					{
						rawData2Send[i] = myData2Send.IRdata.rawbuff[i]*USECPERTICK; /// DIEGO fai tu.
					}
					
                                        myIRsend.sendRaw(rawData2Send,myData2Send.IRdata.count,38);  //vettore, numero valori, frequenza in kHz
					delay(250);
                                        return true;
				default:
					return false;
			}
			return false;
		}
		
		boolean buttonIsSelected(int *selectedButt)
		{
			if(this->IsPressed())
			{
				*selectedButt = this->indexButton;
				return true;
			}
			return false;
		}

		int manageButton()
		{
		//                 	Serial.println("manageButton:index");
		//                 	Serial.println(this->getIndex());

			if(this->IsLongPressed())
			{
//			Serial.println("manageButton:index");
//			Serial.println(this->getIndex());
		//                                Serial.println("manageButton:isAssociatedWith");
		//                                 Serial.println(this->isAssociated);
			// check if button is already configured
				if(this->isAssociatedWith())
				{
					// send command
//					Serial.println("manageButton:SendCommand");
					if(this->SendCommand())
					{
						// command has been sent
						// manage LED
											return COMMAND_SENT;
					}
					else
					{
						Serial.println("manageButton:ERROR SendCommand");
						// command has NOT been sent
						return ERROR_ON_SENDING;
					}
				}
				else
				{
  // button not configured
							return BUTTON_NOT_ASSOCIATED;
				}
			}
			//                        Serial.println("manageButton:NOT pressed");
			return 0;
		}
	private:
		// Variables will change:
		int buttonState;         // current state of the button
		int lastButtonState;     // previous state of the button
		// the following variables are long's because the time, measured in miliseconds,
		// will quickly become a bigger number than can be stored in an int.
		long lastDebounceTime;  // the last time the output pin was toggled
		int buttonPin;	
		
		boolean isAssociated;
		char modeType;
		char currPage;
		int indexButton;
		unsigned long memStart;
};


// GlobalVariables
//Data2Send_t tmp;
IRrecv irrecv(IR_RECV_PIN); //istanzio un oggetto della classe IRrecv (costruttore)
decode_results results; // results= struttura che ha le caratteristiche del tipo decode_results 
//RCSwitch mySwitch = RCSwitch();
//IRsend irsend;//IR

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_PIN_ADD, NEO_GRB + NEO_KHZ800);

// ALTERNATIVA: ARRAY di oggetti di tipo handleButton
handleRemoteButton myButtonsArray[NUM_BUTTONS]; 
handleRemoteButton myButtonRec = handleRemoteButton(BUTT_RECORD_PIN,22);
handleRemoteButton myButtonPage = handleRemoteButton(BUTT_PAGE_PIN,23);

int delayval = 100; // delay for half a second
long g = pixels.Color(255, 0, 0);  //green
long r = pixels.Color(0, 255, 0);  //red
long b = pixels.Color(0, 0, 255); //blue
long y = pixels.Color(255, 255, 0); //yellow

// EEPROM section
// Variables
const int numSect  = NUM_BUTTONS*NUM_PAGES;
const int sizeSect = (totalBytesEEPROM/NUM_PAGES)/NUM_BUTTONS; // Byte



// EEPROM section
// Functions
void i2c_eeprom_write_byte( int deviceaddress, unsigned int eeaddress, byte data ) {
    Wire.beginTransmission(deviceaddress);
    Wire.write((int)(eeaddress >> 8)); // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.write(data);
    Wire.endTransmission();
    delay(5);
  }

// WARNING: address is a page address, 6-bit end will wrap around
// also, data can be maximum of about 30 bytes, because the Wire library has a buffer of 32 bytes
void i2c_eeprom_write_page( int deviceaddress, unsigned int eeaddresspage, byte* data, byte length ) {
Wire.beginTransmission(deviceaddress);
Wire.write((int)(eeaddresspage >> 8)); // MSB
Wire.write((int)(eeaddresspage & 0xFF)); // LSB
byte c;
for ( c = 0; c < length; c++)
  Wire.write(data[c]);
Wire.endTransmission();
}

byte i2c_eeprom_read_byte( int deviceaddress, unsigned int eeaddress ) {
byte rdata = 0xFF;
Wire.beginTransmission(deviceaddress);
Wire.write((int)(eeaddress >> 8)); // MSB
Wire.write((int)(eeaddress & 0xFF)); // LSB
Wire.endTransmission();
Wire.requestFrom(deviceaddress,1);
if (Wire.available()) rdata = Wire.read();
return rdata;
}

// maybe let's not read more than 30 or 32 bytes at a time!
void i2c_eeprom_read_buffer( int deviceaddress, unsigned int eeaddress, byte *buffer, int length ) {
Wire.beginTransmission(deviceaddress);
Wire.write((int)(eeaddress >> 8)); // MSB
Wire.write((int)(eeaddress & 0xFF)); // LSB
Wire.endTransmission();
Wire.requestFrom(deviceaddress,length);
int c = 0;
for ( c = 0; c < length; c++ )
  if (Wire.available()) buffer[c] = Wire.read();
}

void eeprom_write_byte(unsigned int eeaddress, byte data)
{
	i2c_eeprom_write_byte(EXT_EEPROM_ADDR, eeaddress, data );
}  

void eeprom_write_buffer(unsigned int eeaddresspage, byte* data, byte length)
{
	i2c_eeprom_write_page( EXT_EEPROM_ADDR, eeaddresspage, data, length );
}  

byte eeprom_read_byte(unsigned int eeaddress )
{
	return(i2c_eeprom_read_byte(EXT_EEPROM_ADDR, eeaddress));
}  
void eeprom_read_buffer(unsigned int eeaddress, byte *buffer, int length )
{
	i2c_eeprom_read_buffer( EXT_EEPROM_ADDR, eeaddress, buffer, length );
}  

boolean scanEEPROM()
{
	int FirstBytePos = 0;
	byte FirstByteData = 0; 
	boolean toReset = false;
	for(int i = 0;i<numSect;i++)
	{
		FirstByteData = eeprom_read_byte(FirstBytePos + (i*sizeSect));
//                Serial.println(FirstByteData,HEX);
//                Serial.println(FirstBytePos + (i*sizeSect));
//                Serial.println((FirstByteData & mask4StatusBlk)>>4,HEX);
		// check the status of the block
		switch((FirstByteData & mask4StatusBlk)>>4)
		{
			case INIT_EMPTY:
				break;
			case INIT_USED:
				break;
			default:
				toReset = true;		
				break;
		}
//                Serial.println("scanEEPROM: toReset");
//               Serial.println(toReset); 
//               Serial.println("scanEEPROM: i");
//               Serial.println(i); 
	}
	return toReset;
       // delay(10000);
}

void resetEEPROM()
{
	int FirstBytePos = 0;
        byte data = 0x00;
        // reset all the data
	for(unsigned int i = 0;i<numSect;i++)
	{
          eeprom_write_byte(FirstBytePos + (i*sizeSect), data);
	}
	// set the first Byte
	data = (0x00 | INIT_EMPTY)<<4;
	for(unsigned int i = 0;i<numSect;i++)
	{
          eeprom_write_byte(FirstBytePos + (i*sizeSect), data);
	}
}

// match color to button index
long getColor4ButtIndex(int indexPage)
{
    switch(indexPage)
    {
      case 0:
      return pixels.Color(255, 0, 0);
      case 1:
      return pixels.Color(0, 255, 0);
      case 2:
      return pixels.Color(0, 0, 255);
      default:
      return pixels.Color(0, 0, 0);
    }    
}
boolean buttonIsSelected(int *selectedButt)
{
	boolean done = false;
	int i = 0;
	while(i<NUM_BUTTONS && !done)
	{
		if(myButtonsArray[i].buttonIsSelected(selectedButt))
		{
			done = true;
		}
		i++;
	}
	return done;
}

void updateHeaderEEPROM(unsigned long memStart, char modeType)
{
    byte data = (0x00 | INIT_USED)<<4;
    byte data1 = (0x00 | modeType);
//    Serial.println("updateHeaderEEPROM: data");
//    Serial.println(data);
//    Serial.println("updateHeaderEEPROM: data1");
//    Serial.println(data1);
//    Serial.println("updateHeaderEEPROM: written");
//    Serial.println(data|data1);
//    Serial.println("updateHeaderEEPROM: memStart");
//    Serial.println(memStart);
//    delay(5000);
    eeprom_write_byte(memStart, data|data1);   
}
/////// RF RECEIVE //////////////////////////////////////////////////////////////////////////////////////////////////// 
static char * dec2binWzerofill(unsigned long Dec, unsigned int bitLength){
  static char bin[64]; 
  unsigned int i=0;

  while (Dec > 0) {
    bin[32+i++] = (Dec & 1 > 0) ? '1' : '0';
    Dec = Dec >> 1;
  }

  for (unsigned int j = 0; j< bitLength; j++) {
    if (j >= bitLength - i) {
      bin[j] = bin[ 31 + i - (j - (bitLength - i)) ];
    }else {
      bin[j] = '0';
    }
  }
  bin[bitLength] = '\0';
  
  return bin;
} 
 
 
void output(unsigned long decimal, unsigned int length, unsigned int delay, unsigned int* raw, unsigned int protocol) {
  if (decimal == 0) {
    Serial.print("Unknown encoding.");
  } else {
    char* b = dec2binWzerofill(decimal, length);
    Serial.print("Decimal: ");
    Serial.print(decimal);
    Serial.print(" (");
    Serial.print( length );
    Serial.print("Bit) Binary: ");
    Serial.print( b );
    Serial.print(" Tri-State: ");
    Serial.print( bin2tristate( b) );
    Serial.print(" PulseLength: ");
    Serial.print(delay);
    Serial.print(" microseconds");
    Serial.print(" Protocol: ");
    Serial.println(protocol);
  }
  
  Serial.print("Raw data: ");
  for (int i=0; i<= length*2; i++) {
    Serial.print(raw[i]);
    Serial.print(",");
  }
  Serial.println();
  Serial.println();
}


static char* bin2tristate(char* bin) {
  char returnValue[50];
  int pos = 0;
  int pos2 = 0;
  while (bin[pos]!='\0' && bin[pos+1]!='\0') {
    if (bin[pos]=='0' && bin[pos+1]=='0') {
      returnValue[pos2] = '0';
    } else if (bin[pos]=='1' && bin[pos+1]=='1') {
      returnValue[pos2] = '1';
    } else if (bin[pos]=='0' && bin[pos+1]=='1') {
      returnValue[pos2] = 'F';
    } else {
      return "not applicable";
    }
    pos = pos+2;
    pos2++;
  }
  returnValue[pos2] = '\0';
  return returnValue;
} 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 

boolean autoLearning(int indexSelectedButt, int modeType)
{
	unsigned long memStart  = 0;
	boolean done = false;
	//	delay(500);
	//        Serial.println("autoLearning: indexSelectedButt");
	//        Serial.println(indexSelectedButt);
	//        Serial.println("autoLearning: modeType");
	//        Serial.println(modeType);
	memStart = (indexPage * (NUM_BUTTONS * sizeSect)) + (indexSelectedButt * sizeSect);
	if(modeType == IR_TYPE){
		// create struct
		Data2Send_t tmpStruct;
		tmpStruct.IRdata.count = results.rawlen;
		for(int k=0;k<results.rawlen;k++)
		{
			tmpStruct.IRdata.rawbuff[k] = (byte)results.rawbuf[k+1];
		}
       Serial.println("autoLearning: results.rawbuf");
       Serial.println(results.rawlen);
		for(int k=0;k<results.rawlen;k++)
		{
			Serial.println(results.rawbuf[k]);
		}
       
	        
//        Serial.println("autoLearning: sizeSect");
//        Serial.println(sizeSect,DEC);
//        Serial.println("autoLearning: indexSelectedButt");
//        Serial.println(indexSelectedButt,DEC);
      
		// copy in EEPROM
//        Serial.println("autoLearning: memStart");
//        Serial.println(memStart);
		updateHeaderEEPROM(memStart, IR_TYPE);
		//eeprom_write_buffer(memStart+ 1 , tmpStruct.myByteArray, UNION_SIZE);
		for (int i = 0; i < UNION_SIZE; i++) {
			EEPROM.write(memStart+ 1 + i, tmpStruct.myByteArray[i]); // (cella memoria, dato), i dati del vettore sono minori di 256
		}
//                delay(1000);
//                byte data;
//                Serial.println("IR Stored"); 
// 		for (int i = 0; i < UNION_SIZE+1; i++) {
//			data = EEPROM.read(memStart +i); // (cella memoria, dato), i dati del vettore sono minori di 256
//                        Serial.println(data,HEX); 
//		}
//               

		// configure the button 
		ConfigureButton(indexSelectedButt, memStart, IR_TYPE,true);

//		Serial.println();   
		done = true;
	}  
	else if(modeType == RF_TYPE)  {
		output(myRcSwitch.getReceivedValue(), myRcSwitch.getReceivedBitlength(), myRcSwitch.getReceivedDelay(), myRcSwitch.getReceivedRawdata(),myRcSwitch.getReceivedProtocol());
		unsigned long codiceRF= myRcSwitch.getReceivedValue();

		//numero di bit (1 byte), pulse length (2 byte), protocollo (1 byte), codiceRF (3 byte)= 7 byte. Per adesso registro solo il codiceRF (3 byte) 

		// create struct
		Data2Send_t tmpStruct;
		tmpStruct.RFdata.nPulseLength = myRcSwitch.getReceivedDelay();
		tmpStruct.RFdata.nProtocol = myRcSwitch.getReceivedProtocol();
		tmpStruct.RFdata.nRepeatTransmit = 4; // mha??
		tmpStruct.RFdata.codiceRF= myRcSwitch.getReceivedValue();
		tmpStruct.RFdata.len= myRcSwitch.getReceivedBitlength();
		
		// copy in EEPROM
		updateHeaderEEPROM(memStart, RF_TYPE);
		//eeprom_write_buffer(memStart+ 1, tmpStruct.myByteArray, UNION_SIZE);
		for (int i = 0; i < UNION_SIZE; i++) {
			EEPROM.write(memStart+ 1 +i, tmpStruct.myByteArray[i]); // (cella memoria, dato), i dati del vettore sono minori di 256
		}

		// configure the button 
		ConfigureButton(indexSelectedButt, memStart, RF_TYPE,true);
		  
		done = true;
		Serial.println(memStart);                 
	}
	return done;
}

boolean ConfigureButton(int indexSelectedButt, unsigned long memStart, char modeType, bool Associated)
{
	int i=0;
	boolean done = false;
	while(i<NUM_BUTTONS && !done)
	{
		if(indexSelectedButt == myButtonsArray[i].getIndex())
		{
			done = true;
			// set mode type
			myButtonsArray[i].setModeType(modeType);
			// set command to send
			myButtonsArray[i].setMemStart(memStart);
			// set associated
			myButtonsArray[i].setAssociatedWith(Associated);
			// set page
			myButtonsArray[i].setCurrPage(indexPage);
		}
                i++;
	}
	return done;	
}


void PreloadButtonsFromEEPROM()
{
   Serial.println("PreloadButtonsFromEEPROM"); 
    int FirstBytePos = indexPage *(sizeSect * NUM_BUTTONS);
    byte FirstByteData = 0; 
    unsigned long memStart = 0;
    int modeType = 0;
  // we assume the number of buttons is equal to the Mem sections
  for(int i=0;i<NUM_BUTTONS;i++)
  {
      FirstByteData = eeprom_read_byte(FirstBytePos + (i*sizeSect));
      Serial.println(FirstByteData,HEX);
      Serial.println(FirstBytePos + (i*sizeSect));
      // check the status of the block
      switch((FirstByteData & mask4StatusBlk)>>4)
      {
		case INIT_EMPTY:
			ConfigureButton(i,0,0,false);
                        break;
		case INIT_USED:
			memStart = FirstBytePos + (i*sizeSect);
      Serial.println("memStart");
      Serial.println(memStart);
			// check mode type
			switch((FirstByteData & mask4CodeType))
			{
				case RF_TYPE:
					ConfigureButton(i, memStart, RF_TYPE,true);
				break;
				case IR_TYPE:
					ConfigureButton(i, memStart, IR_TYPE,true);
				break;
				default:
					// error
					Serial.println("ERROR: used but mode type not recognized");
					delay(2000);
					// force to init_empty
					byte data = (0x00 | INIT_EMPTY)<<4;
					eeprom_write_byte(FirstBytePos + (i*sizeSect), data); 
			                SlowBlinkAddrLED(3,getColor4ButtIndex(indexPage));
					break;
			}
			break;
		default:
		break;
	}
  }
}
////////////////LED///////////////////////
void ledOn (long colore){
  pixels.setPixelColor(0, colore); // Moderately bright green color.
  pixels.show(); // This sends the updated pixel color to the hardware.
}

void ledOff (){
  pixels.setPixelColor(0, 0); // Moderately bright green color.
  pixels.show(); // This sends the updated pixel color to the hardware.
}

void FastBlinkAddrLED(long period, long color)
{
    long count = 0;
    while(count < (period*10)) // seconds * 10 
    {
      ledOn (color);
      delay(50);
      ledOff ();
      delay(50);
      count ++;  // 100 ms  
    }
    ledOff ();
}
// period in sec 
void SlowBlinkAddrLED(long period, long color)
{
    long count = 0;
    while(count < (period)) // seconds 
    {
      ledOn (color);
      delay(500);
      ledOff();
      delay(500);
      count ++;  // 1000 ms  
    }
    ledOff();
}

// period in sec 
//void FastBlinkingLED(long period, int PIN)
//{
//    long count = 0;
//    while(count < (period*10)) // seconds * 10 
//    {
//      digitalWrite (LED_PIN, HIGH);
//      delay(50);
//      digitalWrite (LED_PIN, LOW);
//      delay(50);
//      count ++;  // 100 ms  
//    }
//    digitalWrite (LED_PIN, LOW);
//}

// period in sec 
//void SlowBlinkingLED(long period, int PIN)
//{
//    long count = 0;
//    while(count < (period)) // seconds 
//    {
//      digitalWrite (LED_PIN, HIGH);
//      delay(500);
//      digitalWrite (LED_PIN, LOW);
//      delay(500);
//      count ++;  // 1000 ms  
//    }
//    digitalWrite (LED_PIN, LOW);
//}

void intro (){
  ledOn(g);
  delay(delayval); // Delay for a period of time (in milliseconds).
  ledOn(r);
  delay(delayval); // Delay for a period of time (in milliseconds).
  ledOn(b);
  delay(delayval); // Delay for a period of time (in milliseconds).
  ledOn(y);
  delay(delayval); // Delay for a period of time (in milliseconds).
  ledOff();
  delay(500);
  ledOn(getColor4ButtIndex(indexPage));
  delay(500); // Delay for a period of time (in milliseconds).
  ledOff();
}

void setup()
{
    Serial.begin(9600);
    Wire.begin(); //inizializzo la porta I2C della Arduino
    for(int i=0; i<NUM_BUTTONS; i++)
    {
      myButtonsArray[i].setIndex(i);
      myButtonsArray[i].setPIN(myPINs4Button[i]);
      pinMode(myPINs4Button[i], INPUT_PULLUP);
    }
	
	
	// scan EEPROM
	if(scanEEPROM())
	{
		Serial.println("Reset EEPROM");
                //delay(5000);
                // reset the EEPROM
		resetEEPROM();
	}
	else
	{
		// load the button
        PreloadButtonsFromEEPROM();
	}
	// config PINs
        
	pinMode(BUTT_RECORD_PIN, INPUT_PULLUP);
	pinMode(BUTT_PAGE_PIN, INPUT_PULLUP);
	// disabilitare interrupt
	myRcSwitch.disableReceive();
	/////////////LED/////////////
	pixels.begin(); // This initializes the NeoPixel library.
        pixels.setPixelColor(0, 0); 
	pixels.show(); // Initialize all pixels to 'off'
	////////////////////////////   
        intro();
}

void manageReturnButton(int retVal, int index)
{
 switch(retVal)
 {
   case COMMAND_SENT:
     //digitalWrite (LED_PIN_ADD, HIGH);
     //ledOn(getColor4ButtIndex(indexPage)); moved to sendCommand
     //delay(250);
     ledOff();
     break;
   case ERROR_ON_SENDING:
     FastBlinkAddrLED(5,getColor4ButtIndex(indexPage));
 break;
   case BUTTON_NOT_ASSOCIATED:
     //SlowBlinkAddrLED(5,getColor4ButtIndex(indexPage));
   break;
   default:
   break;
 }
}  

void manageReturnButton()
{
    for(int i=0; i<NUM_BUTTONS; i++)
    {
      manageReturnButton(myButtonsArray[i].manageButton() , myButtonsArray[i].getIndex());
    }
}

void loop() 
{
	static boolean RecInProgress =false;
        if(!RecInProgress)
	{
//      	      Serial.println("premi ricezione");
//      	      Serial.println("RecInProgress");
//      	      Serial.println(RecInProgress);
              //delay(1000);
		// check record
		if(myButtonRec.IsHeldOn())
		{
			RecInProgress = true;
			// Switch on the LED
      		        // Switch on the LED -> necessario??
			ledOn (getColor4ButtIndex(indexPage));
			Serial.println("Premuto pulsante rec");
			//delay(1000);
		}
                else if(myButtonPage.IsPressed())
                {
                          indexPage++;
                          if(indexPage > (NUM_PAGES-1)){
                            indexPage = 0;
                          }
						  // preload the button related to the page
						  PreloadButtonsFromEEPROM();
			  ledOn (getColor4ButtIndex(indexPage));
                          delay(100);
                          ledOff();

                }
		else
		{
			// scan all the other buttons
			manageReturnButton();
 		}
	}
	else //if(RecInProgress)
	{
		Serial.println("Sono in attesa della pressione di un pulsante");
                char selected = 0;
		unsigned long count4AutoExit = 0; //
		int indexButt = 0;

		// seleziono bottone
		while(!selected)
		{
			// Waiting for a button to configure
			if(buttonIsSelected(&indexButt))
			{
				selected = 1;
			}
                  
			count4AutoExit++;
			if(count4AutoExit > MAX_WAITING_TIME)
			{
			  // exit the loop -> no button has been associated  
			  selected = 2;
			}
		}
		if(selected == 2)
		{
			// fast blink for 10 secs
			//FastBlinkingLED(10,LED_PIN);
                        FastBlinkAddrLED(10, getColor4ButtIndex(indexPage));
			// reset the flag
			RecInProgress = false;
		}
		else
                {
			Serial.println("Button Selected");
			Serial.println(indexButt);
			// LED slow blinking
			//SlowBlinkAddrLED(5,getColor4ButtIndex(indexPage));
			//abilito interrupt del IR e RF
			// IR
			irrecv.enableIRIn(); // Start the receiver
			// RF
			myRcSwitch.enableReceive(0);  // Rf Receiver on inerrupt 0 => that is pin #2
			// autolearning
			char ricevuto = 0;
			int modeType = 0;
			int counter = 0;
			const int maxTime = 10000;
                        delay(500);
			while(!ricevuto)
			{
				ledOn (getColor4ButtIndex(indexPage));
                                //IR
				//delay(2000);
				//if(indexButt ==1) { // DEBUG ONLY
				if (irrecv.decode(&results)) { //metodo dell'istanza della classe IRrcev. Results è una struttura (matrice che può non essere omogenea (capisco che è una struttura perchè dopo utilizzo il simbolo ->
							   //decode è dichiarato all'interno della libreria
							   //dal momento che sto passando un puntatore, dico a decode di scrivere negli indirizzi in essa contenuti
								// se avessi avuto delle variabili, sarebbero state di input per il metodo

				Serial.println("Ho ricevuto IR");
				modeType = IR_TYPE;
				ricevuto=1;  //per poi uscire dal while
				}
				//else if(indexButt ==0) // DEBUG ONLY
				else if(myRcSwitch.available())
				{
					Serial.println("Ho ricevuto RF");
					modeType = RF_TYPE;
					ricevuto=1;  //per poi uscire dal while                        
				}
				counter++;
				Serial.println(counter);
                                if(counter > maxTime)
				{
					ricevuto=2;  //per poi uscire dal while
					Serial.println("ricevuto=2");
				}
                                delay(100);
                                ledOff();
                                delay(100);
			} 
			// disabilita Interrupt                  //
			myRcSwitch.disableReceive();

			if(ricevuto<2)
			{

				if(autoLearning(indexButt, modeType))
				{
					//delay(3000);
					// switch off the LED
					//digitalWrite (LED_PIN, LOW);
					FastBlinkAddrLED(5,getColor4ButtIndex(indexPage));
				}
				else
				{
					// fast blink for 10 secs
					//FastBlinkingLED(10,LED_PIN);
                                        FastBlinkAddrLED(10, getColor4ButtIndex(indexPage));
				} 
			}
			// Reset
			myRcSwitch.resetAvailable();
			irrecv.resume();                  
			// reset the flag
			RecInProgress = false;
			// switch off the led
                        ledOff();
		} // selected button                        
	} // recInProg
}	
