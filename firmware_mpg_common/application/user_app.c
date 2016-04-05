/**********************************************************************************************************************
File: user_app.c                                                                

----------------------------------------------------------------------------------------------------------------------
To start a new task using this user_app as a template:
 1. Copy both user_app.c and user_app.h to the Application directory
 2. Rename the files yournewtaskname.c and yournewtaskname.h
 3. Add yournewtaskname.c and yournewtaskname.h to the Application Include and Source groups in the IAR project
 4. Use ctrl-h (make sure "Match Case" is checked) to find and replace all instances of "user_app" with "yournewtaskname"
 5. Use ctrl-h to find and replace all instances of "UserApp" with "YourNewTaskName"
 6. Use ctrl-h to find and replace all instances of "USER_APP" with "YOUR_NEW_TASK_NAME"
 7. Add a call to YourNewTaskNameInitialize() in the init section of main
 8. Add a call to YourNewTaskNameRunActiveState() in the Super Loop section of main
 9. Update yournewtaskname.h per the instructions at the top of yournewtaskname.h
10. Delete this text (between the dashed lines) and update the Description below to describe your task
----------------------------------------------------------------------------------------------------------------------

Description:
This is a user_app.c file template 

------------------------------------------------------------------------------------------------------------------------
API:

Public functions:


Protected System functions:
void UserAppInitialize(void)
Runs required initialzation for the task.  Should only be called once in main init section.

void UserAppRunActiveState(void)
Runs current task state.  Should only be called once in main loop.


**********************************************************************************************************************/

#include "configuration.h"
#include "lcd_bitmaps.h"
#define GREEN0 GREEN3
#define BLUE0 BLUE3
#define RED0 RED3
/***********************************************************************************************************************
Global variable definitions with scope across entire project.
All Global variable names shall start with "G_"
***********************************************************************************************************************/
/* New variables */
volatile u32 G_u32UserAppFlags;                       /* Global state flags */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Existing variables (defined in other files -- should all contain the "extern" keyword) */
extern AntSetupDataType G_stAntSetupData;                         /* From ant.c */

extern u32 G_u32AntApiCurrentDataTimeStamp;                       /* From ant_api.c */
extern AntApplicationMessageType G_eAntApiCurrentMessageClass;    /* From ant_api.c */
extern u8 G_au8AntApiCurrentData[ANT_APPLICATION_MESSAGE_BYTES];  /* From ant_api.c */

extern volatile u32 G_u32SystemFlags;                  /* From main.c */
extern volatile u32 G_u32ApplicationFlags;             /* From main.c */

extern volatile u32 G_u32SystemTime1ms;                /* From board-specific source file */
extern volatile u32 G_u32SystemTime1s;                 /* From board-specific source file */

extern const u8 au8Scale[64][1];

/***********************************************************************************************************************
Global variable definitions with scope limited to this local application.
Variable names shall start with "UserApp_" and be declared as static.
***********************************************************************************************************************/
static u32 UserApp_u32DataMsgCount = 0;             /* Counts the number of ANT_DATA packets received */
static u32 UserApp_u32TickMsgCount = 0;             /* Counts the number of ANT_TICK packets received */

static fnCode_type UserApp_StateMachine;            /* The state machine function pointer */
static u32 UserApp_u32Timeout;                      /* Timeout counter used across states */


/**********************************************************************************************************************
Function Definitions
**********************************************************************************************************************/

