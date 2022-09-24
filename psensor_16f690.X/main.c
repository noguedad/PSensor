/**********************************************
 * Archivo: main.c
 * Autor: Ing. Victor Noguedad
 * uP: PIC16F690
 * 
 * Creado el 15 de agosto de 2022, 07:39 PM
 * 
 * Sensor de Presion para Autoclave
 * 
 ***********************************************/

/***********************************************
 * BITACORA DE CAMBIOS
 * 
 * 15/08/2022   Version 1.0.0
 * 20/09/2022   Version 2.0.0   
 *  Cambio de µP a PIC16F690, originalmente PIC12F683 
 * Creacion
 ***********************************************/

// *** BITS DE CONFIGURACION ***
#pragma config FOSC = INTRCIO   // Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA4/OSC2/CLKOUT pin, I/O function on RA5/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // MCLR Pin Function Select bit (MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Selection bits (BOR enabled)
#pragma config IESO = ON        // Internal External Switchover bit (Internal External Switchover mode is enabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is enabled)

#include <xc.h>

#define _XTAL_FREQ 8000000      // Frecuencia de operacion 8Mhz

// *** DEFINICIONES GLOBALES ***
#define LED     PORTCbits.RC7    // Salida de LED
#define SET_LED PORTCbits.RC6
#define INPUT   PORTBbits.RB4    // Entrada de BOTON
#define OUTPUT  PORTBbits.RB6    // Salida de RELAY

#define LO      0
#define HI      1
#define TRUE    1
#define FALSE   0

#define MAX_SET     1000        // Maximo de ciclos para declarar modo PROGRAMACION
#define MAX_PROGRAM 500         // Maximo de ciclos para declarar PROGRAMADO
#define MAX_SENSE   10          // Maximo de lecturas arriba del LIMITE para ACTIVAR

// *** VARIABLE GLOBALES ***
int SETMODE = FALSE;	        // Bandera PROGRAMACION
int SENSE = FALSE;              // Bandera LECTURA
int SENSE_COUNT = 0;            // Cuenta de LECTURA arriba del LIMITE
int THRESHOLD = 0;              // Valor de LIMITE para ACTIVAR

// *** CONFIGURACION INICIAL ***
void picSetup()
{
    
    OPTION_REG = 0b00000000;
    INTCON = 0x00;
    PIE1 = 0x00;
    PIE2 = 0x00;
    PCON = 0x00;
    OSCCON = 0b01110111;
    OSCTUNE = 0x00;
    
    PORTA = 0x00;
    PORTB = 0x00;
    PORTC = 0x00;
    
    TRISA = 0b00010000;
    TRISB = 0b00010000;
    TRISC = 0b00000000;
    
    WPUA = 0x00;
    WPUB = 0b00010000;
    
    ANSEL = 0b00001000;
    ANSELH = 0b00000000;
    
    T1CON = 0x00;
    T2CON = 0x00;
    CM1CON0 = 0b00000111;
    CM2CON0 = 0b00000111;
    
    // Lee el valor LIMITE almacenado en la EEPROM del PIC, direccion 0x00
    THRESHOLD = eeprom_read(0x00);
    // Si no se encuentra dentro de los valores esperados (0 - 1023) le asigna el valor mas alto
    if(THRESHOLD > 1023 || THRESHOLD < 0)
    {
        THRESHOLD = 1023;
    }
}

// *** LEE EL SENSOR ***
int readSensor()
{
    ADCON0 = 0b10001101;                    // Seleccionar AN3, justificado a la derecha, Vref = VDD
    ADCON1 = 0x00;
    ADCON0bits.GO = TRUE;                   // Iniciar lectura
    while(ADCON0bits.GO == TRUE);           // Esperar a que termine de leer
    return((int)((ADRESH<<8)+ADRESL));      // Devolver el resultado
}

// *** PARPADEO NORMAL ***
void normalBlink()
{
    SET_LED = HI;
    LED = HI;
    __delay_ms(10);
    LED = LO;
    __delay_ms(500);
    LED = HI;
}

// *** PARPADEO MODO PROGRAMACION ***
void setmodeBlink()
{
    LED = HI;
    SET_LED = HI;
    __delay_ms(10);
    SET_LED = LO;
    __delay_ms(500);
    SET_LED = HI;
}

// *** PUNTO DE ENTRADA ***
void main() 
{
    // Configuracion inicial
    picSetup();
    // Ciclo principal
    while(1)
    {
        // Limpia el contador de presion del boton
       int BTN_COUNT = 0;
       
       //Si se encuentra en modo PROGRAMACION
       if(SETMODE == TRUE)
       {
           // Parpadeo de modo programacion
           setmodeBlink();
           // Si se ha precionado el boton
           while(INPUT == LO)
           {
                // Si no se ha presionado lo suficiente
               if(BTN_COUNT < MAX_PROGRAM)
               {
                    // Incrementar el contador de presion del boton
                   BTN_COUNT++;
               }
               else // So ua se alcanzp el maximo
               {
                    // Leer el sensor y almacenarlo en el LIMITE
                   THRESHOLD = readSensor();
                   // Guardar ese valor en la EEPROM del PIC
                   eeprom_write(0x00, (unsigned char)THRESHOLD);
                   // Apagar el LED y salir del modo PROGRAMACION
                   LED = LO;
                   SETMODE = FALSE;
               }
           }
       }
       else // Si se encuentra en modo normal
       {
           // Parpadeo del led
           normalBlink();
           // Si se esta presionando el boton
           while(INPUT == LO)
           {
               // Si no se ha presionado lo suficiente
               if(BTN_COUNT < MAX_SET)
               {
                   // Aumentar el contador
                   BTN_COUNT++;
               }
               else // Si se presiono lo suficiente
               {
                   // Entrar a modo programacion
                   LED = HI;
                   SETMODE = TRUE;
               }
           }
           // Si la lectura del sensor alcanzo el ajuste
           if(readSensor() >= THRESHOLD)
           {
               // Si ya se habia detectado
               if(SENSE == TRUE)
               {
                    // Pero aun no alcanza el LIMITE para ACTIVAR
                   if(SENSE_COUNT < MAX_SENSE)
                   {
                        // Incrementar el contador
                       SENSE_COUNT++;
                   }
                   else // Si ya se alcanzo el limite
                   {
                        // Activar la salida
                       OUTPUT = HI;
                   }
               }
               else // Si es la primera vez que se detecta
               {
                    // Marcar que se detecto
                   SENSE = TRUE;
                   // Inicializar el contador
                   SENSE_COUNT = 0;
               }
           }
           else // Si la lectura del sensor esta por debajo del ajuste
           {
                // Y ya se habia detectado que se supero el ajuste (o esta activo)
               if(SENSE == TRUE)
               {
                    // Marcar que dejo de detectarse
                   SENSE = FALSE;
                   // Desactivar la salida
                   OUTPUT = LO;
               }
           }
       }
       // Esperar para otro ciclo de lectura
        __delay_ms(500);
    }
    // FIN
    return;
}
