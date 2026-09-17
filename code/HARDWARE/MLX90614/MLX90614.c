#include "mlx90614.h"

float thj; // 环境温度
void SMBus_StartBit(void)
{
    SMBUS_SDA_H();  // Set SDA line
    SMBus_Delay(8); // Wait a few microseconds
    SMBUS_SCK_H();  // Set SCL line
    SMBus_Delay(8); // Generate bus free time between Stop
    SMBUS_SDA_L();  // Clear SDA line
    SMBus_Delay(8); // Hold time after (Repeated) Start
    // Condition. After this period, the first clock is generated.
    //(Thd:sta=4.0us min)
    SMBUS_SCK_L();  // Clear SCL line
    SMBus_Delay(8); // Wait a few microseconds
}

void SMBus_StopBit(void)
{
    SMBUS_SCK_L();  // Clear SCL line
    SMBus_Delay(8); // Wait a few microseconds
    SMBUS_SDA_L();  // Clear SDA line
    SMBus_Delay(8); // Wait a few microseconds
    SMBUS_SCK_H();  // Set SCL line
    SMBus_Delay(8); // Stop condition setup time(Tsu:sto=4.0us min)
    SMBUS_SDA_H();  // Set SDA line
}

u8 SMBus_SendByte(u8 Tx_buffer)
{
    u8 Bit_counter;
    u8 Ack_bit;
    u8 bit_out;

    for (Bit_counter = 8; Bit_counter; Bit_counter--)
    {
        if (Tx_buffer & 0x80)
        {
            bit_out = 1; // If the current bit of Tx_buffer is 1 set bit_out
        }
        else
        {
            bit_out = 0; // else clear bit_out
        }
        SMBus_SendBit(bit_out); // Send the current bit on SDA
        Tx_buffer <<= 1;        // Get next bit for checking
    }

    Ack_bit = SMBus_ReceiveBit(); // Get acknowledgment bit
    return Ack_bit;
}

void SMBus_SendBit(u8 bit_out)
{
    if (bit_out == 0)
    {
        SMBUS_SDA_L();
    }
    else
    {
        SMBUS_SDA_H();
    }
    SMBus_Delay(6); // Tsu:dat = 250ns minimum
    SMBUS_SCK_H();  // Set SCL line
    SMBus_Delay(8); // High Level of Clock Pulse
    SMBUS_SCK_L();  // Clear SCL line
    SMBus_Delay(8); // Low Level of Clock Pulse
    return;
}

u8 SMBus_ReceiveBit(void)
{
    u8 Ack_bit;

    SMBUS_SDA_H();
    SMBus_Delay(6); // High Level of Clock Pulse
    SMBUS_SCK_H();  // Set SCL line
    SMBus_Delay(8); // High Level of Clock Pulse
    if (SMBUS_SDA_PIN())
    {
        Ack_bit = 1;
    }
    else
    {
        Ack_bit = 0;
    }
    SMBUS_SCK_L();  // Clear SCL line
    SMBus_Delay(8); // Low Level of Clock Pulse

    return Ack_bit;
}

u8 SMBus_ReceiveByte(u8 ack_nack)
{
    u8 RX_buffer;
    u8 Bit_Counter;

    for (Bit_Counter = 8; Bit_Counter; Bit_Counter--)
    {
        if (SMBus_ReceiveBit()) // Get a bit from the SDA line
        {
            RX_buffer <<= 1; // If the bit is HIGH save 1  in RX_buffer
            RX_buffer |= 0x01;
        }
        else
        {
            RX_buffer <<= 1; // If the bit is LOW save 0 in RX_buffer
            RX_buffer &= 0xfe;
        }
    }
    SMBus_SendBit(ack_nack); // Sends acknowledgment bit
    return RX_buffer;
}

void SMBus_Delay(u16 time)
{
    u16 i, j;
    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < time; j++)
            ;
    }
}

void SMBus_Init()
{
    GPIO_InitTypeDef GPIO_InitStructure;
    /* Enable SMBUS_PORT clocks */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SMBUS_PORT, ENABLE);
    /*??SMBUS_SCK?SMBUS_SDA????????*/
    GPIO_InitStructure.GPIO_Pin = SMBUS_SCK | SMBUS_SDA;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SMBUS_PORT, &GPIO_InitStructure);
    SMBUS_SCK_H();
    SMBUS_SDA_H();
}