/*--------------------------------------------------------------------------------------------------------------------*/
/* Public functions                                                                                                   */
/*--------------------------------------------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------------------------------------------*/
/* Protected functions                                                                                                */
/*--------------------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------------------
Function: UserAppInitialize

Description:
Initializes the State Machine and its variables.

Requires:
  -

Promises:
  - 
*/
void UserAppInitialize(void)
{
  u8 au8WelcomeMessage[] = "REAL TIME HR GRAPH";
  u8 au8Instructions[] = "B0 toggles radio";
  
  /* Clear screen and place start messages */
#ifdef MPG1
  LCDCommand(LCD_CLEAR_CMD);
  LCDMessage(LINE1_START_ADDR, au8WelcomeMessage); 
  LCDMessage(LINE2_START_ADDR, au8Instructions); 

  /* Start with LED0 in RED state = channel is not configured */
  LedOn(RED);
#endif /* MPG1 */
  
#ifdef MPG2
  PixelAddressType sStringLocation = {LCD_SMALL_FONT_LINE0, LCD_LEFT_MOST_COLUMN}; 
  LcdClearScreen();
  LcdLoadString(au8WelcomeMessage, LCD_FONT_SMALL, &sStringLocation); 
  sStringLocation.u16PixelRowAddress = LCD_SMALL_FONT_LINE1;
  LcdLoadString(au8Instructions, LCD_FONT_SMALL, &sStringLocation); 
  
  /* Start with LED0 in RED state = channel is not configured */
  LedOn(RED0);
#endif /* MPG2 */
  
 /* Configure ANT for this application */
  G_stAntSetupData.AntChannel          = ANT_CHANNEL_USERAPP;
  G_stAntSetupData.AntSerialLo         = ANT_SERIAL_LO_USERAPP;
  G_stAntSetupData.AntSerialHi         = ANT_SERIAL_HI_USERAPP;
  G_stAntSetupData.AntDeviceType       = ANT_DEVICE_TYPE_USERAPP;
  G_stAntSetupData.AntTransmissionType = ANT_TRANSMISSION_TYPE_USERAPP;
  G_stAntSetupData.AntChannelPeriodLo  = ANT_CHANNEL_PERIOD_LO_USERAPP;
  G_stAntSetupData.AntChannelPeriodHi  = ANT_CHANNEL_PERIOD_HI_USERAPP;
  G_stAntSetupData.AntFrequency        = ANT_FREQUENCY_USERAPP;
  G_stAntSetupData.AntTxPower          = ANT_TX_POWER_USERAPP;
 
  /* If good initialization, set state to Idle */
  if( AntChannelConfig(ANT_SLAVE) )
  {
    /* Channel is configured, so change LED to yellow */
#ifdef MPG1
    LedOff(RED);
    LedOn(YELLOW);
#endif /* MPG1 */
    
#ifdef MPG2
    LedOn(GREEN0);
#endif /* MPG2 */
    
    UserApp_StateMachine = UserAppSM_Idle;
  }
  else
  {
    /* The task isn't properly initialized, so shut it down and don't run */
#ifdef MPG1
    LedBlink(RED, LED_4HZ);
#endif /* MPG1 */
    
#ifdef MPG2
    LedBlink(RED0, LED_4HZ);
#endif /* MPG2 */

    UserApp_StateMachine = UserAppSM_Error;
  }

} /* end UserAppInitialize() */


/*----------------------------------------------------------------------------------------------------------------------
Function UserAppRunActiveState()

Description:
Selects and runs one iteration of the current state in the state machine.
All state machines have a TOTAL of 1ms to execute, so on average n state machines
may take 1ms / n to execute.

Requires:
  - State machine function pointer points at current state

Promises:
  - Calls the function to pointed by the state machine function pointer
*/
void UserAppRunActiveState(void)
{
  UserApp_StateMachine();

} /* end UserAppRunActiveState */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Private functions                                                                                                  */
/*--------------------------------------------------------------------------------------------------------------------*/


/**********************************************************************************************************************
State Machine Function Definitions
**********************************************************************************************************************/

