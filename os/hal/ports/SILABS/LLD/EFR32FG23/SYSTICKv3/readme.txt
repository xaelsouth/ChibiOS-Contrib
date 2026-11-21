How to use LETIMER0 present on EFR32FG23 (Series 2) as system tick.

LETIMER0 is clocked from EM23GRPACLK (LFXO/LFRCO/ULFRCO), so the system
tick keeps running in EM2/EM3 deep sleep — ideal for battery-powered devices.

1) In your mcuconf.h make sure that:

#define EFR32_ST_USE_TIMER              1       // 1 = LETIMER0

   and that the LETIMER0 clock source is selected, e.g.:

#define EFR32_EM23GRPACLKSEL            EFR32_EM23GRPACLKSEL_LFXO

   (or LFRCO / ULFRCO)

2) In your chconf.h make sure that:

#define CH_CFG_ST_RESOLUTION            32      // LETIMER0 CNT is 24-bit on FG23;
                                                // use 32 (OSAL rounds down)
#define CH_CFG_ST_TIMEDELTA             8
#define CH_CFG_ST_FREQUENCY             4096    // LFXO 32768 Hz / DIV8 prescaler = 4096 Hz

   The LETIMER0 counter prescaler is derived from CH_CFG_ST_FREQUENCY:
   LFXO 32768 Hz / CNTPRESC = CH_CFG_ST_FREQUENCY.

You can't use osDelay() or other "sleeping" functions for periods
longer than:
         LETIMER_CNT_MAX + 1               16777216 (24-bit)
----------------------------------- =  ------------------- seconds.
LETIMER_CLOCK / CNTPRESC            CH_CFG_ST_FREQUENCY

For instance, with CH_CFG_ST_FREQUENCY = 4096 the max delay is ~4096 seconds.

Known good combinations of CH_CFG_ST_FREQUENCY/CH_CFG_ST_TIMEDELTA:
a) 1000/8 or 1024/8
b) 2000/4 or 2048/4
c) 4000/2 or 4096/2
d) 8000/2 or 8192/2
