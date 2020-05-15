#ifndef LCD_BACKLIGHT_HPP__
#define LCD_BACKLIGHT_HPP__

//#include <samd51p19a.h>
#include <cstdint>

// struct PORTRegs
// {
//     std::uint32_t DIR   ;
//     std::uint32_t DIRCLR;
//     std::uint32_t DIRSET;
//     std::uint32_t DIRTGL;
//     std::uint32_t OUT   ;
//     std::uint32_t OUTCLR;
//     std::uint32_t OUTSET;
//     std::uint32_t OUTTGL;
//     std::uint32_t IN    ;
//     std::uint32_t CTRL  ;
//     std::uint32_t WRCONFIG;
//     std::uint32_t EVCTRL;
//     std::uint8_t  PMUX[16];
//     std::uint8_t  PINCFG[32]; 
// };

// static volatile PORTRegs* const PORT_GROUP0 = reinterpret_cast<volatile PORTRegs* const>(0x41008000 + 0x80*0);
// static volatile PORTRegs* const PORT_GROUP1 = reinterpret_cast<volatile PORTRegs* const>(0x41008000 + 0x80*1);
// static volatile PORTRegs* const PORT_GROUP2 = reinterpret_cast<volatile PORTRegs* const>(0x41008000 + 0x80*2);
// static volatile PORTRegs* const PORT_GROUP3 = reinterpret_cast<volatile PORTRegs* const>(0x41008000 + 0x80*3);

// struct EVSYSChannelRegs
// {
//     std::uint32_t CHANNEL;
//     std::uint8_t  CHINTENCLR;
//     std::uint8_t  CHINTENSET;
//     std::uint8_t  CHINTFLAG;
//     std::uint8_t  CHSTATUS;
// };

// struct EVSYSRegs
// {
//     std::uint8_t  CTRLA;
//     std::uint8_t  _reserved0[3];
//     std::uint32_t SWEVT;
//     std::uint8_t  PRICTRL;
//     std::uint8_t  _reserved1[7];
//     std::uint16_t INTPEND;
//     std::uint8_t  _reserved2[2];
//     std::uint32_t INTSTATUS;
//     std::uint32_t BUSYCH;
//     std::uint32_t READYUSR;
//     EVSYSChannelRegs CHANNEL[32];
//     std::uint32_t USER[67];
// };

// static volatile EVSYSRegs* const EVSYS_ = reinterpret_cast<volatile EVSYSRegs* const>(0x4100e000);

// struct TC8BITRegs
// {
//     std::uint32_t CTRLA;
//     std::uint8_t  CTRLBCLR;
//     std::uint8_t  CTRLBSET;
//     std::uint16_t EVCTRL;
//     std::uint8_t  INTENCLR;
//     std::uint8_t  INTENSET;
//     std::uint8_t  INTFLAG;
//     std::uint8_t  STATUS;
//     std::uint8_t  WAVE;
//     std::uint8_t  DRVCTRL;
//     std::uint8_t  _reserved0;
//     std::uint8_t  DBGCTRL;
//     std::uint32_t SYNCBUSY;
//     std::uint8_t  COUNT;
//     std::uint8_t  _reserved1[6];
//     std::uint8_t  PER;
//     std::uint8_t  CC0;
//     std::uint8_t  CC1;
//     std::uint8_t  _reserved2[17];
//     std::uint8_t  PERBUF;
//     std::uint8_t  CC0BUF;
//     std::uint8_t  CC1BUF;
// };
// static volatile TC8BITRegs* const TC8BIT0 = reinterpret_cast<volatile TC8BITRegs* const>(0x40003800);

// struct CCLRegs
// {
//     std::uint8_t  CTRL;
//     std::uint8_t  _reserved0[3];
//     std::uint8_t  SEQCTRL0;
//     std::uint8_t  SEQCTRL1;
//     std::uint8_t  _reserved1[3];
//     std::uint32_t LUTCTRL[4];
// };
// static volatile CCLRegs* const CCL = reinterpret_cast<volatile CCLRegs* const>(0x42003800);

