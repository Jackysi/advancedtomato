Cables
======

You can either use the cable that came with your
UPS (the easiest if we support it) or you can make your own cable.
We recommend that you obtain a supported cable directly from APC.

If you already have an APC cable, you can determine what kind it is
by examining the flat sides of the two connectors where you will
find the cable number embossed into the plastic. It is generally on
one side of the male connector.

To make your own cable you must first know whether you have a UPS
that speaks the apcsmart protocol or a "dumb" UPS that uses serial
port line voltage signalling.

If you have an smart UPS, and you build your own cable, build a Smart-Custom
cable (see `Smart-Custom Cable for SmartUPSes`_). If you have a 
voltage-signalling or dumb UPS, build a Simple-Custom cable (see 
`Simple-Custom Voltage-Signalling Cable for "dumb" UPSes`_). If you have a 
BackUPS CS with a RJ45 connector, you can build your own Custom-RJ45 cable
(see `Custom-RJ45 Smart Signalling Cable for BackUPS CS Models`_).

Smart-Custom Cable for SmartUPSes
---------------------------------

*You do not have this cable unless you built it yourself.
The Smart-Custom cable is not an APC product.*

::

           SMART-CUSTOM CABLE
         
         Signal Computer                  UPS
                DB9F                     DB9M
          RxD    2   --------------------  2  TxD  Send
          TxD    3   --------------------  1  RxD  Receive
          GND    5   --------------------  9  Ground

When using this cable with apcupsd specify the following in
apcupsd.conf:

::

         UPSCABLE smart
         UPSTYPE apcsmart
         DEVICE /dev/ttyS0 (or whatever your serial port is)

If you have an OS that requires DCD or RTS to be set before you can
receive input, you might try building the standard APC Smart
940-0024C cable listed below (see `940-0024C Cable Wiring`_).

Simple-Custom Voltage-Signalling Cable for "dumb" UPSes
-------------------------------------------------------

*You do not have this cable unless you built it yourself.
The Simple-Custom cable is not an APC product.*

For "dumb" UPSes using voltage signalling, if you are going to
build your own cable, we recommend to make the cable designed by
the apcupsd team as follows:

