#define TXB8 0
#define UPE 2
#define DOR 3
#define FE 4
#define UDRE 5
#define RXC 7

void setup()
{
  mdb_uart_init();
}

void loop()
{
  while(available())
    receiveSend();
}

void receiveSend()
{
  unsigned char status, resh, resl;
  while(!available());
  status = UCSR0A;
  resh = UCSR0B;
  resl = UDR0;


  while(!(UCSR0A & (1<<UDRE)));
  UCSR0B &= ~(1<<TXB8);
  resh = (resh >> 1) & 0x01;
  if (resh)
    UCSR0B |= (1<<TXB8);
  UDR0 = resl;
}

int available()
{
  return (UCSR0A & (1<<RXC));
}

void mdb_uart_init()
{
  UCSR0B = (1<<RXEN0)|(1<<TXEN0);
  UBRR0H = 0;
  UBRR0L = 103; // 9600baud
  UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00); //9bit mode
  UCSR0B |= (1<<UCSZ02); //also for 9 bit mode
  UCSR0C = UCSR0C | B00000100; //one stop bit
  UCSR0C = UCSR0C | B00000000; //no parity bit
}