class LCDBackLight
{
private:
    std::uint8_t currentOutput = 100;
    std::uint8_t maxOutput = 100;
public:
    std::uint8_t getOutput() const { return this->currentOutput; }
    std::uint8_t getMaxOutput() const { return this->maxOutput; }
    
    void setOutput(std::uint8_t output)
    {
        this->currentOutput = output < this->maxOutput ? output : this->maxOutput;
        TC0->COUNT8.CC[0].reg = this->currentOutput;
        while(TC0->COUNT8.SYNCBUSY.bit.CC0);
    }
    void setMaxOutput(std::uint8_t maxOutput)
    {
        this->maxOutput = maxOutput;
        if( this->currentOutput > this->maxOutput ) {
            this->currentOutput = this->maxOutput;
        }
        TC0->COUNT8.PER.reg = this->maxOutput;
        while(TC0->COUNT8.SYNCBUSY.bit.PER);
        TC0->COUNT8.CC[0].reg = this->currentOutput;
        while(TC0->COUNT8.SYNCBUSY.bit.CC0);
    }

    void initialize()
    {
        /* Enable Peripheral Clocks */
        GCLK->PCHCTRL[9].reg = 0 | (1u<<6);         // TC0, TC1
        while(!GCLK->PCHCTRL[9].bit.CHEN);
        GCLK->PCHCTRL[11].reg = 0 | (1u<<6);    // EVSYS[0]
        while(!GCLK->PCHCTRL[11].bit.CHEN);
        GCLK->PCHCTRL[33].reg = 0 | (1u<<6);    // CCL
        while(!GCLK->PCHCTRL[33].bit.CHEN);
        /* Enable Peropheral APB Clocks */
        MCLK->APBAMASK.bit.TC0_ = 1;
        MCLK->APBBMASK.bit.EVSYS_ = 1;
        MCLK->APBCMASK.bit.CCL_ = 1;

        /* Configure PORT */
        PORT->Group[2].DIRSET.reg = (1<<5);
        PORT->Group[2].EVCTRL.reg = 0x85; // PC05, OUT
        /* Configure EVSYS */
	    EVSYS->USER[1].reg = 0x01;  // Channel0 -> PORT_EV0
        EVSYS->Channel[0].CHANNEL.reg = 0x74 | (0x02<<8) | (0x00<<10);  // CCL_LUTOUT0, ASYNCHRONOUS, NO_EVT_OUTPUT
        /* Configure CCL */
        CCL->CTRL.reg = (1<<0); // SWRST
        CCL->SEQCTRL[0].reg = 0x0; // Disable SEQCTRL
        CCL->LUTCTRL[0].reg = (0xaau << 24) | (1u<<22) | (0x6<<8) | (1<<1); // TRUTH=0xaa, LUTEO, INSEL0=0x06(TC), ENABLE
        CCL->CTRL.reg = (1<<1); // ENABLE

        /* Configure TC0 */
        TC0->COUNT8.CTRLA.reg = (1u<<0);   // SWRST;
        while( TC0->COUNT8.SYNCBUSY.bit.SWRST );
        
        TC0->COUNT8.CTRLA.reg = (0x01 << 2) | (0x01 << 4) | (0x04 << 8);   // MODE=COUNT8, PRESCALER=DIV16, PRESCSYNC=PRESC
        TC0->COUNT8.WAVE.reg  = 0x02; // WAVEGEN=NPWM;
        TC0->COUNT8.CTRLBSET.reg = (1u<<1); // LUPD
        TC0->COUNT8.PER.reg = this->maxOutput;
        TC0->COUNT8.CC[0].reg = this->currentOutput;
        TC0->COUNT8.CC[1].reg = 0u;
        TC0->COUNT8.DBGCTRL.bit.DBGRUN = 1;
        TC0->COUNT8.INTFLAG.reg = 0x33;    // Clear all flags
        while( TC0->COUNT8.SYNCBUSY.reg );
        
        TC0->COUNT8.CTRLA.bit.ENABLE = 1;   // ENABLE
        while( TC0->COUNT8.SYNCBUSY.bit.ENABLE );
    }
};
#endif //LCD_BACKLIGHT_HPP__