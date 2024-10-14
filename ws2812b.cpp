/* 
 * File:   ws2812b.cpp
 * Author: philippe SIMIER Lycée touchard Le Mans
 *
 * Created on 12 octobre 2024, 17:07
 */

#include "ws2812b.h"

WS2812b::WS2812b(unsigned int numLED) :
numLEDs(numLED) {
    if (numLED > LED_BUFFER_LENGTH) {

        throw std::runtime_error("Exception to constructor ws2812b");

    }

    initHardware();
    clearLEDBuffer();
}


// Map 4k register memory for direct access from user space and return a user space pointer to it.
// See: http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka3750.html,
// The pointer addresses a 32-bit WORD, not an 8-bit byte. (So, addresses may seem 4x too small!)

volatile unsigned *WS2812b::mapRegisterMemory(int base) {
    static int mem_fd = 0;
    char *mem, *map;

    /* open /dev/mem */
    if (!mem_fd) {
        if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
            //printf("can't open /dev/mem \n");
            exit(-1);
        }
    }

    /* mmap register */
    // Allocate MAP block
    if ((mem = (char *) malloc(BLOCK_SIZE + (PAGE_SIZE - 1))) == NULL) {
        printf("allocation error \n");
        exit(-1);
    }

    // Make sure pointer is on 4K boundary
    if ((unsigned long) mem % PAGE_SIZE)
        mem += PAGE_SIZE - ((unsigned long) mem % PAGE_SIZE);

    // Now map it
    map = (char *) mmap(
            (caddr_t) mem,
            BLOCK_SIZE,
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_FIXED,
            mem_fd,
            base
            );

    if ((long) map < 0) {
        printf("mmap error %d\n", (int) map);
        exit(-1);
    }

    // Always use volatile pointer!
    return (volatile unsigned *) map;
}

// set up a memory regions to access GPIO, PWM and the clock manager

void WS2812b::setupRegisterMemoryMappings() {
    gpio = mapRegisterMemory(GPIO_BASE);
    pwm = mapRegisterMemory(PWM_BASE);
    clk = mapRegisterMemory(CLOCK_BASE);
    dma = mapRegisterMemory(DMA_BASE);
}

// Zero out the PWM waveform buffer

void WS2812b::clearPWMBuffer() {
    int i;

    for (i = 0; i < PWM_WAVEFORM_LENGTH; i++) {
        PWMWaveform[i] = 0x00000000;
    }
}

// Zero out the LED buffer

void WS2812b::clearLEDBuffer() {
    int i;
    for (i = 0; i < LED_BUFFER_LENGTH; i++) {
        LEDBuffer[i].r = 0;
        LEDBuffer[i].g = 0;
        LEDBuffer[i].b = 0;
    }
}

// Start or stop PWM output

void WS2812b::enablePWM(unsigned char state) {
    *(pwm + PWM_CTL) |= (state << PWM_CTL_PWEN1);
}

// Is the FIFO empty?

unsigned char WS2812b::FIFOEmpty() {
    if (*(pwm + PWM_STA) & (1 << PWM_STA_EMPT1)) {
        return true;
    } else {
        return false;
    }
}

// Turn r, g, and b into a Color_t struct

Color WS2812b::RGB2Color(unsigned char r, unsigned char g, unsigned char b) {

    Color color = {r, g, b};
    return color;

}

// Set pixel color (24-bit color)

void WS2812b::setPixelColor(unsigned int pixel, unsigned char r, unsigned char g, unsigned char b) {

    if (pixel < 0) {
        throw std::runtime_error("Exception to set pixel");
    }
    if (pixel > LED_BUFFER_LENGTH - 1) {
        throw std::runtime_error("Exception to set pixel");

    }
    LEDBuffer[pixel] = RGB2Color(r, g, b);


}

void WS2812b::setPixelColor(unsigned int pixel, Color color, float lum) {
    color.b = color.b * lum;
    color.g = color.g * lum;
    color.r = color.r * lum;
    LEDBuffer[pixel] = color;

}


// Set an individual bit in the PWM output array, accounting for word boundaries

