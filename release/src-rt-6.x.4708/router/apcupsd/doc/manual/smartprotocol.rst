APC smart protocol
==================

The APC UPS
protocol was originally analyzed by Pavel Korensky with additions
from Andre H. Hendrick beginning in 1995, and we want to give
credit for good, hard work, where credit is due. After having said
that, you will see that Steven Freed built much of the original
apcupsd information file.

The start of this chapter of the apcupsd manual in HTML format was
pulled from the Network UPS Tools (NUT) site 
(http://www.networkupstools.org/protocols/apcsmart.html). It
has been an invaluable tool in improving apcupsd, and I consider it
the Bible of APC UPS programming. In the course of using it, I
have added information gleaned from apcupsd and information
graciously supplied by APC. 

Description
-----------

Here's the information on the elusive APC smart signaling protocol
used by their higher end units (Back-UPS Pro, Smart-UPS,
Matrix-UPS, etc). What you see here has been collected from a
variety of sources. Some people analyzed the chatter between
PowerChute and their hardware. Others sent various characters to
the UPS and figured out what the results meant.

RS-232 differences
------------------

Normal 9 pin serial connections have TxD on 3 and RxD on 2. APC's
smart serial ports put TxD on pin 1 and RxD on pin 2. This means
you go nowhere if you use a normal straight through serial cable.
In fact, you might even power down the load if you plug one of
those cables in. This is due to the odd routing of pins - DTR and
RTS from the PC usually wind up driving the on/off line. So, when
you open the port, they go high and \*poof\* your computer dies.

The Smart Protocol
------------------

Despite the lack of official information from APC, this table has
been constructed. It's standard RS-232 serial communications at
2400 bps/8N1. Don't rush the UPS while transmitting or it may stop
talking to you. This isn't a problem with the normal single
character queries, but it really does matter for multi-char things
like "@000". Sprinkle a few calls to usleep() in your code and
everything will work a lot better.

The following table describes the single character "Code" or
command that you can send to the UPS, its meaning, and what sort of
response the UPS will provide. Typically, the response shown below
is followed by a newline (\\n in C) and a carriage return (\\r in
C). If you send the UPS a command that it does not recognize or
that is not available on your UPS, it will normally respond with "NA"
for "not available", otherwise the response is given in the
"Typical results" column.

+---------+------------+----------------+--------------------------------------+
|Character|Meaning     |Typical results |Other info                            |
+=========+============+================+======================================+
|^A       |Model string|SMART-UPS 700   |Spotty support for this query on older|
|         |            |                |models                                |
+---------+------------+----------------+--------------------------------------+
|^N       |Turn on UPS |n/a             |Send twice, with 1.5s delay between   |
|         |            |                |chars. Only on 3rd gen SmartUPS and   |
|         |            |                |Black Back-UPS Pros                   |
+---------+------------+----------------+--------------------------------------+
|^Z       |Permitted   |*long string*   |Gives the EEPROM permitted values for |
|         |EEPROM      |                |your model. See `EEPROM Values`_ for  |
|         |Values      |                |details.                              |
+---------+------------+----------------+--------------------------------------+
|A        |Front panel |Light show +    |Also sounds the beeper for 2 seconds  |
|         |test        |"OK"            |                                      |
+---------+------------+----------------+--------------------------------------+
|B        |Battery     |27.87           |Varies based on current level of      |
|         |voltage     |                |charge. See also Nominal Battery      |
|         |            |                |Voltage.                              |
+---------+------------+----------------+--------------------------------------+
|C        |Internal    |036.0           |Units are degrees C                   |
|         |Temperature |                |                                      |
+---------+------------+----------------+--------------------------------------+
|D        |Runtime     |!, then $       |Runs until battery is below 25% (35%  |
|         |calibration |                |for Matrix) Updates the 'j' values.   |
|         |            |                |Only works at 100% battery charge. Can|
|         |            |                |be aborted with a second "D"          |
+---------+------------+----------------+--------------------------------------+
|E        |Automatic   |336             |Writable variable. Possible values:   |
|         |self test   |                |                                      |
|         |interval    |                |- "336" (14 days)                     |
|         |            |                |- "168" (7 days)                      |
|         |            |                |- "ON " (at power on) note extra space|
|         |            |                |- "OFF" (never)                       |
+---------+------------+----------------+--------------------------------------+
|F        |Line        |60.00           |Units are Hz. Value varies based on   |
|         |frequency   |                |locality, usually 50/60.              |
+---------+------------+----------------+--------------------------------------+
|G        |Cause of    |O               |Possible values:                      |
|         |last        |                |                                      |
|         |transfer    |                |- R (unacceptable utility voltage rate|
|         |to battery  |                |  of change)                          |
|         |            |                |- H (high utility voltage)            |
|         |            |                |- L (low utility voltage)             |
|         |            |                |- T (line voltage notch or spike)     |
|         |            |                |- O (no transfers since turnon)       |
|         |            |                |- S (transfer due to U command or     |
|         |            |                |  activation of UPS test from front   |
|         |            |                |  panel)                              |
|         |            |                |- NA (transfer reason still not       |
|         |            |                |  available; read again)              |
+---------+------------+----------------+--------------------------------------+
|I        |Measure-UPS |FF              |*not decoded yet*                     |
|         |Alarm enable|                |                                      |
+---------+------------+----------------+--------------------------------------+
|J        |Measure-UPS |0F,00           |*not decoded yet*                     |
|         |Alarm status|                |                                      |
+---------+------------+----------------+--------------------------------------+
|K        |Shutdown    |OK or *         |Send twice with > 1.5s delay between  |
|         |with grace  |                |chars. Older units send "*" instead of|
|         |period (no  |                |"OK". Length of grace period is set   |
|         |return)     |                |with Grace Period command. UPS will   |
|         |            |                |remain off and NOT power on if utility|
|         |            |                |power is restored.                    |
+---------+------------+----------------+--------------------------------------+
|L        |Input line  |118.3           |Value varies based on locality. Does  |
|         |voltage     |                |not always read 000.0 on line failure.|
+---------+------------+----------------+--------------------------------------+
|M        |Maximum line|118.9           |This is the max voltage since the last|
|         |voltage     |                |time this query was run.              |
+---------+------------+----------------+--------------------------------------+
|N        |Minimum line|118.1           |This is the min voltage since the last|
|         |voltage     |                |time this query was run.              |
+---------+------------+----------------+--------------------------------------+
|O        |Output      |118.3           |Also see on battery output voltage.   |
|         |voltage     |                |                                      |
+---------+------------+----------------+--------------------------------------+
|P        |Power load  |023.5           |Relative to capacity of the UPS.      |
|         |%           |                |                                      |
+---------+------------+----------------+--------------------------------------+
|Q        |Status flags|08              |Bitmapped, see `status bits`_ below   |
+---------+------------+----------------+--------------------------------------+
|R        |Turn dumb   |BYE             |Only on 3rd gen SmartUPS, SmartUPS    | 
|         |            |                |v/s, BackUPS Pro. Must send enter     |
|         |            |                |smart mode command to resume comms.   |
+---------+------------+----------------+--------------------------------------+
|S        |Soft        |OK              |Command executes after grace period.  |
|         |shutdown    |                |UPS goes online when power returns.   |
|         |            |                |Only works when on battery.           |
+---------+------------+----------------+--------------------------------------+
|U        |Simulate    |!, then $       |See `Alert messages`_ section for info|
|         |power       |                |on ! and $.                           |
|         |failure     |                |                                      |
+---------+------------+----------------+--------------------------------------+
|V        |Old firmware|"GWD" or "IWI"  |See `Interpretation of the Old        |
|         |revision    |                |Firmware Revision`_                   |
+---------+------------+----------------+--------------------------------------+
|W        |Self test   |OK              |Tests battery, like pushing button on |
|         |            |                |the front panel. Results stored in "X"|
+---------+------------+----------------+--------------------------------------+
|X        |Self test   |OK              |Possible values:                      |
|         |results     |                |                                      |
|         |            |                |- OK = good battery                   |
|         |            |                |- BT = failed due to insufficient     |
|         |            |                |  capacity                            |
|         |            |                |- NG = failed due to overload         |
|         |            |                |- NO = no results available (no test  |
|         |            |                |  performed in last 5 minutes)        |
+---------+------------+----------------+--------------------------------------+
|Y        |Enter smart |SM              |This must be sent before any other    |
|         |mode        |                |commands will work. See also turn dumb|
|         |            |                |command to exit smart mode.           |
+---------+------------+----------------+--------------------------------------+
|Z        |Shutdown    |n/a             |Send twice with > 1.5s delay between  |
|         |immediately |                |chars. UPS switches load off          |
|         |            |                |immediately (no grace period)         |
+---------+------------+----------------+--------------------------------------+
|a        |Protocol    |*long string*   |Returns three main sections delimited |
|         |info        |                |by periods:                           |
|         |            |                |                                      |
|         |            |                |- Protocol version                    |
|         |            |                |- Alert messages (aka async notifiers)|
|         |            |                |- Valid commands                      |
+---------+------------+----------------+--------------------------------------+
|b        |Firmware    |50.9.D          |See `Interpretation of the New        |
|         |revision    |                |Firmware Revision`_.                  |
|         |            |                |                                      |
|         |            |                |Decoding the example:                 |
|         |            |                |                                      |
|         |            |                |- 50 = SKU (variable length)          | 
|         |            |                |- 9 = firmware revision               |
|         |            |                |- D = country code (D=USA,            |
|         |            |                |  I=International, A=Asia, J=Japan,   |
|         |            |                |  M=Canada)                           |
+---------+------------+----------------+--------------------------------------+
|c        |UPS local   |UPS_IDEN        |Writable variable. Up to 8 letter     |
|         |id          |                |identifier for keeping track of your  |
|         |            |                |hardware.                             |
+---------+------------+----------------+--------------------------------------+
|e        |Return      |00              |Writable variable. Minimum battery    |
|         |threshold   |                |charge % before UPS will return online|
|         |            |                |after a soft shutdown. Possible       |
|         |            |                |values:                               |
|         |            |                |                                      |
|         |            |                |- 00 = 00% (UPS turns on immediately) |
|         |            |                |- 01 = 15%                            |
|         |            |                |- 02 = 25%                            |
|         |            |                |- 03 = 90%                            |
+---------+------------+----------------+--------------------------------------+
|f        |Battery     |099.0           |Percentage of battery charge remaining|
|         |level %     |                |                                      |
+---------+------------+----------------+--------------------------------------+
|g        |Nominal     |024             |The battery voltage that's expected to|
|         |battery     |                |be present in the UPS normally. This  |
|         |voltage     |                |is a constant based on the type,      |
|         |            |                |number, and wiring of batteries in the|
|         |            |                |UPS. Typically "012", "024" or "048". |
+---------+------------+----------------+--------------------------------------+
|h        |Measure-UPS |042.4           |Percentage. Only works on models with |
|         |ambient     |                |Measure-UPS SmartSlot card.           |
|         |humidity (%)|                |                                      |
+---------+------------+----------------+--------------------------------------+
|i        |Measure-UPS |00              |Bitmapped hex variable. Mapping:      |
|         |dry contacts|                |                                      |
|         |            |                |- 10 = contact 1                      |
|         |            |                |- 20 = contact 2                      |
|         |            |                |- 40 = contact 3                      |
|         |            |                |- 80 = contact 4                      |
+---------+------------+----------------+--------------------------------------+
|j        |Estimated   |0327:           |Value is in minutes. Terminated with  |
|         |runtime     |                |a colon.                              |
+---------+------------+----------------+--------------------------------------+
|k        |Alarm delay |0               |Writable variable. Controls behavior  |
|         |            |                |of UPS beeper. Possible values:       |
|         |            |                |                                      |
|         |            |                |- 0 = 5 second delay after power fail |
|         |            |                |- T = 30 second delay                 |
|         |            |                |- L = alarm at low battery only       |
|         |            |                |- N = no alarm                        |
+---------+------------+----------------+--------------------------------------+
|l        |Low transfer|103             |Writable variable. UPS goes on battery|
|         |voltage     |                |when voltage drops below this point.  |
+---------+------------+----------------+--------------------------------------+
|m        |Manufacture |11/29/96        |Format may vary by country (MM/DD/YY  |
|         |date        |                |vs DD/MM/YY). Unique within groups of |
|         |            |                |UPSes (production runs)               |
+---------+------------+----------------+--------------------------------------+
|n        |Serial      |WS9643050926    |Unique for each UPS                   |
|         |number      |                |                                      |
+---------+------------+----------------+--------------------------------------+
|o        |Nominal     |115             |Expected output voltage when running  |
|         |Output      |                |on batteries. May be a writable       |
|         |Voltage     |                |variable on 220/230/240 VAC units.    |
+---------+------------+----------------+--------------------------------------+
|p        |Shutdown    |020             |Seconds. Writable variable. Sets the  |
|         |grace delay |                |delay before soft shutdown completes. |
|         |            |                |(020/180/300/600)                     |
+---------+------------+----------------+--------------------------------------+
|q        |Low battery |02              |Minutes. Writable variable. The UPS   |
|         |warning     |                |will report a low battery condition   |
|         |            |                |this many minutes before it runs out  |
|         |            |                |of power                              |
+---------+------------+----------------+--------------------------------------+
|r        |Wakeup delay|000             |Seconds. Writable variable. The UPS   |
|         |            |                |will wait this many seconds after     |
|         |            |                |reaching the minimum charge before    |
|         |            |                |returning online. (000/060/180/300)   |
+---------+------------+----------------+--------------------------------------+
|s        |Sensitivity |H               |Writable variable. Possible values:   |
|         |            |                |                                      |
|         |            |                |- H = highest                         |
|         |            |                |- M = medium                          |
|         |            |                |- L = lowest                          |
|         |            |                |- A = autoadjust (Matrix only)        |
+---------+------------+----------------+--------------------------------------+
|t        |Measure-UPS |80.5            |Degrees C. Only works on models with  |
|         |ambient     |                |the Measure-UPS SmartSlot card.       |
|         |temperature |                |                                      |
+---------+------------+----------------+--------------------------------------+
|u        |Upper       |132             |Writable variable. UPS goes on battery|
|         |transfer    |                |when voltage rises above this point.  |
|         |voltage     |                |                                      |
+---------+------------+----------------+--------------------------------------+
|v        |Measure-UPS |4Kx             |Firmware information for Measure-UPS  |
|         |firmware    |                |board                                 |
+---------+------------+----------------+--------------------------------------+
|x        |Last battery|11/29/96        |Writable variable. Holds whatever the |
|         |change date |                |user set in it. Eight characters.     |
+---------+------------+----------------+--------------------------------------+
|y        |Copyright   |\(C) APCC       |Only works if firmware letter is      |
|         |notice      |                |later than O                          |
+---------+------------+----------------+--------------------------------------+
|z        |Reset to    |CLEAR           |Resets most variables to initial      |
|         |factory     |                |factory values except identity or     |
|         |settings    |                |battery change date. Not available on |
|         |            |                |SmartUPS v/s or BackUPS Pro.          |
+---------+------------+----------------+--------------------------------------+
|\+       |Capability  |*various*       |Cycle forward through possible        |
|         |cycle       |                |capability values. UPS sends          |
|         |(forward)   |                |afterward to confirm change to EEPROM.|
+---------+------------+----------------+--------------------------------------+
|\-       |Capability  |*various*       |Cycle backward through possible       |
|         |cycle       |                |capability values. UPS sends          |
|         |(backward)  |                |afterward to confirm change to EEPROM.|
+---------+------------+----------------+--------------------------------------+
|@nnn     |Shutdown and|OK or *         |UPS shuts down after grace period with|
|         |return      |                |delayed wakeup after nnn tenths of an |
|         |            |                |hour plus any wakeup delay time. Older|
|         |            |                |models send "*" instead of "OK".      |
+---------+------------+----------------+--------------------------------------+
|0x7f     |Abort       |OK              |Use to abort @, S, K                  |
|         |shutdown    |                |                                      |
+---------+------------+----------------+--------------------------------------+
|~        |Register #1 |*see below*     |See `Register 1`_ table               |
+---------+------------+----------------+--------------------------------------+
|'        |Register #2 |*see below*     |See `Register 2`_ table               |
+---------+------------+----------------+--------------------------------------+
|0        |Battery     |                |See `Resetting the UPS Battery        |
|         |constant    |                |Constant`_                            |
+---------+------------+----------------+--------------------------------------+
|4        |*???*       |                |Prints 35 on SmartUPS 1000            |
+---------+------------+----------------+--------------------------------------+
|5        |*???*       |                |Prints EF on SmartUPS 1000            |
+---------+------------+----------------+--------------------------------------+
|6        |*???*       |                |Prints F9 on SmartUPS 1000            |
+---------+------------+----------------+--------------------------------------+
|7        |DIP switch  |                |See `Dip switch info`_                |
|         |positions   |                |                                      |
+---------+------------+----------------+--------------------------------------+
|8        |Register #3 |*see below*     |See `Register 3`_ table               |
+---------+------------+----------------+--------------------------------------+
|9        |Line quality|FF              |Possible values:                      |
|         |            |                |                                      |
|         |            |                |- 00 = unacceptable                   |
|         |            |                |- FF = acceptable                     |
+---------+------------+----------------+--------------------------------------+
|>        |Number of   |                |SmartCell models return number of     |
|         |external    |                |connected packs. Other models return  |
|         |battery     |                |value set by the user (use +/-).      |
|         |packs       |                |                                      |
+---------+------------+----------------+--------------------------------------+
|[        |Measure-UPS |NO,NO           |Degrees C. Writable Variable. Possible|
|         |Upper temp  |                |values: 55, 50, 45, ..., 05.          |
|         |limit       |                |Use +/- to change values.             |
+---------+------------+----------------+--------------------------------------+
|]        |Measure-UPS |NO,NO           |Degrees C. Writable Variable. Possible|
|         |lower temp  |                |values: 55, 50, 45, ..., 05.          |
|         |limit       |                |Use +/- to change values.             |
+---------+------------+----------------+--------------------------------------+
|{        |Measure-UPS |NO,NO           |Percentage. Writable Variable.        |
|         |Upper       |                |Possible values: 90, 80, 70, ..., 10. |
|         |humidity    |                |Use +/- to change values.             |
|         |limit       |                |                                      |
+---------+------------+----------------+--------------------------------------+
|}        |Measure-UPS |NO,NO           |Percentage. Writable Variable.        |
|         |lower       |                |Possible values: 90, 80, 70, ..., 10. |
|         |humidity    |                |Use +/- to change values.             |
|         |limit       |                |                                      |
+---------+------------+----------------+--------------------------------------+
|**Matrix-UPS and Symmetra Commands**                                          |
+---------+------------+----------------+--------------------------------------+
|^        |Run in      |BYP, INV, ERR   |If online, "BYP" response is received |
|         |bypass mode |                |as bypass mode starts. If already in  |
|         |            |                |bypass, "INV" is received and UPS goes|
|         |            |                |online. If UPS can't transfer, "ERR"  |
|         |            |                |received                              |
+---------+------------+----------------+--------------------------------------+
|<        |Number of   |000             |Count of bad packs connected to the   |
|         |bad battery |                |UPS                                   |
|         |packs       |                |                                      |
+---------+------------+----------------+--------------------------------------+
|/        |Load current|*nn.nn*         |True RMS load current drawn by UPS    |
+---------+------------+----------------+--------------------------------------+
|\\       |Apparent    |*nnn.nn*        |Output load as percentage of full     |
|         |load power  |                |rated load in VA.                     |
+---------+------------+----------------+--------------------------------------+
|^V       |Output      |                |Writable variable. Possible values:   |
|         |voltage     |                |                                      |
|         |selection   |                |- A = automatic (based on input tap)  |
|         |            |                |- M = 208 VAC                         |
|         |            |                |- I = 240 VAC                         |
+---------+------------+----------------+--------------------------------------+
|^L       |Front panel |                |Writable variable. Possible values:   |
|         |language    |                |                                      |
|         |            |                |- E = English                         |
|         |            |                |- F = French                          |
|         |            |                |- G = German                          |
|         |            |                |- S = Spanish                         |
|         |            |                |- 1 = *unknown*                       |
|         |            |                |- 2 = *unknown*                       |
|         |            |                |- 3 = *unknown*                       |
|         |            |                |- 4 = *unknown*                       |
+---------+------------+----------------+--------------------------------------+
|w        |Run time    |                |Writable variable. Minutes of runtime |
|         |conservation|                |to leave in battery (UPS shuts down   |
|         |            |                |"early"). Possible values:            |
|         |            |                |                                      |
|         |            |                |- NO = disabled                       |
|         |            |                |- 02 = leave 2 minutes of runtime     |
|         |            |                |- 05 = leave 5 minutes                |
|         |            |                |- 08 = leave 8 minutes                |
+---------+------------+----------------+--------------------------------------+


Dip switch info
---------------

=== ====== =====================================================================
Bit Switch Option when bit=1
=== ====== =====================================================================
0   4      Low battery alarm changed from 2 to 5 mins. Autostartup disabled on 
           SU370ci and 400
1   3      Audible alarm delayed 30 seconds
2   2      Output transfer set to 115 VAC (from 120 VAC) or to 240 VAC (from 
           230 VAC)
3   1      UPS desensitized - input voltage range expanded
4-7        Unused at this time
=== ====== =====================================================================


Status bits
-----------

This is probably the most important register of the UPS, which
indicates the overall UPS status. Some common things you'll see:

- 08 = On line, battery OK
- 10 = On battery, battery OK
- 50 = On battery, battery low
- SM = Status bit is still not available (retry reading)

=== ============================================================================
Bit Meaning when bit=1
=== ============================================================================
0   Runtime calibration occurring
    (Not reported by Smart UPS v/s and BackUPS Pro)
1   SmartTrim (Not reported by 1st and 2nd generation SmartUPS models)
2   SmartBoost
3   On line (this is the normal condition)
4   On battery
5   Overloaded output
6   Battery low
7   Replace battery
=== ============================================================================


Alert messages
--------------

These single character messages are sent by the UPS any time there
is an Alert condition. All other responses indicated above are sent
by the UPS only in response to a query or action command.

========= ============= ========================================================
Character Meaning       Description
========= ============= ========================================================
!         Line Fail     Sent when the UPS goes on-battery, repeated  every 30
                        seconds until low battery condition reached. Sometimes 
                        occurs more than once in the first 30 seconds.

$         Return from   UPS back on line power. Only sent if a ! has been sent
          line fail     previously.
                                
%         Low battery   Sent to indicate low battery. Not implemented on 
                        SmartUPS v/s or BackUPS Pro models

\+        Return from   Sent when the battery has been recharged to some level
          low batt      Only sent if a % has been sent previously.

?         Abnormal      Sent for conditions such as "shutdown due to overload"
          condition     or "shutdown due to low battery  capacity". Also occurs 
                        within 10 minutes of turnon.

=         Return from   Sent when the UPS returns from an abnormal condition
          abnormal      where ? was sent, but not a turn-on. Not implemented on
          condition     SmartUPS v/s or BackUPS Pro models.

\*        About to      Sent when the UPS is about to switch off the load. No
          turn off      commands are processed after this character is sent. Not
                        implemented on SmartUPS v/s, BackUPS Pro, or 3rd 
                        generation SmartUPS models.

#         Replace       Sent when the UPS detects that the battery needs to be
          battery       replaced. Sent every 5 hours until a new battery test is
                        run or the UPS is shut off. Not implemented on SmartUPS 
                        v/s or BackUPS Pro models.

&         Check alarm   Sent to signal that temp or humidity out of set limits.
          register      Also sent when one of the contact closures changes 
          for fault     state. Sent every 2 minutes until the alarm conditions
          (Measure-UPS) are reset. Only sent for alarms enabled with I. Cause of
                        alarm may be determined with J. Not implemented on 
                        SmartUPS v/s or BackUPS Pro.

\|        Variable      Sent whenever any EEPROM variable is changed. Only
          change in     supported on Matrix UPS and 3rd generation SmartUPS 
          EEPROM        models.
========= ============= ========================================================


Register 1
----------

All bits are valid on the Matrix UPS. SmartUPS models only support
bits 6 and 7. Other models do not respond.

=== ============================================================================
Bit Meaning when bit=1
=== ============================================================================
0   In wakeup mode (typically lasts < 2s)
1   In bypass mode due to internal fault (see `Register 2`_ or `Register 3`_)
2   Going to bypass mode due to command
3   In bypass mode due to command
4   Returning from bypass mode
5   In bypass mode due to manual bypass control
6   Ready to power load on user command
7   Ready to power load on user command or return of line power
=== ============================================================================


Register 2
----------

Matrix UPS models report bits 0-5. SmartUPS models only support
bits 4-6. SmartUPS v/s and BackUPS Pro report bits 4, 6, 7.
Unused bits are set to 0. Other models do not respond.

=== ============================================================================
Bit Meaning when bit=1
=== ============================================================================
0   Fan failure in electronics, UPS in bypass 
1   Fan failure in isolation unit
2   Bypass supply failure
3   Output voltage select failure, UPS in bypass 
4   DC imbalance, UPS in bypass
5   Battery is disconnected
6   Relay fault in SmartTrim or SmartBoost
7   Bad output voltage
=== ============================================================================


Register 3
----------

All bits are valid on the Matrix UPS and 3rd generation SmartUPS
models. SmartUPS v/s and BackUPS Pro models report bits 0-5. All
others report 0-4. State change of bits 1,2,5,6,7 are reported
asynchronously with ? and = messages.

=== ============================================================================
Bit Meaning when bit=1
=== ============================================================================
0   Output unpowered due to shutdown by low battery
1   Unable to transfer to battery due to overload
2   Main relay malfunction - UPS turned off
3   In sleep mode from @ command (maybe others)
4   In shutdown mode from S command
5   Battery charger failure
6   Bypass relay malfunction
7   Normal operating temperature exceeded
=== ============================================================================


Interpretation of the Old Firmware Revision
-------------------------------------------

The Old Firmware Revision is obtained with the "V" command, which
gives a typical response such as "GWD" or "IWI", and can be
interpreted as follows:

::

    Old Firmware revision and model ID String for SmartUPS & MatrixUPS

    This is a three character string XYZ

       where X == Smart-UPS or Matrix-UPS ID Code.
         range 0-9 and A-P
           1 == unknown
           0 == Matrix 3000
           5 == Matrix 5000
         the rest are Smart-UPS and Smart-UPS-XL
           2 == 250       3 == 400       4 == 400
           6 == 600       7 == 900       8 == 1250
           9 == 2000      A == 1400      B == 1000
           C == 650       D == 420       E == 280
           F == 450       G == 700       H == 700XL
           I == 1000      J == 1000XL    K == 1400
           L == 1400XL    M == 2200      N == 2200XL
           O == 3000      P == 5000

       where Y == Possible Level of Smart Features, unknown???
           G == Stand Alone
           T == Stand Alone
                   V == ???
           W == Rack Mount

       where Z == National Model Use Only Codes
           D == Domestic        115 Volts
           I == International   230 Volts
           A == Asia ??         100 Volts
           J == Japan ??        100 Volts


Interpretation of the New Firmware Revision
-------------------------------------------

::

    New Firmware revision and model ID String in NN.M.L is the format

        where NN == UPS ID Code.
            12 == Back-UPS Pro 650
            13 == Back-UPS Pro 1000
            52 == Smart-UPS 700
            60 == SmartUPS 1000
            72 == Smart-UPS 1400

            where NN now Nn has possible meanings.
                N  == Class of UPS
                1n == Back-UPS Pro
                5n == Smart-UPS
                7n == Smart-UPS NET

                 n == Level of intelligence
                N1 == Simple Signal, if detectable WAG(*)
                N2 == Full Set of Smart Signals
                N3 == Micro Subset of Smart Signals

        where M == Possible Level of Smart Features, unknown???
            1 == Stand Alone
            8 == Rack Mount
            9 == Rack Mount

        where L == National Model Use Only Codes
            D == Domestic        115 Volts
            I == International   230 Volts
            A == Asia ??         100 Volts
            J == Japan ??        100 Volts
            M == North America   208 Volts (Servers)

EEPROM Values
-------------

Upon sending a ^Z, your UPS will probably spit back approximately
254 characters something like the following (truncated here for the
example):

::

    #uD43132135138129uM43229234239224uA43110112114108 ....

It looks bizarre and ugly, but is easily parsed. The # is some kind
of marker/ident character. Skip it. The rest fits this form:

-  Command character - use this to select the value

-  Locale - use 'b' to find out what yours is (the last character),
   '4' applies to all

-  Number of choices - '4' means there are 4 possibilities coming
   up

-  Choice length - '3' means they are all 3 chars long

Then it's followed by the choices, and it starts over. 

Matrix-UPS models have ## between each grouping for some reason.

Here is an example broken out to be more readable:

::

    CMD DFO RSP FSZ FVL
    u   D   4   3   127 130 133 136
    u   M   4   3   229 234 239 224
    u   A   4   3   108 110 112 114
    u   I   4   3   253 257 261 265
    l   D   4   3   106 103 100 097
    l   M   4   3   177 172 168 182
    l   A   4   3   092 090 088 086
    l   I   4   3   208 204 200 196
    e   4   4   2   00   15  50  90
    o   D   1   3   115
    o   J   1   3   100
    o   I   1   3   230 240 220 225
    o   M   1   3   208
    s   4   4   1     H   M   L   L
    q   4   4   2    02  05  07  10
    p   4   4   3   020 180 300 600
    k   4   4   1     0   T   L   N
    r   4   4   3   000 060 180 300
    E   4   4   3   336 168  ON OFF

    CMD == UPSlink Command.
        u = upper transfer voltage
        l = lower transfer voltage
        e = return threshold
        o = output voltage
        s = sensitivity
        p = shutdown grace delay
        q = low battery warning
        k = alarm delay
        r = wakeup delay
        E = self test interval

    DFO == (4)-all-countries (D)omestic (I)nternational (A)sia (J)apan
         (M) North America - servers.
    RSP == Total number possible answers returned by a given CMD.
    FSZ == Max. number of field positions to be filled.
    FVL == Values that are returned and legal.
         

Programming the UPS EEPROM
--------------------------

There are at this time a maximum of 12 different values that can be
programmed into the UPS EEPROM. They are:

======= ========================================================================
Command Meaning
======= ========================================================================
c       The UPS Id or name
x       The last date the batteries were replaced
u       The Upper Transfer Voltage
l       The Lower Transfer Voltage
e       The Return Battery Charge Percentage
o       The Output Voltage when on Batteries
s       The Sensitivity to Line Quality
p       The Shutdown Grace Delay
q       The Low Battery Warning Delay
k       The Alarm Delay
r       The Wakeup Delay
E       The Automatic Self Test Interval
======= ========================================================================

The first two cases (Ident and Batt date) are somewhat special in
that you tell the UPS you want to change the value, then you supply
8 characters that are saved in the EEPROM. The last ten item are
programmed by telling the UPS that you want it to cycle to the next
permitted value.

In each case, you indicate to the UPS that you want to change the
EEPROM by first sending the appropriate query command (e.g. "c" for
the UPS ID or "u" for the Upper Transfer voltage. This command is
then immediately followed by the cycle EEPROM command or "-". In
the case of the UPS Id or the battery date, you follow the cycle
command by the eight characters that you want to put in the EEPROM.
In the case of the other ten items, there is nothing more to
enter.

The UPS will respond by "OK" and approximately 5 seconds later by a
vertical bar (\|) to indicate that the EEPROM was changed.
