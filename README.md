# BVG Departure Board

A small LED matrix board that hangs on my wall and shows the next departures from my local
BVG bus stop, the same way the real BVG displays look. Two 64x32 panels chained
together, driven by an ESP32-S3. It refreshes every 20 seconds.

## The API

The board pulls departures from a [vbb-rest](https://github.com/derhuerst/vbb-rest) instance
that I run myself:

```
https://api.public.geeek.me
```

The board only calls this endpoint with:

```
{base}/{stopId}/departures?results=20&duration=60
```

## Parts

- Waveshare ESP32-S3 RGB Matrix Driver Board
- 2x P2.5 indoor full colour LED module, HUB75, 160x80 mm, 64x32 pixels
- PD3.0/PD3.1/QC3.0 trigger module, USB-C to DC, set to 5V (default)
- KCD1 rocker switch, 10x15 mm
- 8x M3x15m screws
- 20 AWG wire

The enclosure is 3d printed. I'll add the Onshape link once it's tidied up.

Power goes straight to the panels and the driver board from the trigger module, through
the rocker switch, not using the onboard usb c connector.

## Flashing

PlatformIO. The board definition is in `boards/`, needs to be changed if a different board is used.

```bash
pio run -t upload
```

Serial monitor runs at 115200:

```bash
pio device monitor
```

If the board doesn't show up as a serial device, hold BOOT while plugging in the USB-C cable.

## Config

Everything is set up over wifi, there's nothing to hardcode.

On first boot the board opens an access point called **BVG-Display-Setup**. Connect to it,
the config page should open by itself, otherwise go to `192.168.4.1`. Pick your wifi and
fill in the rest of the config.

| Field | What it does |
| --- | --- |
| Stop ID | VBB stop id, e.g. `900086107`. Look it up at `/locations?query=...` on the API |
| Lines | Comma separated, e.g. `221,125`. Leave empty for all lines |
| Directions | Comma separated, has to match the direction string exactly. Empty for all |
| Brightness | 1-255, default 50 |
| API endpoint | Only if you're not using the default |

After it connects, the board shows its IP address for a couple of seconds. The same config
page stays reachable at that address, so you can change the stop or brightness later without
resetting anything. Settings are stored in NVS and survive a reboot.

## TODO

- show error codes on display
- add font to seperate repo
- enable single panel support
- show api requests in web ui (for debugging)
- idea: add flashing for bus if less than 30 seconds away
