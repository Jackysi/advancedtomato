Maintaining Your UPS Batteries
==============================

Battery Technology
------------------

Sealed Lead Acid (SLA) batteries, otherwise known as Valve Regulated Lead Acid
(VRLA) batteries, were originally known as "dry batteries". When first 
introduced in the 1950s, they used a gel electrolyte. The otherwise free acid 
was immobilised with a fine silica powder and formed a gel substance. 

In the 1970s the technology moved to Absorbed Glass Mat (AGM) where the 
separators between the lead plates are made of highly porous micro-fine glass 
fibres which absorb and immobilise the acid and prevent it from spilling. A 
crack or hole in the casing of a VRLA battery using AGM technology will not 
result in a measurable electrolyte spill. Spill containment with VRLA batteries 
is therefore not meaningful or appropriate.

AGM has became the preferred VRLA technology for use in standby or float 
applications and is used in UPSes in the telecommunications, power, and many 
other mission critical industries where the power supply must not be 
interrupted. APC UPSes use VRLA batteries. VRLA batteries are designed to 
recombine hydrogen and oxygen and emit only extremely small amounts of 
hydrogen under normal operating conditions. Normal room ventilation is 
sufficient to remove any hydrogen, so special ventilation is not required.

Battery Life
------------

Most brand name UPS batteries should last 3-5 years. Some APC Back-UPS models 
may have a shorter battery life expectancy. Refer to the user's manual of your 
APC Back-UPS to determine the exact battery life expectancy or contact APC
Technical Support.

Below are some APC guidelines for ensuring optimum battery life expectancy:

1. Make sure that you keep your APC UPS in a cool, dry location with plenty of ventilation. Ideally, the temperature where your UPS is kept should not exceed 75 Deg F (24 Deg C). Also, for ventilation purposes, leave roughly one to two inches on each side for proper airflow.

2. The optimum operating temperature for a lead acid battery is 25 Deg C (77 Deg F). Elevated temperature reduces longevity. As a guideline, every 8 Deg C (15 Deg F) rise in temperature will cut the battery life in half. A battery which would last for 6 years at 25 Deg C (77 Deg F), will only be good for 3 years if operated at 33 Deg C (95 Deg F). Keep in mind that the battery temperature inside your UPS will always be warmer than the ambient temperature of the location where the UPS is installed.

3. Only perform runtime calibrations on your UPS one or two times a year, if necessary. Some of our customers want to check their systems to verify that their runtime is sufficient. However, consistently performing these calibrations can significantly decrease the life expectancy of your battery.   

4. Do not store batteries for extended periods of time. New batteries can be stored for 6 to 12 months from date of purchase. After this period, the battery should be used or it will lose a great deal of its charge. It is not advisable to store batteries that have already been in use.

5. Do not exceed 80 percent of a UPS unit's rated capacity due to the reduction in run time. When you increase your load, your runtime decreases. In the event of a utility power failure, a UPS loaded to full capacity will drain and discharge it's battery quickly and will decrease the life expectancy.

The Smart-UPS detects line voltage distortions such as spikes, notches, dips,
and swells, as well as distortions caused by operation with inexpensive
fuel-powered generators. By default, the UPS reacts to distortions by
transferring to on-battery operation to protect the equipment that you are
plugging into the UPS. Where power quality is poor, the UPS may frequently
transfer to on-battery operation. Battery longevity and service life of the
UPS may be conserved by reducing the sensitivity of the UPS, as long as your
equipment can operate normally under the conditions detailed below. Any type
of voltage disturbance includes; High/Low/No RMS Voltage, Total Harmonic
Distortion(THD), Change in Voltage over Time(dv/dt), Frequency (Hz) out of
tolerance.

**High Sensitivity Mode**
  In the event of any type of voltage disturbance, the UPS will transfer to
  battery power and watch the AC line until it can transfer back to line. The
  transfer time in this mode depends on how far the line voltage deviates from
  the sinewave reference.

**Medium Sensitivity Mode**
  In the event of a RMS voltage-out-of-tolerance(High/Low/No) and
  RMS-rate-of-change disturbances(dv/dt) in the line voltage, the UPS will
  transfer to battery power and watch the AC line until it can transfer back to
  line. In this mode the transfer times are longer but still within acceptable
  limits to insure the continuity of a computer's operation.

