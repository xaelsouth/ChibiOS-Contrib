# Generic matrix keypad driver (`devices_lib/keypad`)

Scans an `n_rows x n_cols` key matrix. Rows are GPIO lines driven low one at
a time (others hi-Z); columns are read through an ADC channel each via a
caller-supplied callback, so the driver works with **any** ADC — on i.MX23
use the HAL's `imx23_lradc_read()` (the MXS LRADC driver lives in the i.MX23
port, `os/hal/ports/IMX23/IMX23xx/imx23_lradc.*`, since that register block
is MXS-specific), or any other HAL ADC driver.

## Wiring schematic (4x4 example, one ADC channel per column)

```
                 VDDA (ADC reference, e.g. 3V3)
                  │
                 ┌┴┐ R_pullup (e.g. 10k) on each column
                 │ │
   col ADC ch0 ──┼─┴────┬──────────┬──────────┬──────────┐
   col ADC ch1 ──┼──────┼───┐      │          │          │
   col ADC ch2 ──┼──────┼───┼──────┼──┐       │          │
   col ADC ch3 ──┼──────┼───┼──────┼──┼───────┼──┐       │
                 │      │   │      │  │       │  │       │
   row GPIO 0 ───┼──────┼───┼───── KEY ──── KEY ───── KEY ...
   row GPIO 1 ───┼──────┼── KEY ─────┼──┼──────┼──┼── KEY
   row GPIO 2 ───┼────── KEY ────────┼──┼──── KEY ───┼──
   row GPIO 3 ─── KEY ────────────────┼──┼──────┼── KEY
                 │      │   │      │  │       │  │       │
                GND    ... (each key bridges one row line to one column line)

   MCU                          key matrix (4x4)
   ┌─────────┐                ┌───────────────────────────────┐
   │ GPIO r0 ├───────────────►│ row 0                         │
   │ GPIO r1 ├───────────────►│ row 1                         │
   │ GPIO r2 ├───────────────►│ row 2                         │
   │ GPIO r3 ├───────────────►│ row 3                         │
   │ ADC c0  │◄──10k──VDDA───┤ col 0                         │
   │ ADC c1  │◄──10k──VDDA───┤ col 1                         │
   │ ADC c2  │◄──10k──VDDA───┤ col 2                         │
   │ ADC c3  │◄──10k──VDDA───┤ col 3                         │
   └─────────┘                └───────────────────────────────┘
```

Scan algorithm (`keypadScan()`): for each row, drive it low (open-drain),
wait `KEYPAD_SETTLE_MS`, read every column channel; a raw value below
`press_threshold` means the key at (row, col) is pressed. Returns the
`keymap` character, or `0` when nothing is pressed.

## Usage (i.MX23 EVK, 4x4 keypad on LRADC pads 0-3, rows on GPIO bank1)

```c
static MXSLRAdcDriver-ish ctx;   /* on i.MX23: pass &IMX23 LRADC via wrapper */
static uint32_t adc_read_cb(void *ctx, uint8_t ch, sysinterval_t to) {
  return imx23_lradc_read(ch, to);            /* HAL LRADC driver */
}

static const ioline_t rows[4] = { LINE_GPIO_1_0, LINE_GPIO_1_1,
                                  LINE_GPIO_1_2, LINE_GPIO_1_3 };
static const uint8_t  cols[4] = { 0, 1, 2, 3 };        /* LRADC pads */
static const char keymap[16] = "123A456B789C*0#D";
static Keypad kp;

keypadInit(&kp, adc_read_cb, NULL, rows, 4, cols, 4, keymap, 0x8000);
char k = keypadScan(&kp);      /* '5' when the center key is pressed */
```

## 4-wire resistive touch (for reference — uses the port LRADC driver)

The touch part is MXS-specific (CTRL0 plate switches), so it stays in the
i.MX23 HAL driver `imx23_lradc_touch_read()`; wiring:

```
   4-wire panel        i.MX23 LRADC
   ┌─────────┐        ┌──────────────────────────────┐
   │ X+ ─────┼───────►│ LRADC pad (x_sense), XPULSW  │◄── driven by CTRL0
   │ X- ─────┼───────►│ LRADC pad, XNURSW            │◄── driven by CTRL0
   │ Y+ ─────┼───────►│ LRADC pad (y_sense), YPLLSW  │
   │ Y- ─────┼───────►│ LRADC pad, YNLRSW            │
   └─────────┘        └──────────────────────────────┘
   X measurement: drive X+ high / X- low, sense Y plate via y_sense pad.
   Y measurement: drive Y+ high / Y- low, sense X plate via x_sense pad.
```
