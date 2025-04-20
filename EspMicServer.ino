#include <Arduino.h>

#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <I2S.h>

#include <WiFiManager.h>
#include <MiniShell.h>

#define printf Serial.printf
#define PIN_I2S_LR D8
#define SERVER_PORT 1234
#define AUDIO_BUFFER_SIZE 512

static WiFiManager wifiManager;
static MiniShell shell(&Serial);
static WiFiServer server(SERVER_PORT);
static WiFiClient client;
static int16_t buffer[AUDIO_BUFFER_SIZE];

static const uint8_t wav_header[44] = {
    'R', 'I', 'F', 'F',         // ChunkID
    0xFF, 0xFF, 0xFF, 0x7F,     // ChunkSize (placeholder, ignored by most tools)
    'W', 'A', 'V', 'E',         // Format
    'f', 'm', 't', ' ',         // Subchunk1ID
    16, 0x00, 0x00, 0x00,       // Subchunk1Size (16 for PCM)
    0x01, 0x00,                 // AudioFormat (1 = PCM)
    0x01, 0x00,                 // NumChannels (1 = mono)
    0x80, 0x3E, 0x00, 0x00,     // SampleRate (16000Hz)
    0x00, 0x7D, 0x00, 0x00,     // ByteRate = SampleRate * NumChannels * BitsPerSample/8
    0x02, 0x00,                 // BlockAlign = NumChannels * BitsPerSample/8
    0x10, 0x00,                 // BitsPerSample (16 bits)
    'd', 'a', 't', 'a',         // Subchunk2ID
    0xFF, 0xFF, 0xFF, 0x7F      // Subchunk2Size (also fake, set to 0x7FFFFFFF)
};

static void show_help(const cmd_t *cmds)
{
    for (const cmd_t * cmd = cmds; cmd->cmd != NULL; cmd++) {
        printf("%10s: %s\n", cmd->name, cmd->help);
    }
}

static int do_reboot(int argc, char *argv[])
{
    ESP.restart();
    return 0;
}

static int do_help(int argc, char *argv[]);
static const cmd_t commands[] = {
    { "help", do_help, "Show help" },
    { "reboot", do_reboot, "Reboot" },
    { NULL, NULL, NULL }
};

static int do_help(int argc, char *argv[])
{
    show_help(commands);
    return 0;
}

void setup(void)
{
    Serial.begin(115200);

    // configure i2s
    pinMode(PIN_I2S_LR, OUTPUT);        // L/R signal
    digitalWrite(PIN_I2S_LR, 0);
    i2s_set_rate(16000);

    // connect to wifi
    printf("Starting WIFI manager (%s)...\n", WiFi.SSID().c_str());
    wifiManager.setConfigPortalTimeout(120);
    wifiManager.autoConnect("EspMicServer");

    // configure server
    server.setNoDelay(true);
    server.begin();

    MDNS.begin("espmicserver");
    MDNS.addService("audio", "tcp", SERVER_PORT);
    MDNS.addServiceTxt("audio", "tcp", "desc", "WAV audio stream");

    // play audio with ffplay tcp://espmicserver.local:1234

    // start i2s
    i2s_rxtx_begin(true, false);
}

void loop(void)
{
    // network processing
    if (!client) {
        // connection setup
        client = server.accept();
        if (client) {
            printf("Client connected: %s\n", client.remoteIP().toString().c_str());

            // send initial wav header
            client.setNoDelay(true);
            client.write(wav_header, sizeof(wav_header));
        }
    } else {
        // data processing loop
        uint16_t avail = i2s_rx_available();
        if (avail > 0) {
            printf("%u.", avail);
            if (avail > AUDIO_BUFFER_SIZE) {
                avail = AUDIO_BUFFER_SIZE;
            }
            int index = 0;
            for (int i = 0; i < avail; i++) {
                int16_t left = 0;
                int16_t right = 0;
                i2s_read_sample(&left, &right, true);
                buffer[index++] = (left + right) / 2;
            }
            client.write((uint8_t *) buffer, index * 2);
        }
    }

    // parse command line
    shell.process(">", commands);

    // keep mdns updated
    MDNS.update();
}