u16 SMBus_ReadMemory(u8 slaveAddress, u8 command)
{
    u16 data;        // Data storage (DataH:DataL)
    u8 Pec;          // PEC byte storage
    u8 DataL = 0;    // Low data byte storage
    u8 DataH = 0;    // High data byte storage
    u8 arr[6];       // Buffer for the sent bytes
    u8 PecReg;       // Calculated PEC byte storage
    u8 ErrorCounter; // Defines the number of the attempts for communication with MLX90614

    ErrorCounter = 0x00; // Initialising of ErrorCounter
    slaveAddress <<= 1;  // 2-7???????

    do
    {
    repeat:
        SMBus_StopBit();   // If slave send NACK stop comunication
        --ErrorCounter;    // Pre-decrement ErrorCounter
        if (!ErrorCounter) // ErrorCounter=0?
        {
            break; // Yes,go out from do-while{}
        }

        SMBus_StartBit();                 // Start condition
        if (SMBus_SendByte(slaveAddress)) // Send SlaveAddress ???Wr=0????????
        {
            goto repeat; // Repeat comunication again
        }
        if (SMBus_SendByte(command)) // Send command
        {
            goto repeat; // Repeat comunication again
        }

        SMBus_StartBit();                     // Repeated Start condition
        if (SMBus_SendByte(slaveAddress + 1)) // Send SlaveAddress ???Rd=1????????
        {
            goto repeat; // Repeat comunication again
        }

        DataL = SMBus_ReceiveByte(ACK); // Read low data,master must send ACK
        DataH = SMBus_ReceiveByte(ACK); // Read high data,master must send ACK
        Pec = SMBus_ReceiveByte(NACK);  // Read PEC byte, master must send NACK
        SMBus_StopBit();                // Stop condition

        arr[5] = slaveAddress;         //
        arr[4] = command;              //
        arr[3] = slaveAddress + 1;     // Load array arr
        arr[2] = DataL;                //
        arr[1] = DataH;                //
        arr[0] = 0;                    //
        PecReg = PEC_Calculation(arr); // Calculate CRC
    } while (PecReg != Pec); // If received and calculated CRC are equal go out from do-while{}

    data = (DataH << 8) | DataL; // data=DataH:DataL
    return data;
}

u8 PEC_Calculation(u8 pec[])
{
    u8 crc[6];
    u8 BitPosition = 47;
    u8 shift;
    u8 i;
    u8 j;
    u8 temp;

    do
    {
        /*Load pattern value 0x000000000107*/
        crc[5] = 0;
        crc[4] = 0;
        crc[3] = 0;
        crc[2] = 0;
        crc[1] = 0x01;
        crc[0] = 0x07;

        /*Set maximum bit position at 47 ( six bytes byte5...byte0,MSbit=47)*/
        BitPosition = 47;

        /*Set shift position at 0*/
        shift = 0;

        /*Find first "1" in the transmited message beginning from the MSByte byte5*/
        i = 5;
        j = 0;
        while ((pec[i] & (0x80 >> j)) == 0 && i > 0)
        {
            BitPosition--;
            if (j < 7)
            {
                j++;
            }
            else
            {
                j = 0x00;
                i--;
            }
        } /*End of while */

        /*Get shift value for pattern value*/
        shift = BitPosition - 8;

        /*Shift pattern value */
        while (shift)
        {
            for (i = 5; i < 0xFF; i--)
            {
                if ((crc[i - 1] & 0x80) && (i > 0))
                {
                    temp = 1;
                }
                else
                {
                    temp = 0;
                }
                crc[i] <<= 1;
                crc[i] += temp;
            } /*End of for*/
            shift--;
        } /*End of while*/

        /*Exclusive OR between pec and crc*/
        for (i = 0; i <= 5; i++)
        {
            pec[i] ^= crc[i];
        } /*End of for*/
    } while (BitPosition > 8); /*End of do-while*/

    return pec[0];
}

float SMBus_ReadTemp(void)
{
    return (SMBus_ReadMemory(SA, RAM_ACCESS | RAM_TOBJ1) * 0.02 - 273.15);
}
// tf 额温 temp1 环境温度
float tobody(float tf) // 温度矫正
{
    float tbody, tf_low, tf_high;
    thj = SMBus_ReadMemory(0x00, 0x06) * 0.02 - 273.15; // 获取环境温度
    if (thj < 25)                                       // 芯片温度低于25
    {
        tf_low = 32.66 + 0.186 * (thj - 25);
        tf_high = 34.84 + 0.148 * (thj - 25);
    }
    else
    {
        tf_low = 32.66 + 0.086 * (thj - 25);
        tf_high = 34.84 + 0.1 * (thj - 25);
    }
    if ((tf_low < tf) && (tf < tf_high))
        tbody = 36.3 + 0.5 / (tf_high - tf_low) * (tf - tf_low);
    else if (tf > tf_high)
        tbody = 36.8 + (0.829321 + 0.002364) * (tf - tf_high);
    else if (tf < tf_low)
        tbody = 36.3 + (0.551658 + 0.021525 * thj) * (tf - tf_high);
    return tbody;
}
