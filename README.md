# rfsend
Control RF Switches such as these ones by Mercury via the Beaglebone PRU.

![Mercury RC5 Switch](/images/rc5.jpg)

As well as your Beaglebone you will need a cheap 433mhz transmitter. Ebay is a good
source. Connect Vcc to P9.07 (+5v SYS), Gnd to P9.1 (DGND) & Data to P8.11. 

![Transmitter](/images/tx.jpg)

The Beaglebones, apart from being completely open source, have one big advantage over their
rivals such as the Raspberry pi. The AM3358 Sitara processor used contains not only an ARM
Cortex-A8 but also 2 32 bit 200MHz microcontrollers, known as PRU's (Programmable Realtime Units).
rfsend uses one of these to create the pulse train for the 433mhz transmitter. They run independently of the main 
processor and are completely deterministic - making them great for doing timing critical
tasks such as this.

rfsend uses the Linux Userspace IO (UIO) method of communicating with the PRU's. Ensure
you have this configured properly on your linux distribution. I'm using Debian 9.1
with a 4.4.88-ti kernel. In order to use uio instead of the default rproc, modify your
/boot/uEnv.txt as shown:

'''
###PRUSS OPTIONS
###pru_rproc (4.4.x-ti kernel)
#uboot_overlay_pru=/lib/firmware/AM335X-PRU-RPROC-4-4-TI-00A0.dtbo
###pru_uio (4.4.x-ti & mainline/bone kernel)
uboot_overlay_pru=/lib/firmware/AM335X-PRU-UIO-00A0.dtbo
'''

Reboot & do a '''lsmod | grep uio''' Check you get
'''
uio_pruss               4629  0
uio_pdrv_genirq         4243  0
uio                    11100  2 uio_pruss,uio_pdrv_genirq
'''

You will also need to load a cape to configure the Beaglebone pins.
'''sudo sh -c "echo cape-universal > /sys/devices/platform/bone_capemgr/slots"'''

or, if you want it loaded at boot, add it to '/etc/default/capemgr':

```
# Default settings for capemgr. This file is sourced by /bin/sh from
# /etc/init.d/capemgr.sh

# Options to pass to capemgr
# CAPE=BB-UART1,BB-UART2
CAPE=cape-universal
```

Now 'cat /sys/devices/platform/bone_capemgr/slots' - You should now see something like:
```
0: PF----  -1 
 1: PF----  -1 
 2: PF----  -1 
 3: PF----  -1 
 4: P-O-L-   0 Override Board Name,00A0,Override Manuf,cape-universal
 ```
 
 Finally, connect the P8.11 pin to PRU0 with '''config-pin P8.11 pruout'''
 If you don't have the config-pin script installed you can use this instead
 '''sudo sh -c "echo pruout > /sys/devices/platform/ocp/ocp\:P8_11_pinmux/state"'''
  (You might want to add that to a bootup script so the pin mode survives reboots.)

 

![Code Table](/images/codes.jpg)