/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for a message to be queued */
static void UserAppSM_Idle(void)
{
  /* Look for BUTTON 0 to open channel */
  if(WasButtonPressed(BUTTON0))
  {
    /* Got the button, so complete one-time actions before next state */
    ButtonAcknowledge(BUTTON0);
     LcdClearScreen();

     PixelBlockType sideScale = {0,19,128,5};
     LcdLoadBitmap(&au8Scale[0][0], &sideScale);
     PixelAddressType line0 = {0,0};
     LcdLoadString("183", LCD_FONT_SMALL, &line0);
     PixelAddressType line1 = {8,0};
     LcdLoadString("167", LCD_FONT_SMALL, &line1);
     PixelAddressType line2 = {16,0};
     LcdLoadString("151", LCD_FONT_SMALL, &line2);
     PixelAddressType line3 = {24,0};
     LcdLoadString("135", LCD_FONT_SMALL, &line3);
     PixelAddressType line4 = {32,0};
     LcdLoadString("119", LCD_FONT_SMALL, &line4);
     PixelAddressType line5 = {40,0};
     LcdLoadString("103", LCD_FONT_SMALL, &line5);
     PixelAddressType line6 = {48,0};
     LcdLoadString("87", LCD_FONT_SMALL, &line6);
     PixelAddressType line7 = {56,0};
     LcdLoadString("71", LCD_FONT_SMALL, &line7);
     
    /* Queue open channel and change LED0 from yellow to blinking green to indicate channel is opening */
    AntOpenChannel();

#ifdef MPG1
    LedOff(YELLOW);
    LedBlink(GREEN, LED_2HZ);
#endif /* MPG1 */    
    
#ifdef MPG2
    LedOff(RED0);
    LedBlink(GREEN0, LED_2HZ);
#endif /* MPG2 */
    
    /* Set timer and advance states */
    UserApp_u32Timeout = G_u32SystemTime1ms;
    UserApp_StateMachine = UserAppSM_WaitChannelOpen;
  }
    
} /* end UserAppSM_Idle() */
     

/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for channel to open */
static void UserAppSM_WaitChannelOpen(void)
{
  /* Monitor the channel status to check if channel is opened */
  if(AntRadioStatus() == ANT_OPEN)
  {
#ifdef MPG1
    LedOn(GREEN);
#endif /* MPG1 */    
    
#ifdef MPG2
    LedOn(GREEN0);
#endif /* MPG2 */
    
    UserApp_StateMachine = UserAppSM_ChannelOpen;
  }
  
  /* Check for timeout */
  if( IsTimeUp(&UserApp_u32Timeout, TIMEOUT_VALUE) )
  {
    AntCloseChannel();

#ifdef MPG1
    LedOff(GREEN);
    LedOn(YELLOW);
#endif /* MPG1 */    
    
#ifdef MPG2
    LedOn(RED0);
    LedOn(GREEN0);
#endif /* MPG2 */
    
    UserApp_StateMachine = UserAppSM_Idle;
  }
    
} /* end UserAppSM_WaitChannelOpen() */


