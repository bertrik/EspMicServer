# EspMicServer
Records audio on an ESP8266 from an I2S microphone and makes it available as a network stream

## Hardware
The hardware consists of the following:
* wemos d1 mini board, containing an ESP8266
* INMP441 microphone, connected with dupont wire

Connections are as follows:

| Wemos | INMP441 | Remark |
| ----- | ------- |- |
| D5    | WS      | Sample clock |
| D6    | SD      | Data-out from the mic |
| D7    | SCK     | High speed bit clock|
| D8    | L/R     | Tells microphone which I2S channel to use  
| GND   | GND     ||
| 3V3   | VDD     ||

## Software

### ESP8266 firmware
The firmware uses platformio.

It initialises the microphone for 16000 Hz sample rate.
Pin D8 sets the I2S channel (left/right) that the microphone publishes data on. 

The firmware opens a port that external applications can connect to. Connecting to it causes a WAV header to be sent, followed by continuous mono 16-bit signed samples from the microphone. 

Build it using visual code + platformio plugin, or from the command line:

One-time initialisation to install platformio:
```bash
python3 -m venv .venv
source .venv/bin/activate
pip3 install platformio
```

Reinitialise the python virtual environment when working on the code again:
```bash
source .venv/bin/activate
```

Compile and upload the code:
```bash
pio run -t upload
pio device monitor
```

### Host-side audio receiver
There are several options.

One is to use `ffplay` from the ffmpeg package:
```
ffplay tcp://espmicserver.local:1234
```

Another is to use `sox` in combination with netcat `nc`:
```
nc espmicserver.local 1234 |play -t wav -
```