**Low Sensitivity Mode**
  In the event of a RMS voltage-out-of-tolerance disturbances(High/Low/No) 
  in the line voltage, the UPS will transfer to battery power and watch the 
  AC line until it can transfer back to line. In this mode the transfer times 
  are longer but still within acceptable limits to insure the continuity of a
  computer's operation.

To change the sensitivity of the UPS, press the small, white "sensitivity" 
button on the rear of the UPS. Use a pointed object (such as a pen) to do so. 
The default setting is "high"; press the button once to set the sensitivity to 
"medium", and press it again to set it to "low"; pressing it a third time will 
set it back to "high". The sensitivity setting change will take effect 
immediately. The green LED next to the button is a sensitivity setting 
indicator - brightly lit is "high" sensitivity, dimly lit is "medium", and 
off is "low" sensitivity.

Flashing Battery Charge Graph LEDs
----------------------------------

The battery charge graph LEDs on the front panel of a Smart-UPS will flash
in unison when the UPS is operating online and the runtime remaining 
(calculated by the Smart-UPS microprocessor) is less than two minutes 
more than the low battery signal warning time (minimum of two minutes).

This would usually indicate that you need to either decrease the load
or install new batteries. If the batteries are new, then you need to perform
a runtime calibration (see below). 

At a pinch, you could also decrease the low battery warning time. There are
four possible settings: 2, 5, 7, or 10 minutes.

Battery Replacement
-------------------

If you own your UPS for long enough, you will inevitably need to replace 
the UPS battery or battery cartridge. An APC battery cartridge comprises 
two batteries physically stuck together with double-sided tape and wired 
in series.

After the decision to replace the batteries, you will face
another decision almost immediately: whether to purchase genuine APC
replacement batteries or not. There are pros and cons to purchasing 
genuine replacement APC batteries.

**APC Battery Pros**

- APC batteries are supported by APC
- APC batteries come with all the necessary hardware
- APC batteries come as pre-made cartridges 
- APC batteries will physically fit your UPS

**APC Battery Cons**

- APC batteries cost up to 4 times the cost of third party batteries

There are also pros and cons to purchasing third party batteries.

**Third Party Battery Pros**

- A third party battery may cost up to 1/4 the price of APC batteries
- A third party battery may have a higher capacity for the same physical size

**Third Party Battery Cons**

- You will need to recycle your battery hardware (cables, connectors etc)
- You will need to create your own battery cartridges (with double-sided tape)
- You will need to ensure the third party battery is the right physical size
- You will need to ensure the third party battery is the right capacity
- Use of a third party battery will void APC's Equipment Protection Policy
- Use of a third party battery may void UL, CSA, VDE, and other safety certifications (according to APC)

If you do decide to use third party replacement batteries, please do not
choose the cheapest available generic SLA batteries. These batteries will, 
almost without exception, not last as long as brand name
batteries and will need replacing within 12-18 months instead of 3-5 years.
Even when using brand name replacement batteries, make sure that you choose
the UPS version (aka "standby") which may cost slightly more, 
but which will last significantly longer in typical UPS usage (long periods
of standby punctuated with infrequent deep discharges).

The brands of battery found in genuine APC battery cartridges have included: 
Panasonic and B&B Battery (aka Best & Best Battery and BB Battery). Yuasa 
(aka Genesis) is also a recommended brand, albeit a bit on the pricey side.

**Note:** When substituting a third party battery with a higher capacity than 
the original, make sure that it still physically fits in the UPS casing. If the 
battery does not fit, do not be tempted to install it "externally". The UPS 
may not be able to charge it in a timely manner and/or it may damage the UPS 
charging circuitry without appropriate modifications which are generally 
beyond an end user's capability.

Battery Installation
--------------------

Although you can do a hot swap of your batteries while the computer and
any other connected equipment is running, it may not be very satisfactory 
because the UPS will not always detect that the batteries have been swapped 
and apcupsd will continue to report "Low Battery". 

There are several ways to correct this situation: 

1. If you have a "smart" UPS model, you can force a self-test to make the 
UPS notice that the battery has been replaced.