/*-------------------------------------------------------------------------------------------------------------------*/
/* Channel is open, so monitor data */
static void UserAppSM_ChannelOpen(void)
{
  static u8 u8LastState = 0xff;
  static u8 au8TickMessage[] = "EVENT x\n\r";  /* "x" at index [6] will be replaced by the current code */
  static u8 au8DataContent[] = "xxxxxxxxxxxxxxxx";
  static u8 au8LastAntData[ANT_APPLICATION_MESSAGE_BYTES] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  static u8 au8TestMessage[] = {0, 0, 0, 0, 0xA5, 0, 0, 0};
  bool bGotNewData;

  /* Check for BUTTON0 to close channel */
  if(WasButtonPressed(BUTTON0))
  {
    /* Got the button, so complete one-time actions before next state */
    ButtonAcknowledge(BUTTON0);
    
    /* Queue close channel and change LED to blinking green to indicate channel is closing */
    AntCloseChannel();
    u8LastState = 0xff;

#ifdef MPG1
    LedOff(YELLOW);
    LedOff(BLUE);
    LedBlink(GREEN, LED_2HZ);
#endif /* MPG1 */    
    
#ifdef MPG2
    LedOff(RED0);
    LedOff(BLUE0);
    LedBlink(GREEN0, LED_2HZ);
#endif /* MPG2 */
    
    /* Set timer and advance states */
    UserApp_u32Timeout = G_u32SystemTime1ms;
    UserApp_StateMachine = UserAppSM_WaitChannelClose;
  } /* end if(WasButtonPressed(BUTTON0)) */
  
  /* Always check for ANT messages */
  if( AntReadData() )
  {
     /* New data message: check what it is */
    if(G_eAntApiCurrentMessageClass == ANT_DATA)
    {
      UserApp_u32DataMsgCount++;
      
      /* Check if the new data is the same as the old data and update as we go */
      bGotNewData = TRUE;//used to be false
      for(u8 i = 0; i < ANT_APPLICATION_MESSAGE_BYTES; i++)
      {
        if(G_au8AntApiCurrentData[i] != au8LastAntData[i])
        {
          bGotNewData = TRUE;
          au8LastAntData[i] = G_au8AntApiCurrentData[i];

          au8DataContent[2 * i] = HexToASCIICharUpper(G_au8AntApiCurrentData[i] / 16);
          au8DataContent[2 * i + 1] = HexToASCIICharUpper(G_au8AntApiCurrentData[i] % 16); 
        }
      }
      
      
      if(bGotNewData)
      {
        /* We got new data: show on LCD */
#ifdef MPG1
        LCDClearChars(LINE2_START_ADDR, 20); 
        LCDMessage(LINE2_START_ADDR, au8DataContent); 
#endif /* MPG1 */    
    
#ifdef MPG2
        PixelAddressType sStringLocation1 = {0, 122}; 
        PixelAddressType sStringLocation2 = {8, 122};
        PixelAddressType sStringLocation3 = {16, 122};
        const unsigned char * HRTop = ReturnTopDigit(G_au8AntApiCurrentData[7]);
        const unsigned char * HRMiddle = ReturnMiddleDigit(G_au8AntApiCurrentData[7]);
        const unsigned char * HRBottom = ReturnLastDigit(G_au8AntApiCurrentData[7]);
        LcdLoadString(HRTop, LCD_FONT_SMALL, &sStringLocation1); 
        LcdLoadString(HRMiddle, LCD_FONT_SMALL, &sStringLocation2); 
        LcdLoadString(HRBottom, LCD_FONT_SMALL, &sStringLocation3); 
        //LcdLoadString(" ",LCD_FONT_SMALL, &sStringLocation);
        printToScreen(G_au8AntApiCurrentData[7]);
#endif /* MPG2 */

        /* Update our local message counter and send the message back */
        au8TestMessage[7]++;
        if(au8TestMessage[7] == 0)
        {
          au8TestMessage[6]++;
          if(au8TestMessage[6] == 0)
          {
            au8TestMessage[5]++;
          }
        }
        AntQueueBroadcastMessage(au8TestMessage);

        /* Check for a special packet and respond */
#ifdef MPG1
        if(G_au8AntApiCurrentData[0] == 0xA5)
        {
          LedOff(LCD_RED);
          LedOff(LCD_GREEN);
          LedOff(LCD_BLUE);
          
          if(G_au8AntApiCurrentData[1] == 1)
          {
            LedOn(LCD_RED);
          }
          
          if(G_au8AntApiCurrentData[2] == 1)
          {
            LedOn(LCD_GREEN);
          }

          if(G_au8AntApiCurrentData[3] == 1)
          {
            LedOn(LCD_BLUE);
          }
        }
#endif /* MPG1 */    
    
#ifdef MPG2
        /*if(G_au8AntApiCurrentData[0] == 0xFF)
        {
          LedOff(RED3);
          LedOff(GREEN3);
          LedOff(BLUE3);
          
          if(G_au8AntApiCurrentData[1] == 0xFF)
          {
            LedOn(RED3);
          }
          
          if(G_au8AntApiCurrentData[2] == 0xFF)
          {
            LedOn(GREEN3);
          }

          if(G_au8AntApiCurrentData[3] == 0xFF)
          {
            LedOn(BLUE3);
          }
        }*/
        
        
#endif /* MPG2 */
      } /* end if(bGotNewData) */
    } /* end if(G_eAntApiCurrentMessageClass == ANT_DATA) */
    
    else if(G_eAntApiCurrentMessageClass == ANT_TICK)
    {
      UserApp_u32TickMsgCount++;

      /* Look at the TICK contents to check the event code and respond only if it's different */
      if(u8LastState != G_au8AntApiCurrentData[ANT_TICK_MSG_EVENT_CODE_INDEX])
      {
        /* The state changed so update u8LastState and queue a debug message */
        u8LastState = G_au8AntApiCurrentData[ANT_TICK_MSG_EVENT_CODE_INDEX];
        au8TickMessage[6] = HexToASCIICharUpper(u8LastState);
        DebugPrintf(au8TickMessage);

        /* Parse u8LastState to update LED status */
        switch (u8LastState)
        {
#ifdef MPG1
          /* If we are synced with a device, blue is solid */
          case RESPONSE_NO_ERROR:
          {
            LedOff(GREEN);
            LedOn(BLUE);
            break;
          }

          /* If we are paired but missing messages, blue blinks */
          case EVENT_RX_FAIL:
          {
            LedOff(GREEN);
            LedBlink(BLUE, LED_2HZ);
            break;
          }

          /* If we drop to search, LED is green */
          case EVENT_RX_FAIL_GO_TO_SEARCH:
          {
            LedOff(BLUE);
            LedOn(GREEN);
            break;
          }
#endif /* MPG 1 */
#ifdef MPG2
          /* If we are synced with a device, blue is solid */
          case RESPONSE_NO_ERROR:
          {
            LedOff(GREEN0);
            LedOn(BLUE0);
            break;
          }

          /* If we are paired but missing messages, blue blinks */
          case EVENT_RX_FAIL:
          {
            LedOff(GREEN0);
            LedBlink(BLUE0, LED_2HZ);
            break;
          }

          /* If we drop to search, LED is green */
          case EVENT_RX_FAIL_GO_TO_SEARCH:
          {
            LedOff(BLUE0);
            LedOn(GREEN0);
            break;
          }
#endif /* MPG 2 */
          /* If the search times out, the channel should automatically close */
          case EVENT_RX_SEARCH_TIMEOUT:
          {
            DebugPrintf("Search timeout\r\n");
            break;
          }

          default:
          {
            DebugPrintf("Unexpected Event\r\n");
            break;
          }
        } /* end switch (G_au8AntApiCurrentData) */
      } /* end if (u8LastState != G_au8AntApiCurrentData[ANT_TICK_MSG_EVENT_CODE_INDEX]) */
    } /* end else if(G_eAntApiCurrentMessageClass == ANT_TICK) */
    
  } /* end AntReadData() */
  
  /* A slave channel can close on its own, so explicitly check channel status */
  if(AntRadioStatus() != ANT_OPEN)
  {
#ifdef MPG1
    LedBlink(GREEN, LED_2HZ);
    LedOff(BLUE);
#endif /* MPG1 */

#ifdef MPG2
    LedBlink(GREEN0, LED_2HZ);
    LedOff(BLUE0);
#endif /* MPG2 */
    u8LastState = 0xff;
    
    UserApp_u32Timeout = G_u32SystemTime1ms;
    UserApp_StateMachine = UserAppSM_WaitChannelClose;
  } /* if(AntRadioStatus() != ANT_OPEN) */
      
} /* end UserAppSM_ChannelOpen() */


