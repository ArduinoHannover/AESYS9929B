# AESYS9929B
Library for driving AESYS9929B Displays (üstra TW6000).

## Connection

Connect at the back left (there should be a flatband cable attached).

You can either solder Headers or Cables to the existing header, cut the cable or anything you can imagine ;-)

![Connecion Diagram](https://raw.githubusercontent.com/ArduinoHannover/AESYS9929B/master/connection.png)

`DAT_H` (`D_H`) is not needed if jumper (below CLK_H) is set to left position ("first panel"), just connect `DAT_L`/`D_L`

`RCK` is Latch
a
`LIGHT` is the light sensor in front of the board.

## Power requirements

The panels are supposed to be driven by 24V.

| Unit | est. Current |
| --- | ---: |
| Pre-Filter | 10.3 mA |
| Step-Down (per Panel) | 67.8 mA |
| Logic (per Panel) | 20 mA |
| LED (per 100% active) | 2.6 mA |

So per Panel there will be ~2.7 W Power draw in idle.

Full brightness with one 40x16px Board will be around 40W (160px wide ≙ 160W / 7A @ 24V).

You may change from 24V to 5V but keep the current in mind (32A @ 5V).
I do not recommend a full 5V system for large displays.