::

                SIMPLE-CUSTOM CABLE
         
         Signal Computer                  UPS
                DB9F   4.7K ohm          DB9M
          DTR    4   --[####]--*              DTR set to +5V by Apcupsd
                               |
          CTS    8   ----------*---------  5  Low Battery
          GND    5   --------------------  4  Ground
          DCD    1   --------------------  2  On Battery
          RTS    7   --------------------  1  Kill UPS Power

List of components one needs to make the Simple cable:

#. One (1) male DB9 connector, use solder type connector only.

#. One (1) female DB9/25F connector, use solder type connector
   only.

#. One (1) 4.7K ohm 1/4 watt 5% resistor.

#. rosin core solder.

#. three (3) to five (5) feet of 22AWG multi-stranded four or more
   conductor cable.

Assembly instructions:

#. Solder the resistor into pin 4 of the female DB9 connector.

#. Next bend the resistor so that it connects to pin 8 of the
   female DB9 connector.

#. Pin 8 on the female connector is also wired to pin 5 on the male
   DB9 connector. Solder both ends.

#. Solder the other pins, pin 5 on the female DB9 to pin 4 on the
   male connector; pin 1 on the female connector to pin 2 on the male
   connector; and pin 7 on the female connector to pin 1 on the male
   connector.

#. Double check your work.


We use the DTR (pin 4 on the female connector) as our +5 volts
power for the circuit. It is used as the Vcc pull-up voltage for
testing the outputs on any "UPS by APC" in voltage-signalling mode.
This cable may not work on a BackUPS Pro if the default
communications are in apcsmart mode. This cable is also valid for
use on a ShareUPS BASIC Port. It is reported to work on
SmartUPSes, however the Smart Cable described above is preferred.

To have a better idea of what is going on inside apcupsd,
for the SIMPLE cable apcupsd reads three signals and sets three:

    Reads:
        CD, which apcupsd uses for the On Battery signal when high.
         
        CTS, which apcupsd uses for the Battery Low signal when high.
         
        RxD (SR), which apcupsd uses for the Line Down
            signal when high. This signal isn't used for much.
         
    Sets:
        DTR, which apcupsd sets when it detects a power failure (generally
             5 to 10 seconds after the CD signal goes high). It
             clears this signal if the CD signal subsequently goes low
             -- i.e. power is restored.
         
        TxD (ST), which apcupsd clears when it detects that the CD signal
             has gone low after having gone high - i.e. power is restored.
         
        RTS, which apcupsd sets for the killpower signal -- to cause the UPS
             to shut off the power.

Please note that these actions apply only to the SIMPLE cable. The
signals used on the other cables are different.

Finally, here is another way of looking at the CUSTOM-SIMPLE
cable:

::

         APCUPSD SIMPLE-CUSTOM CABLE
         
         Computer Side  |  Description of Cable           |     UPS Side
         DB9f  |  DB25f |                                 |   DB9m  | DB25m
         4     |   20   |  DTR (5vcc)             *below  |    n/c  |
         8     |    5   |  CTS (low battery)      *below  | <-  5   |   7
         2     |    3   |  RxD (no line voltage)  *below  | <-  3   |   2
         5     |    7   |  Ground (Signal)                |     4   |  20
         1     |    8   |  CD (on battery from UPS)       | <-  2   |   3
         7     |    4   |  RTS (kill UPS power)           | ->  1   |   8
         n/c   |    1   |  Frame/Case Gnd (optional)      |     9   |  22
         
         Note: the <- and -> indicate the signal direction.

When using this cable with apcupsd specify the following in
apcupsd.conf:

::

         UPSCABLE simple
         UPSTYPE dumb
         DEVICE /dev/ttyS0 (or whatever your serial port is)

Custom-RJ45 Smart Signalling Cable for BackUPS CS Models
--------------------------------------------------------

If you have a BackUPS CS, you are probably either using it with the
USB cable that is supplied or with the 940-0128A supplied by APC,
which permits running the UPS in dumb mode. By building your own
cable, you can now run the BackUPS CS models (and perhaps also the
ES models) using smart signalling and have all the same information
that is available as running it in USB mode.

The jack in the UPS is actually a 10 pin RJ45. However, you can
just as easily use a 8 pin RJ45 connector, which is more standard
(ethernet TX, and ISDN connector). It is easy to construct the
cable by cutting off one end of a standard RJ45-8 ethernet cable
and wiring the other end (three wires) into a standard DB9F female
serial port connector.

Below, you will find a diagram for the CUSTOM-RJ45 cable:

::

           CUSTOM-RJ45 CABLE
         
         Signal Computer              UPS     UPS
                DB9F                 RJ45-8  RJ45-10
          RxD    2   ----------------  1      2     TxD  Send
          TxD    3   ----------------  7      8     RxD  Receive
          GND    5   ----------------  6      7     Ground
          FG  Shield ----------------  3      4     Frame Ground
         
         The RJ45-8 pins are: looking at the end of the connector:
         
          8 7 6 5 4 3 2 1
         ___________________
         | . . . . . . . . |
         |                 |
         -------------------
                |____|
         
         The RJ45-10  pins are: looking at the end of the connector:
         
         10 9 8 7 6 5 4 3 2 1
         _______________________
         | . . . . . . . . . . |
         |                     |
         -----------------------
                |____|

For the serial port DB9F connector, the pin numbers are stamped in
the plastic near each pin. In addition, there is a diagram near the
end of this chapter.

Note, one user, Martin, has found that if the shield is not
connected to the Frame Ground in the above diagram (not in our
original schematic), the UPS (a BackUPS CS 500 EI) will be unstable
and likely to rapidly switch from power to batteries (i.e.
chatter).

When using this cable with apcupsd specify the following in
apcupsd.conf:

::

         UPSCABLE smart
         UPSTYPE apcsmart
         DEVICE /dev/ttyS0 (or whatever your serial port is)

The information for constructing this cable was discovered and
transmitted to us by slither_man. Many thanks!

Other APC Cables that apcupsd Supports
--------------------------------------

apcupsd will also support the following off the shelf cables that
are supplied by APC


-  940-0020[B/C] Simple Signal Only, all models.
-  940-0023A Simple Signal Only, all models.
-  940-0119A Simple Signal Only, Back-UPS Office, and BackUPS ES.
-  940-0024[B/C/G] Smart mode Only, SU and BKPro only.
-  940-0095[A/B/C] PnP (Plug and Play), all models.
-  940-1524C Smart mode Only
-  940-0128A Simple Signal Only, Back-UPS CS in serial mode.
-  All USB cables such as 940-0127[A/B]


Voltage Signalling Features Supported by Apcupsd for Various Cables
-------------------------------------------------------------------

The following table shows the features supported by the current
version of apcupsd for various cables running the UPS in
voltage-signalling mode.

============= ========== =========== ========== ==================
Cable         Power Loss Low Battery Kill Power Cable Disconnected
============= ========== =========== ========== ==================
940-0020B     Yes        No          Yes        No
940-0020C     Yes        Yes         Yes        No
940-0023A     Yes        No          No         No
940-0119A     Yes        Yes         Yes        No
940-0127A     Yes        Yes         Yes        No
940-0128A     Yes        Yes         Yes        No
940-0095A/B/C Yes        Yes         Yes        No
simple        Yes        Yes         Yes        No
============= ========== =========== ========== ==================


Voltage Signalling
------------------

Apparently, all APC voltage-signalling UPSes with DB9 serial ports
have the same signals on the output pins of the UPS. The difference
at the computer end is due to different cable configurations. Thus,
by measuring the connectivity of a cable, one can determine how to
program the UPS.

The signals presented or accepted by the UPS on its DB9 connector
using the numbering scheme listed above is:

::

    UPS Pin         Signal meaning
     1     <-     Shutdown when set by computer for 1-5 seconds.
     2     ->     On battery power (this signal is normally low but
                       goes high when the UPS switches to batteries).
     3     ->     Mains down (line fail) See Note 1 below.
     5     ->     Low battery. See Note 1 below.
     6     ->     Inverse of mains down signal. See Note 2 below.
     7     <-     Turn on/off power (only on advanced UPSes only)

     Note 1: these two lines are normally open, but close when the
         appropriate signal is triggered. In fact, they are open collector
         outputs which are rated for a maximum of +40VDC and 25 mA. Thus
         the 4.7K ohm resistor used in the Custom Simple cable works
         quite well.

     Note 2: the same as note 1 except that the line is normally closed,
         and opens when the line voltage fails.

The Back-UPS Office 500 signals
-------------------------------

The Back-UPS Office UPS has a telephone type jack as output, which
looks like the following:

::

         Looking at the end of the connector:
         
            6 5 4 3 2 1
           _____________
          | . . . . . . |
          |             |
          |  |----------|
          |__|

It appears that the signals work as follows:

::

           UPS            Signal meaning
         1 (brown)    <-   Shutdown when set by computer for 1-5 seconds.
         2 (black)    ->   On battery power
         3 (blue)     ->   Low battery
         4 (red)           Signal ground
         5 (yellow)   <-   Begin signalling on other pins
         6 (none)          none

Analyses of APC Cables
----------------------

940-0020B Cable Wiring
~~~~~~~~~~~~~~~~~~~~~~
:Supported Models: Simple Signaling such as BackUPS
:Contributed by: Lazar M. Fleysher

Although we do not know what the black box semiconductor contains,
we believe that we understand its operation (many thanks to Lazar
M. Fleysher for working this out).

This cable can only be used on voltage-signalling UPSes, and
provides the On Battery signal as well as kill UPS power. Most
recent evidence (Lazar's analysis) indicates that this cable under
the right conditions may provide the Low Battery signal. This is
yet to be confirmed.

*This diagram is for informational purposes and may not be complete. 
We don't recommend that use it to build you build one yourself.*

::

         APC Part# - 940-0020B
    
         Signal Computer                  UPS
                DB9F                     DB9M
          CTS    8   --------------------  2  On Battery
          DTR    4   --------------------  1  Kill power
          GND    5   ---------------*----  4  Ground
                                    |
                         ---        *----  9  Common
          DCD    1  ----|///|-----------   5  Low Battery
                        |\\\|
          RTS    7  ----|///| (probably a
                         ---   semi-conductor)

940-0020C Cable Wiring
~~~~~~~~~~~~~~~~~~~~~~

:Supported Models: Simple Signaling such as BackUPS

This cable can only be used on voltage-signalling UPSes, and
provides the On Battery signal, the Low Battery signal as well as
kill UPS power. You may specify ``UPSCABLE 940-0020C``.

*This diagram is for informational purposes and may not be complete. 
We don't recommend that use it to build you build one yourself.*

::

         APC Part# - 940-0020C
    
         Signal Computer                  UPS
                DB9F                     DB9M
          CTS    8   --------------------  2  On Battery
          DTR    4   --------------------  1  Kill power
          GND    5   ---------------*----  4  Ground
                                    |
                                    *----  9  Common
          RTS    7 -----[ 93.5K ohm ]----- 5  Low Battery
                        or semi-conductor

940-0023A Cable Wiring
~~~~~~~~~~~~~~~~~~~~~~

:Supported Models: Simple Signaling such as BackUPS


This cable can only be used on voltage-signalling UPSes, and
apparently only provides the On Battery signal. As a consequence,
this cable is pretty much useless, and we recommend that you find a
better cable because all APC UPSes support more than just On
Battery. Please note that we are not sure the following diagram is
correct.

*This diagram is for informational purposes and may not be complete. 
We don't recommend that use it to build you build one yourself.*

::

         APC Part# - 940-0023A
         
         Signal Computer                  UPS
                DB9F                     DB9M
          DCD    1   --------------------  2  On Battery
         
                       3.3K ohm
          TxD    3   --[####]-*
                              |
          DTR    4   ---------*
          GND    5   ---------------*----  4  Ground
                                    |
                                    *----  9  Common

940-0024C Cable Wiring
~~~~~~~~~~~~~~~~~~~~~~

:Supported Models: SmartUPS (all models with DB9 serial port)

If you wish to build the standard cable furnished by APC
(940-0024C), use the following diagram.

::

         APC Part# - 940-0024C
         
         Signal Computer                  UPS
                DB9F                     DB9M
          RxD    2   --------------------  2  TxD  Send
          TxD    3   --------------------  1  RxD  Receive
          DCD    1   --*
                       |
          DTR    4   --*
          GND    5   --------------------  9  Ground
          RTS    7   --*
                       |
          CTS    8   --*

940-0095A Cable Wiring
~~~~~~~~~~~~~~~~~~~~~~

:Supported Models: APC BackUPS Pro PNP
:Contributed by: Chris Hanson cph at zurich.ai.mit.edu

This is the definitive wiring diagram for the 940-0095A cable
submitted by Chris Hanson, who disassembled the original cable,
destroying it in the process. He then built one from his diagram
and it works perfectly.

::

         APC Part# - 940-0095A
    
         UPS end                                      Computer end
         -------                                      ------------
                           47k        47k
         BATTERY-LOW (5) >----R1----*----R2----*----< DTR,DSR,CTS (4,6,8)
                                  |          |
                                  |          |
                                  |         /  E
                                  |       |/
                                  |    B  |
                                  *-------|  2N3906 PNP
                                          |
                                          |\
                                            \  C
                                             |
                                             |
                                             *----< DCD (1)     Low Batt
                                             |
                                             |
                                             R 4.7k
                                             3
                                             |
                                      4.7k   |
         SHUTDOWN (1)    >----------*----R4----*----< TxD (3)
                                  |
                                  |  1N4148
                                  *----K|---------< RTS (7)      Shutdown
         
         POWER-FAIL (2)  >--------------------------< RxD,RI (2,9) On Batt
         
         GROUND (4,9)    >--------------------------< GND (5)

Operation:


-  DTR is "cable power" and must be held at SPACE. DSR or CTS may
   be used as a loopback input to determine if the cable is plugged
   in.

-  DCD is the "battery low" signal to the computer. A SPACE on this
   line means the battery is low. This is signalled by BATTERY-LOW
   being pulled down (it is probably open circuit normally).

   Normally, the transistor is turned off, and DCD is held at the MARK
   voltage by TxD. When BATTERY-LOW is pulled down, the voltage
   divider R2/R1 biases the transistor so that it is turned on,
   causing DCD to be pulled up to the SPACE voltage.

-  TxD must be held at MARK; this is the default state when no data
   is being transmitted. This sets the default bias for both DCD and
   SHUTDOWN. If this line is an open circuit, then when BATTERY-LOW is
   signalled, SHUTDOWN will be automatically signalled; this would be
   true if the cable were plugged in to the UPS and not the computer,
   or if the computer were turned off.

-  RTS is the "shutdown" signal from the computer. A SPACE on this
   line tells the UPS to shut down.

-  RxD and RI are both the "power-fail" signals to the computer. A
   MARK on this line means the power has failed.

-  SPACE is a positive voltage, typically +12V. MARK is a negative
   voltage, typically -12V. Linux appears to translate SPACE to a 1
   and MARK to a 0.


940-0095B Cable Wiring
~~~~~~~~~~~~~~~~~~~~~~

:Supported Models: Many simple-signaling (aka voltage signaling) 
    models such as BackUPS

*This diagram is for informational purposes and may not be complete. 
We don't recommend that use it to build you build one yourself.*

::

         APC Part# - 940-0095B
         
         Signal Computer                  UPS
                DB9F                     DB9M
          DTR    4   ----*
          CTS    8   ----|
          DSR    6   ----|
          DCD    1   ----*
          GND    5   ---------------*----  4  Ground
                                    |
                                    *----  9  Common
          RI     9   ----*
                         |
          RxD    2   ----*---------------  2  On Battery
          TxD    3   ----------[####]----  1  Kill UPS Power
                               4.7K ohm

940-0119A Cable Wiring
~~~~~~~~~~~~~~~~~~~~~~

:Supported Models: Older BackUPS Office

*This diagram is for informational purposes and may not be complete. 
We don't recommend that use it to build you build one yourself.*

::

         APC Part# - 940-0119A
         
           UPS      Computer
           pins     pins      Signal             Signal meaning
         1 (brown)    4,6      DSR DTR     <-   Shutdown when set by computer for 1-5 seconds.
         2 (black)    8,9      RI  CTS     ->   On battery power
         3 (blue)     1,2      CD  RxD     ->   Low battery
         4 (red)       5       Ground
         5 (yellow)    7       RTS         <-   Begin signalling on other pins
         6 (none)     none

Serial BackUPS ES Wiring
~~~~~~~~~~~~~~~~~~~~~~~~

:Supported Models: Older Serial BackUPS ES
:Contributed by: William Stock

The BackUPS ES has a straight through serial cable with no
identification on the plugs. To make it work with apcupsd, specify
the { UPSCABLE 940-0119A} and { UPSTYPE backups}. The equivalent of
cable 940-0119A is done on a PCB inside the unit.

::

         computer           ----------- BackUPS-ES -----------------
         DB9-M              DB-9F
         pin    signal      pin
         
          4      DSR   ->    4 --+
                                 |  diode   resistor
          6      DTR   ->    6 --+---->|----/\/\/\---o kill power
         
          1      DCD   <-    1 --+
                                 |
          2      RxD   <-    2 --+----------------+--o low battery
                                                  |
          7      RTS   ->    7 --------+--/\/\/\--+
                                       |
                                       +--/\/\/\--+
                                                  |
          8      RI    <-    8 --+----------------+--o on battery
                                 |
          9      CTS   <-    9 --+
         
          5      GND   ---   5 ----------------------o ground
         
          3      TxD         3 nc

940-0128A Cable Wiring
~~~~~~~~~~~~~~~~~~~~~~

:Supported Models: Older USB BackUPS ES and CS
:Contributed by: Many, thanks to all for your help!

Though these UPSes are USB UPSes, APC supplies a serial cable
(typically with a green DB9 F connector) that has 940-0128A stamped
into one side of the plastic serial port connector. The other end
of the cable is a 10 pin RJ45 connector that plugs into the UPS
(thanks to Dean Waldow for sending a cable!). Apcupsd version 3.8.5
and later supports this cable when specified as { UPSCABLE
940-0128A} and { UPSTYPE dumb}. However, running in this mode much
of the information that would be available in USB mode is lost. In
addition, when apcupsd attempts to instruct the UPS to kill the
power, it begins cycling about 4 times a second between battery and
line. The solution to the problem (thanks to Tom Suzda) is to
unplug the UPS and while it is still chattering, press the power
button (on the front of the unit) until the unit beeps and the
chattering stops. After that the UPS should behave normally and
power down 1-2 minutes after requested to do so.

Thanks to all the people who have helped test this and have
provided information on the cable wiring, our best guess for the
cable schematic is the following:

::

         APC Part# - 940-0128A
    
         computer      --------- Inside the Connector---------  UPS
         DB9-F         |                                     |  RJ45
         pin - signal  |                                     |  Pin - Color
                       |                                     |
          4     DSR  ->|---+                                 |
                       |   |  diode   resistor               |
          6     DTR  ->|---+---->|----/\/\/\---o kill power  |  8  Orange
                       |                                     |
          1     DCD  <-|----+                                |
                       |    |                                |
          2     RxD  <-|----+----------------+--o low battery|  3  Brown
                       |                     |               |
          7     RTS  ->|----------+--/\/\/\--+               |
                       |          |                          |
                       |          +--/\/\/\--+               |
                       |                     |               |
          8     RI   <-|----+----------------+--o on battery |  2  Black
                       |    |                                |
          9     CTS  <-|----+                                |
                       |                         signal      |
          5     GND  --|-----------------------o ground      |  7  Red
                       |                                     |
          3     TxD    |                                     |
                       |                         chassis     |
          Chassis/GND  |-----------------------o ground      |  4  Black
                       |                                     |
                       |          Not connected              |  1, 5, 6, 9, 10
                       --------------------------------------
         
         The RJ45 pins are: looking at the end of the connector:
         
         10 9 8 7 6 5 4 3 2 1
         _______________________
         | . . . . . . . . . . |
         |                     |
         -----------------------
                |____|

940-0128D Cable Wiring
~~~~~~~~~~~~~~~~~~~~~~

:Supported Models: BackUPS XS1000(BX-1000), Possibly other USB models

:Contributed by: Jan Babinski jbabinsk at pulsarbeacon dot com

940-0128D is functionally similar to the 940-0128A cable except for
NC on (6) DTR and (2) RD on the computer side.

Unverified: Try setting apcupsd to ``UPSTYPE dumb`` and ``UPSCABLE 940-0128A``.

::

         APC Part# - 940-0128D
         
         DB9(Computer)               RJ45-10(UPS)
         
          (5)     (1)                 ____________
         ( o o o o o )               [ oooooooooo ]
          \ o o o o /                [____________]
           (9)   (6)                 (10)  [_]  (1)
         
         
          RI(9)<---+
                   |
         CTS(8)<---+--- E   2N2222(NPN)
                         \|___
                    ____ /| B |
                   |    C     |
                   |          |
                   +---vvvv---+--[>|------<(2)OnBatt
         RTS(7)>---|    2k      1N5819
                   +---vvvv---+--[>|------<(3)LowBatt
                   |          |
                   +--- C     |
                         \|___|
                         /| B
         DCD(1)<------- E    2N2222(NPN)
         
         DTR(4)>-------------------------->(8)KillPwr
         
         GND(5)----------------------------(7)Signal GND
         (Shield)--------------------------(4)Chassis GND

940-0127B Cable Wiring
~~~~~~~~~~~~~~~~~~~~~~

:Supported Models: BackUPS XS1000(BX-1000), Possibly other USB models
:Contributed by: Jan Babinski jbabinsk at pulsarbeacon dot com

Standard USB cable for USB-capable models with 10-pin RJ45 connector.

::

         APC Part# - 940-0127B
    
         USB(Computer)      RJ45-10(UPS)
          _________          ____________
         | = = = = |        [ oooooooooo ]
         |_________|        [____________]
          (1)   (4)         (10)  [_]  (1)
         
           +5V(1)-----------(1)+5V
         DATA+(2)-----------(9)DATA+
         DATA-(3)-----------(10)DATA-
           GND(4)-----------(7)Signal GND
         (Shield)-----------(4)Chassis GRND



Win32 Implementation Restrictions for Simple UPSes
--------------------------------------------------

Due to inadequacies in the
Win32 API, it is not possible to set/clear/get all the serial port
line signals. apcupsd can detect: CTS, DSR, RNG, and CD. It can set
and clear: RTS and DTR.

This imposes a few minor restrictions on the functionality of some
of the cables. In particular, LineDown on the Custom Simple cable,
and Low Battery on the 0023A cable are not implemented.


Internal Apcupsd Actions for Simple Cables
------------------------------------------

::

         This section describes how apcupsd 3.8.5 (March 2002)
         treats the serial port line signals for simple cables.
         
         apcaction.c:
          condition = power failure detected
          cable = CUSTOM_SIMPLE
          action = ioctl(TIOCMBIS, DTR)      set DTR (enable power bit?)
         
         apcaction.c:
          condition = power back
          cable = CUSTOM_SIMPLE
          action = ioctl(TIOCMBIC, DTR)      clear DTR (clear power bit)
          action = ioctl(TIOCMBIC, ST)       clear ST (TxD)
         
         apcserial.c:
          condition = serial port initialization
          cable = 0095A, 0095B, 0095C
          action = ioctl(TIOMBIC, RTS)       clear RTS (set PnP mode)
         
          cable = 0119A, 0127A, 0128A
          action = ioctl(TIOMBIC, DTR)       clear DTR (killpower)
          action = ioctl(TIOMBIS, RTS)       set   RTS (ready to receive)
         
         apcserial.c:
          condition = save_dumb_status
          cable = CUSTOM_SIMPLE
          action = ioctl(TIOMBIC, DTR)       clear DTR (power bit?)
          action = ioctl(TIOMBIC, RTS)       clear RTS (killpower)
         
          cable = 0020B, 0020C, 0119A, 0127A, 0128A
          action = ioctl(TIOMBIC, DTR)       clear DTR (killpower)
         
          cable = 0095A, 0095B, 0095C
          action = ioctl(TIOMBIC, RTS)       clear RTS (killpower)
          action = ioctl(TIOMBIC, CD)        clear DCD (low batt)
          action = ioctl(TIOMBIC, RTS)       clear RTS (killpower) a second time!
         
         apcserial.c:
          condition = check_serial
         
          cable = CUSTOM_SIMPLE
          action = OnBatt = CD
          action = BattLow = CTS
          action = LineDown = SR
         
          cable = 0020B, 0020C, 0119A, 0127A, 0128A
          action = OnBatt = CTS
          action = BattLow = CD
          action = LineDown = 0
         
          cable = 0023A
          action = Onbatt = CD
          action = BattLow = SR
          action = LineDown = 0
         
          cable = 0095A, 0095B, 0095C
          action = OnBatt = RNG
          action = BattLow = CD
          action = LineDown = 0
         
         
         apcserial.c
          condition = killpower
         
          cable = CUSTOM_SIMPLE, 0095A, 0095B, 0095C
          action = ioctl(TIOMCBIS, RTS)      set RTS (kills power)
          action = ioctl(TIOMCBIS, ST)       set TxD
         
          cable = 0020B, 020C, 0119A, 0127A, 0128A
          action = ioctl(TIOMCBIS, DTR)      set DTR (kills power)
