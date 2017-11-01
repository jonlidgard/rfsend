```Usage: rfsend [OPTION...] [COMMAND...]
For Beaglebone Microcontrollers only.
Sends a string of commands over RF to control RC5 type switches  

  -d, --debug                Produce debug output
  -i, --invert               Invert signal
  -l, --length[=BITS]        Command length (default 24) bits
  -p, --protocol[=ID]        Message protocol id (default 1)
  -r, --repeat[=COUNT]       Repeat the output COUNT (default 3) times
  -t, --time[=MICROSECS]     Override default protocol pulse timing

  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

Uses PRU0 via the pru_uio overlay.
Ensure you have an appropriate cape loaded
  (e.g. cape_universal + config-pin p8.11 pruout)
  Connect P8_11 to <DATA> on a cheap 433Mhz transmitter module
  Protocol ID: 1 - PT2260 (350us pulse timing)
  Protocol ID: 2 - ?? (650us pulse timing)
  Protocol ID: 3 - ?? (100us pulse timing)
  Protocol ID: 4 - ?? (380us pulse timing)
  Protocol ID: 5 - ?? (500us pulse timing)
  Protocol ID: 6 - HT6P20B (450us pulse timing)

  ** MUST BE RUN WITH ROOT PRIVILEGES **
```

See the wiki for more info: