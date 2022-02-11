#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

#ifdef F_CPU
#undef F_CPU
#endif
//define cpu clock speed if not defined
#define F_CPU 4000000
//#endif
#define BAUDRATE 19200
#define UBRRVAL ((F_CPU/(BAUDRATE*16UL))-1)
#define pin_data_in (PINC & (1 << PINC1))
#define pin_ck_in (PINC & (1 << PINC2))
#define DEBOUNCE 4
#define BUF_LEN 48

char string_out[BUF_LEN]="startup\n";
uint8_t send_char=0;
uint8_t tx=1;
uint8_t msg_len=8;

ISR (USART_TX_vect)          //USART TX complete interrupt
{
    if(send_char<msg_len && tx==1)
    {
        UDR0=string_out[send_char];
        send_char++;
    }
    else 
    {
        tx=0;
        send_char=0;
    }
}

int main(void) {
    
   // make the LED pin an output for PORTB5
    DDRB |= 1 << PORTB5;
    DDRC &= ~(1 << PORTC1 | 1 << PORTC2);
    PORTC |= 1 << PORTC1; // pullup PORTC1 Data in
    PORTC |= 1 << PORTC2; // pullup PORTC2 CK in
    //Set baud rate
    UBRR0L=UBRRVAL;      //low byte
    UBRR0H=(UBRRVAL>>8);   //high byte
    UCSR0C=(0<<UMSEL01)|(0<<UMSEL00)|(0<<UPM01)|(0<<UPM00)| //Set data frame format: asynchronous mode,no parity, 1 stop bit, 8 bit size
        (0<<USBS0)|(0<<UCSZ02)|(1<<UCSZ01)|(1<<UCSZ00); 
    UCSR0B=(1<<RXEN0)|(1<<TXEN0)|(1<<TXCIE0); //Enable Transmitter and Receiver and Interrupt on receive complete
    TCCR1B |= (1<<CS12) | (CS10); // CS12:10=101 -> 1/1024 of F_CLK free running
    sei(); //enable global interrupts
    uint32_t data_register1=0;
    int32_t data_out=0;
    uint8_t data_in=0, data=0;
    uint8_t ck_in=0,ck=0, old_ck=0;
    uint8_t started=0;
    uint8_t bits_rcvd=0;
    uint8_t mm_inch=0; // 0=mm 1=inch
    uint8_t sign=0; // 0=+ 1=-
    uint16_t ticks=0, last_ticks=0;

    UDR0='\n';
    while (1)
    {
        if((pin_ck_in == 0)&& (ck_in < DEBOUNCE)) ck_in++; // inverted
        else if((pin_ck_in !=0 ) && (ck_in > 0)) ck_in--; // inverted
        if((pin_data_in == 0) && (data_in < DEBOUNCE)) data_in++; // inverted
        else if((pin_data_in != 0) && (data_in > 0)) data_in--; // inverted
        
        if(ck_in == DEBOUNCE) {old_ck=ck;ck=1;}
        else if(ck_in == 0) {old_ck=ck;ck=0;}
        if(data_in == DEBOUNCE) data=1;
        else if(data_in == 0) data=0;

        if(ck == 0 && old_ck == 1 && !started) // ck falling edge
        {
            started=1;
            bits_rcvd=0;
            last_ticks=TCNT1;
            data_register1=0;
            //UDR0='s';
        }
        if(ck == 1 && old_ck == 0 && started && bits_rcvd<24) // ck rising edge
        {

            data_register1=data_register1>>1;
            if(data==1) data_register1|= 0x00800000;
            bits_rcvd++;
        }
        if(bits_rcvd==24) // data risig edge with ck=1
        {
            started=0;
            
            if(data_register1 & 0x00800000) mm_inch=1; // 24th bit
            else mm_inch=0;
            if(data_register1 & 0x00100000) sign=1;  // 21th bit
            else sign=0;
            if(sign) data_out=-(int32_t)(data_register1 & 0x000fffff);
            else data_out=(int32_t)(data_register1 & 0x000fffff);

            if(tx==0)
            {
                if(mm_inch) 
                {
                    ltoa(data_out/2,string_out,10); // convert to string
                    if(data_out & 1) strcat(string_out,".5");
                    strcat(string_out," thou\r");
                }
                else  
                {
                    ltoa(data_out,string_out,10); // convert to string
                    strcat(string_out," um\r");
                }
                msg_len=strlen(string_out);
                tx=1; UDR0='\n';// start the transmission
            }
        }
        ticks=TCNT1;
        if(ticks-last_ticks > 500 && started) // timeout 50ms
        {
                started=0;
                //UDR0='r';
        }
        if(ticks-last_ticks > 5000) // timeout 500ms
        {   
            if(tx==0)
            {
                started=0;
                strcpy(string_out,"No conn\r");
                msg_len=strlen(string_out);
                tx=1; UDR0='\n';
                last_ticks=ticks;
            }
        }
        // toggle the LED
        PORTB ^= 1 << PORTB5;
    }

    return 0;  
}