2. If after a self-test, the UPS does not detect that the battery has been
replaced, you can use apctest to do a soft battery runtime calibration.
For details of doing this, refer to the "Soft" Runtime Calibration section
below.

3. If after the soft battery runtime recalibration, the UPS does not detect
that the battery has been replaced, you will need to do a manual battery
runtime calibration. For details of doing this, refer to the "Manual" Runtime 
Calibration section below.

"Soft" Runtime Calibration 
--------------------------

A runtime calibration causes the UPS to recalculate its available runtime 
capacity based on its current load.

Caution: a runtime calibration will deeply discharge the UPS batteries, which 
can leave a UPS temporarily unable to support its equipment if a utility power
failure occurs. Frequent calibrations reduce the life of batteries. APC
recommends performing a runtime calibration only annually, semiannually, or 
whenever the load on the UPS is increased.

In order to perform a "soft" runtime calibration it is necessary to wait for 
the UPS to recharge its batteries to 100% capacity. Once this has been done, 
you can then initiate a runtime calibration through apctest.

APC Documentation Notes:

1. In order for the calibration to be accurate, the output load has to be more
than 40% (some APC documentation recommends at least 30%). Also, it 
is advisable not to increase or reduce the load when the UPS is calibrating 
its run time.

2. Under no circumstances should the UPS be turned off during a run time
calibration procedure! Once initiated, the calibration must be allowed to run
until completion.

3. The run time calibration procedure is not necessary nor advisable for a new
UPS. Only old UPSes with batteries that are not subject to discharge for long
periods of time should be allowed to perform a run time calibration.

4. Matrix-UPS and Smart-UPS recalculate the runtime-related parameters every 
time the UPS goes on battery. 

When doing a runtime calibration with "older" batteries, APC Technical Support
recommend doing a complete discharge and recharge first.

If you have "dumb" UPS (aka simple signalling) like a Back-UPS, then your only 
option is to do a manual runtime calibration.

"Manual" Runtime Calibration
----------------------------

Most of the information in this section is taken from APC's website.
Any non-APC additions have been inserted in square brackets.

For a "smart" or "smart signalling" Back-UPS Pro or Smart-UPS:

    Perform a Runtime Calibration. This is a manual procedure and
    should not be confused with the runtime calibration performed
    through PowerChute plus [or apctest]. The batteries inside of the 
    Smart-UPS are controlled by a microprocessor within the UPS. 
    Sometimes it is necessary to reset this microprocessor, especially 
    after the installation of new batteries. Stop the PowerChute plus 
    [or apcupsd] software from running and disconnect the serial cable. 
    There must be at least a 30% load attached to the UPS during this 
    procedure, but the process will cause the UPS to shut off and cut 
    power to its outlets. Therefore, attach a non-critical load to the 
    UPS and then force the UPS on battery by disconnecting it from 
    utility power [suggest not disconnecting, but simply turning off
    utility power thereby preserving earthing].  Allow the unit to 
    run on battery until it turns off completely.  Make sure a 30% load 
    is present! Plug the UPS back into the wall outlet [switch utility 
    power back on] and allow it to recharge (it will recharge more quickly
    turned off and with no load present). Once the unit has recharged,
    the "runtime remaining" calculation should be more accurate.
    Remember that if the unit is an older model, then the runtime will
    not improve significantly.

    Background:

    An APC Smart-UPS has a microprocessor which calculates runtime
    primarily based on the load attached to the UPS and on its battery
    capacity. On the right side of the front display panel there is a
    vertical graph of five LEDs. Each LED is an indication of battery
    charge in increments of twenty percent: 20, 40, 60, 80, 100%
    (bottom to top). For example, if the battery charge is 99%, then
    only four of the five LEDs are illuminated.

    To ensure that an operating system receives a graceful shutdown
    when using PowerChute plus or a SmartSlot accessory, an alert is
    generated by the Smart-UPS indicating that the UPS has reached a
    low battery condition. The alert is audible (rapid beeping), visual
    (flashing battery LED or LEDs), and readable through the graphical
    interface of PowerChute plus software (or a native UPS shutdown
    program within a particular operating system.) In order to
    calculate this "low battery condition," all Smart-UPS products have
    a preconfigured low battery signal warning time of two minutes
    (this is the factory default setting). There are a total of four
    user-changeable settings: 2, 5, 7, or 10 minutes. If the low
    battery signal warning time is set for 2 minutes, then the alerts
    will activate simultaneously two minutes prior to shutdown.
    Similarly, if the total runtime for a particular UPS is 30 minutes
    with a low battery signal warning time set at 10 minutes, then the
    UPS will run on battery for 20 minutes before the low battery alert
    begins.

    Total runtime is primarily based on two factors, battery capacity
    and UPS load. UPS load and runtime on battery are inversely
    proportional: as load increases, battery runtime decreases and vice
    versa. When utility power is lost, the UPS begins discharging the
    battery in order to support the attached load. Once power returns,
    the Smart-UPS will automatically begin to recharge its battery.