void WS2812b::setPWMBit(unsigned int bitPos, unsigned char bit) {

    // Fetch word the bit is in
    unsigned int wordOffset = (int) (bitPos / 32);
    unsigned int bitIdx = bitPos - (wordOffset * 32);

    // printf("bitPos=%d wordOffset=%d bitIdx=%d value=%d\n", bitPos, wordOffset, bitIdx, bit);
    switch (bit) {
        case 1:
            PWMWaveform[wordOffset] |= (1 << bitIdx);
            break;
        case 0:
            PWMWaveform[wordOffset] &= ~(1 << bitIdx);
            break;
    }
}

// Get an individual bit from the PWM output array, accounting for word boundaries

unsigned char WS2812b::getPWMBit(unsigned int bitPos) {

    // Fetch word the bit is in
    unsigned int wordOffset = (int) (bitPos / 32);
    unsigned int bitIdx = bitPos - (wordOffset * 32);

    if (PWMWaveform[wordOffset] & (1 << bitIdx)) {
        return true;
    } else {
        return false;
    }
}


// Reverse the bits in a word

unsigned int WS2812b::reverseWord(unsigned int word) {

    unsigned int output = 0;

    for (int i = 0; i < 32; i++) {

        output |= word & (1 << i) ? 1 : 0;
        if (i < 31) {
            output <<= 1;
        }
    }
    return output;
}



// Clear PWM errors (using SETBIT because you "clear" errors by writing a 1 to their bit positions)

void WS2812b::clearPWMErrors() {
    SETBIT(*(pwm + PWM_STA), PWM_STA_WERR1);
    SETBIT(*(pwm + PWM_STA), PWM_STA_RERR1);
    SETBIT(*(pwm + PWM_STA), PWM_STA_GAPO1);
    SETBIT(*(pwm + PWM_STA), PWM_STA_BERR);
}

// Clear the PWM FIFO

void WS2812b::clearFIFO() {
    SETBIT(*(pwm + PWM_CTL), PWM_CTL_CLRF1);
}


// Initialize the PWM generator

void WS2812b::initHardware() {
    // mmap register space
    setupRegisterMemoryMappings();

    // set PWM alternate function for GPIO18
    SET_GPIO_ALT(18, 5);

    // stop clock and waiting for busy flag doesn't work, so kill clock
    *(clk + PWM_CLK_CNTL) = 0x5A000000 | (1 << 5);
    usleep(1000);

    // Disable DMA
    CLRBIT(*(pwm + PWM_DMAC), PWM_DMAC_ENAB);
    usleep(1000);

    // Set up the PWM clock
    // The fractional part is quantized to a range of 0-1024, so multiply the decimal part by 1024.
    // E.g., 0.25 * 1024 = 256.
    // So, if you want a divisor of 400.5, set idiv to 400 and fdiv to 512.
    unsigned int idiv = 400;
    unsigned short fdiv = 0; // Should be 16 bits, but the value must be <= 1024
    *(clk + PWM_CLK_DIV) = 0x5A000000 | (idiv << 12) | fdiv;
    usleep(1000);

    // Enable the clock.
    // Next-to-last digit means "enable clock"
    // Last digit is 1 (oscillator), 4 (PLLA), 5 (PLLC), or 6 (PLLD) (according to the docs) although
    // PLLA doesn't seem to work.
    // Note that the PLLs can be slowed down if the system is under heavy load - this may mean that
    // eventually it's necessary to use another (slower) PLL or the oscillator. However, it may not
    // matter under the operating conditions specific to any given project. I wouldn't change it
    // unless necessary.
    *(clk + PWM_CLK_CNTL) = 0x5A000015;
    usleep(1000);

    // Disable PWM (by clearing the control register, including bit PWEN1)
    *(pwm + PWM_CTL) = 0;
    usleep(1000);

    // Clear status registers (to remove errors)
    //*(pwm + PWM_STA) = -1;

    // The range (transmitted word size) is 32 bits.
    // >32: Pad with zeros.
    // <32: Truncate.
    *(pwm + PWM_RNG1) = 32;
    usleep(1000);

    // Clear any errors
    clearPWMErrors();
    usleep(1000);

    // Set up PWM control registers
    unsigned int controlWord = 0x00000000;
    SETBIT(controlWord, PWM_CTL_MODE1); // 1=Set serializer mode, 0=set PWM algorithm mode
    CLRBIT(controlWord, PWM_CTL_RPTL1); // 1=Repeat last contents if FIFO runs dry, 0=don't
    CLRBIT(controlWord, PWM_CTL_SBIT1); // Silence/padding bit (normally 0)
    CLRBIT(controlWord, PWM_CTL_POLA1); // Polarity (normally 0)
    SETBIT(controlWord, PWM_CTL_USEF1); // 1=Use FIFO, 0=Use DAT1 register
    *(pwm + PWM_CTL) = controlWord;
    usleep(1000);

}

