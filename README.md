# pi-audio
The goal of this project is to build a system for wireless streaming of audio to
multiple speakers, similar in concept to the Sonos system. The master/server
program runs on a desktop or laptop, and connects to multiple Pi units over WiFi
(or Ethernet).

# Installation and Usage
The Pi should be running [Arch Linux ARM](http://archlinuxarm.org/) with the
`libsndfile` package installed. Clone the repository and run the `make` command.
Run `./pi-audio-client` on the Pi, and `./pi-audio-master` on the controlling
computer. Currently all configuration is done by changing the source code.