For a Matrix UPS:

    It is unnecessary to subject a battery bank to an excessively long 
    calibration. Remove battery packs or increase the load (space heaters 
    are good dummy loads) to obtain a reasonable time length for the 
    calibration (under an hour if possible).

    At the start of a calibration, the Matrix microprocessor saves the 
    Estimated Run Time displayed.

    The unit will then go to battery power until the capacity is 25%. After 
    this run time has been completed, the original Estimated Run Time is compared 
    with the actual run time. It will then increase or decrease this value to 
    correspond to the new run time achieved. If, at any time during the discharge,
    one of the following rules is violated the calibration will be aborted or 
    corrupted:

    1. Battery capacity must be 100% at start of calibration (all packs must indicated as float).
    2. Initial "Estimated Run Time" must not exceed 128 minutes (remove battery packs if necessary).
    3. Load must be above 25%.
    4. Load must not fluctuate more than ± 5%.
    5. The UPS must be allowed to run down to 25% battery capacity. PowerChute [or apcupsd] and Accessories must be removed since they can abort the calibration prematurely.

For a "dumb" or "simple signalling" UPS (eg a Back-UPS):

    This could be done if you have changed your equipment load or battery. 
    Stop the PowerChute [or apcupsd] software from running; disconnect the 
    serial cable between the computer and UPS. Next unplug the UPS from the 
    wall [suggest not disconnecting but simply turning off the utility power 
    thereby preserving the earthing] and let it run on battery until it 
    reaches low battery. Once it reaches low battery plug it back into 
    the wall outlet [turn the utility power back on] and let it recharge. 
    Recharge time can take up to 4 hours.

Resetting the UPS Battery Constant
----------------------------------

In some cases none of the battery runtime calibration methods result in 
the UPS reporting a reasonably correct battery runtime. It has been 
speculated that this is because the battery constant value has drifted 
so far from normal that the microprocessor in the UPS cannot correct it.

The good news is that if you are located in the USA, all you have to do
is contact APC Technical Support and they will send you a serial port
*dongle* which plugs into the serial port of your UPS and reprograms the 
battery constant value for you to the correct value.

The bad news is that for many users outside the USA, this service does not
appear to be available. It is, however, recommended that you first try 
contacting APC Technical Support to verify the correct battery constant 
value. The APC representatives in the Support Forum on the APC website 
are also very helpful in this regard. 

*If all else fails*, the information below is for you.

**WARNING:** Only the values for the Smart-UPS 700 model SU700 and 
Smart-UPS 1400 model SU1400, both with international firmware (and 
therefore international voltage), have been verified. YOU, gentle reader, 
USE THIS INFORMATION AT YOUR OWN RISK in the full knowledge that you 
may render your UPS inoperable and perhaps irreparable, and you will 
have no-one to blame but yourself. *Caveat Utilitor!*