/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for channel to close */
static void UserAppSM_WaitChannelClose(void)
{
  /* Monitor the channel status to check if channel is closed */
  if(AntRadioStatus() == ANT_CLOSED)
  {
#ifdef MPG1
    LedOff(GREEN);
    LedOn(YELLOW);
#endif /* MPG1 */

#ifdef MPG2
    LedOn(GREEN0);
    LedOn(RED0);
#endif /* MPG2 */
    UserApp_StateMachine = UserAppSM_Idle;
  }
  
  /* Check for timeout */
  if( IsTimeUp(&UserApp_u32Timeout, TIMEOUT_VALUE) )
  {
#ifdef MPG1
    LedOff(GREEN);
    LedOff(YELLOW);
    LedBlink(RED, LED_4HZ);
#endif /* MPG1 */

#ifdef MPG2
    LedBlink(RED0, LED_4HZ);
    LedOff(GREEN0);
#endif /* MPG2 */
    
    UserApp_StateMachine = UserAppSM_Error;
  }
    
} /* end UserAppSM_WaitChannelClose() */


/*-------------------------------------------------------------------------------------------------------------------*/
/* Handle an error */
static void UserAppSM_Error(void)          
{

} /* end UserAppSM_Error() */


void printToScreen(u8 HR){
  static u16 column = 30;
  
  static u32 timer  = 0;
  static u8 state = 0;
  static u8 HRHistory [5] = {0,0,0,0,0};
  static u8 average = 0;
  u8 nextState  = state;
  
  PixelBlockType pixelBlock = {0,0,0,0};
  const u8 aau8Pixel [0x01][0x01] = {
      {0x01}
    };
  switch (state){
  case 0:
    if( column < 120 && G_u32SystemTime1ms > timer + 10){
      timer = G_u32SystemTime1ms + 10;
      nextState = 1;
    }
    if( column == 120 && G_u32SystemTime1ms > timer + 10){
      timer = G_u32SystemTime1ms + 10;
      nextState = 2;
    }
    
    
  break;

  case 1:
    for(int i = 1; i < 5; i++){
      HRHistory [i-1] = HRHistory[i];
    }
    HRHistory[4] = HR;
    
    average = (HRHistory[0] + HRHistory[1] + HRHistory[2]  + HRHistory[3] + HRHistory[4])/5;
    
    
    pixelBlock.u16RowStart= (u16) (64 - ((average - 55)/2));
    pixelBlock.u16ColumnStart =  column;
    pixelBlock.u16ColumnSize =   1;
    pixelBlock.u16RowSize = 1;
    LcdLoadBitmap(&aau8Pixel[0][0], &pixelBlock);
    column++;
    nextState = 0;
  break;  
  
  case 2:
    
    
    
    for(int i = 1; i < 5; i++){
      HRHistory [i-1] = HRHistory[i];
    }
    HRHistory[4] = HR;
    
    average = (HRHistory[0] + HRHistory[1] + HRHistory[2]  + HRHistory[3] + HRHistory[4])/5;
    const u8 aau8Pixel [0x01][0x01] = {
      {0x01}
    };
    
    pixelBlock.u16RowStart= (u16) (64 - ((average - 55)/2));
    pixelBlock.u16ColumnStart =  column;
    pixelBlock.u16ColumnSize = 1;
    pixelBlock.u16RowSize = 1;
    LcdLoadBitmap(&aau8Pixel[0][0], &pixelBlock);
    
    PixelBlockType testBlock = {0,31, 64, 90};
    LcdShift(&testBlock,1, LCD_SHIFT_LEFT);
    
    nextState = 0;
    
    
  break;  

  case 3:
    
    
  break;
  }
  state = nextState;

}


