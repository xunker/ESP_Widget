### ESP Widget

ESP8266/Arduino code for an OLED-based desktop widget.

#### Goals

The end goal is to be able to build a simple, stand-alone WiFI desktop widget using only the basic ESP-01 module.

Currently it supports no buttons because of the lack of pins, but future version may re-purpose the TX/RX pins as GPIO for buttons.

#### Design

Intended to use [Stefan Ries' 3D-printed OLED Desktop Widget case](http://www.thingiverse.com/thing:857858). Code is based off [Danbick's ESP Wifi Scanner demo code](http://www.esp8266.com/viewtopic.php?p=18447).

The code is designed for a 128x64 OLED using I2C (see BoM for more info), but should be completely compatible with with a 128x32 OLED display too.

#### I/O and Code

The code is written for Arduino IDE v 1.6.5 or higher. It assumes the I2C address of the display will be `0x3C` and that `SDA` is `GPIO-0` and `SCL` is `GPIO-2`.

It will connect to the WiFi Network of your choice and then poll a given URL every 60 seconds. It will then display text (8 lines of 16 characters) or an image bitmap (future design, not yet working).

It expects the HTTP response from the URL to look like this:

```
HTTP/1.1 200 OK
Content-Type: text/plain
Content-Length: 131

TEXT:
1234567890123456
The Quick Brown
Fox Jumped Over
The Lazy Dog.
`1234567890-=
~!@#$%^&*()_+?
[]\{}|;':",./<>
----------------
```

It will ignore the header and will only look for the marker `TEXT:\n`, which signals the beginning of the text to write to the screen. It then reads each line until the end of the response and writes that text to the OLED. Lines longer than 16 characters will truncated. It expects the `TEXT:` and all subsequent text lines to be terminated by `\n` (Line Feed, 0x0A in hex or 10 in dec) and NOT `\r\n` (MS-DOS CRLF).

If you have a 128x32 OLED your `TEXT:` block should only contain 4 lines. If you provide more lines than your display can show, the display *should* ignore them but [YMMV](https://en.wiktionary.org/wiki/your_mileage_may_vary).

In the future you will also be able to instead give a response as a bitmap to write directly to the screem:

```
HTTP/1.1 200 OK
Content-Type: text/plain
Content-Length: 1284

BITMAP:
0x00
0x00
0x80
0x80
..etc..

```

..but not yet.

#### BoM (Bill of Materials)

* ESP-01 with minimum of 512KB of flash.
* 128x64 I2C OLED display.
  - I used [this one from Amazon](http://www.amazon.com/Diymall-Serial-128x64-Display-Arduino/dp/B00O2KDQBE).
* USB Power connector.
  - I recommend a [breakout board like this](https://www.adafruit.com/products/1764)).
* 3.3V regulator.
  Remember that the WiFi Radio uses a lot of power, so I recommend an the [LD1117](https://www.adafruit.com/products/2165) or similar. If you want more efficiency you can also use a switching regulator like the [TSR12433](https://www.adafruit.com/products/1066), but they are **very** expensive. The [L4931](https://www.adafruit.com/products/2166) may tempt you because of its small size, but it does not have the current capacity to run the Wifi Radio.

For burning the code you can also use an [ESP-01 breakout board](https://www.tindie.com/products/FemtoCow/esp8266-ftdi-and-breadboard-adapter-with-33v-reg/), but that won't fit inside the 3D-printed case.