// Write the LED buffer to the PWM FIFO input, translating it into the WS2812 wire format

void WS2812b::show() {

    // Set up PWM control registers
    unsigned int controlWord = 0x00000000;
    SETBIT(controlWord, PWM_CTL_MODE1); // 1=Set serializer mode, 0=set PWM algorithm mode
    CLRBIT(controlWord, PWM_CTL_RPTL1); // 1=Repeat last contents if FIFO runs dry, 0=don't
    CLRBIT(controlWord, PWM_CTL_SBIT1); // Silence/padding bit (normally 0)
    CLRBIT(controlWord, PWM_CTL_POLA1); // Polarity (normally 0)
    SETBIT(controlWord, PWM_CTL_USEF1); // 1=Use FIFO, 0=Use DAT1 register
    *(pwm + PWM_CTL) = controlWord;
    usleep(1000);

    // Erase whatever might be in the PWM buffer
    clearPWMBuffer();

    // Read data from LEDBuffer[], translate it into wire format, and write to PWMWaveform
    int i, j;

    unsigned int colorBits = 0; // Holds the GRB color before conversion to wire bit pattern
    unsigned char colorBit = 0; // Holds current bit out of colorBits to be processed
    unsigned int wireBit = 0; // Holds the current bit we will set in PWMWaveform

    for (i = 0; i < (int) numLEDs; i++) {

        colorBits = ((unsigned int) LEDBuffer[i].r << 8) | ((unsigned int) LEDBuffer[i].g << 16) | LEDBuffer[i].b;

        // Iterate through color bits to get wire bits
        for (j = 23; j >= 0; j--) {
            colorBit = (colorBits & (1 << j)) ? 1 : 0;
            switch (colorBit) {
                case 1:
                    //wireBits = 0b110;     // High, High, Low
                    setPWMBit(wireBit++, 1);
                    setPWMBit(wireBit++, 1);
                    setPWMBit(wireBit++, 0);
                    break;
                case 0:
                    //wireBits = 0b100;     // High, Low, Low
                    setPWMBit(wireBit++, 1);
                    setPWMBit(wireBit++, 0);
                    setPWMBit(wireBit++, 0);
                    break;
            }
        }
    }

    // Stop PWM (assuming it's running)
    enablePWM(false);
    usleep(1000);

    // Clear the FIFO
    clearFIFO();
    usleep(1000);

    // printf("Before filling FIFO: ");
    // dumpPWMStatus();

    //printf("\n");
    //dumpPWMBuffer();
    //printf("\n");

    for (i = 0; i < 16; i++) {
        // First, we have to reverse the bit order
        PWMWaveform[i] = reverseWord(PWMWaveform[i]);

        // That done, we add the word to the FIFO
        //printf("Adding word to FIFO: ");
        //printBinary(PWMWaveform[i], 32);
        //printf("\n");
        *(pwm + PWM_FIF1) = PWMWaveform[i];
        //*(pwm + PWM_FIF1) = 0xACAC00F0; // A test pattern easily visible on an oscilloscope, but not WS2812 compatible
        usleep(1000);
    }

    // printf("After filling FIFO: ");
    // dumpPWMStatus();

    // Enable PWM, which will now read the waveform out of the FIFO
    enablePWM(true);

}