unsigned char* NumbersToChars( u8 arg){
  static unsigned char charNumber [3] = "   ";
  u8 bottomDigit = arg % 10;
  charNumber[2] = bottomDigit +48;
  arg /= 10;
  u8 middleDigit = arg %10;
  charNumber[1] = middleDigit + 48;
  arg /= 10;
  if(arg != 0){
    u8 topDigit = arg% 10;
    charNumber[0] = topDigit + 48;
  }
  else{
    charNumber[0] = ' ';}
    return charNumber;
}

unsigned char * ReturnLastDigit(u8 arg){
  static unsigned char charNumber [1] = "";
  u8 bottomDigit = arg % 10;
  charNumber[0] = bottomDigit +48;
  return charNumber;
}
unsigned char * ReturnMiddleDigit(u8 arg){
  static unsigned char charNumber [1] = "";
  u8 middleDigit ;
  arg /= 10;
  middleDigit = arg %10;
  charNumber[0] = middleDigit + 48;
  return charNumber;
}
unsigned char* ReturnTopDigit(u8 arg){
  static unsigned char charNumber [1] = "";
  arg /=10;
  arg/= 10;
  if(arg != 0){
    u8 topDigit = arg% 10;
    charNumber[0] = topDigit + 48;
  }
  else{
    charNumber[0] = ' ';}
    return charNumber;
}
/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/