The battery constant is the hex number in the column labelled "0",
presumably for register 0, in the following table::

  UPS Model         4  5  6  0    Hex   Firmware
  SU250               EE F8 B1
  SU400               EE F8 9F    E1
  SU600               EA F4 9F    E5
  SU900               F3 FC 9F    ED
  SU1250              EE FA 9F    F5
  SU2000              F1 F9 9F    FD
  SU450,700        28 F2 FA 96 07,RM=47  52.11.I
  SU450XL,700XL    28 EE F8 9F 700XL=27   51.9.I
  SU1000,INET      35 EF F9 A0    0B     60.11.I
  SU1000XL         34 EE FC 9A    2B      61.9.I
  SU1400           35 EE FC 9A		 70.11.I
  SU1400RM         28 ED FA 89
  SU1400R2IBX135   08 B4 10 A3
  SU1400RMXLI3U    45 F6 F4 80            73.x.I
  SU1400RMXLI3U    20 F3 FD 81            73.x.I
  SU2200I          35 EE FB AF           90.14.I
  SU2200XL,3000    35 EE FB AF 3000=17   90.14.I
  SU3000RMXLI3Ublk 35 F3 F4 AF    77     93.14.I
  SU5000I white    20 F2 FA 91    1F    110.14.I
  SU1400XL,XLI,RM  45 F6 E4 80
  SU420I           25 95 09 85    16      21.7.I
  SU420SI          0E 95 0A 8C
  SU620I           29 99 0B 8A    1A
  BP420SI          0E 95 0A 8C    06      11.2.I
  BP650SI          10 97 0C 91    0A      12.3.I
  Power Stack 250  0C 95 0F B2            26.5.I
  Power Stack 450  0D 96 10 99    36      26.5.I
  SC250RMI1U       0C 95 0F B3    32     735.a.1
  SC420I           0E 95 OA 8C    16     725.1.I
  SC620I           10 97 OB 99    1A     726.x.I
  SC1000I          08 95 10 94    8A     737.x.I
  SC1500I          07 95 14 8F    1E     738.x.I
  SU1000XL         17 EE F9 D5
  MATRIX 3000,5000    E9 F5 B0
  SU700RMI2U       07 B1 0D 92    8A     152.4.I
  SU1000RMI2U      08 B5 0D C7    8E     157.3.I
  SU1400RMI2U      08 B4 10 A3    92     162.3.I
  SUA1000I         07 B5 13 BC    0A    652.12.I
  SUA1000XLI       0B BD 0F 7F    4A    681.13.I
  SUA750XLI        0A B9 0C 86    46     630.3.I
  SUA750I          04 B6 14 82    06    651.12.I
  SUA750RMI2U      07 B1 0D 82    86    619.12.I
  SUA1500I         09 B9 13 A1    0E 601/653.x.I
  SUA1500RMI2U     08 B4 10 A1    8E     617.3.I
  SUA2200I         08 B8 12 B3    26    654.12.I
  SUA2200RMI2U     09 BC 11 81    A6     665.4.I
  SUA2200XLI       0A B7 0F 7F    66     690.x.I
  SUA3000RMI2U     04 B9 0E 70    AA     666.4.I
  SUA3000RMXLI3U   0A B6 0E 89    xx     xxx.x.x
  SUOL1000I        06 B6 1B A6
  SUOL2000XL       0D BD 14 75    52     416.5.I
  SURT1000XLI      0A BB 19 A8    4E     411.x.I
  SURT3000XLI      06 B6 0F CC    56     450.2.I
  SURT5000XLI      05 BA 15 86    5A    451.13.W
  SURT7500XLI      03 BB 20 97    63
  SURT10000XLI     06 B8 19 AB          476.12.W
  SUM1500RMXLI2U   03 B7 0D A5    62     716.3.I
  SUM3000RMXLI2U   03 B7 0D A5    6A     715.3.I
  BP500AVR                        26      17.1.I
  
The instructions for resetting the battery constant are as follows:

1. Shutdown the apcupsd daemon;
2. Run apctest;
3. Choose option 6 to enter terminal mode;
4. Enter Y (UPS should respond SM);
5. Enter 1 (one, not el; wait 4 seconds);
6. Enter 1 (one, not el; UPS should respond PROG);
7. Enter 0 (zero, not oh; UPS should respond with current constant);
8. Write down the existing value so that if something goes wrong, you can at least put it back to that value;
9. Enter + (plus) or - (minus) to increment/decrement the value;
10. Enter R to reprogram constant value (UPS should respond Bye);
11. Enter Y (UPS should respond SM);
12. Enter 0 (zero, not oh; UPS should respond with the new constant);
13. Enter Esc to exit terminal mode;
14. Choose option 7 to exit apctest.
