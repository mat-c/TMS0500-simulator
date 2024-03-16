=ti59 and TMC0501 microcode simulator=

This project allow to run microcode for TMC0501 generation calculator.
See http://www.datamath.org/Chips/TMS0500-CS.htm for more information

This is based of previous work :
http://hsl.wz.cz/ti\_59.htm
https://www.hrastprogrammer.com/emulators.htm

The goal is to able to run microcode of several calculator and understand
how the differents chips work together.

For that each chip is implemented in a separate file and communicate with
bus like shared data.


== hardware details ==
TMC0501 is connected to other chips with :
- clk signals : idle/clk1/clk2
- irg : instruction rom chips to TMC0501
- ext : data communication (bidir)
- iox4 : data communication for alu data (bidir)
- lines to driver led segment display (segA-H + dpt)
- lines to read keyboard

irg/ext/io are open drain active high (pull down when not driven).

one instruction cycle is 16 clk tick : S0 to S15

irg contains 13bits instruction send during S3 to S15
every chip listen to irg to synchronize on the current instruction

For one cycle, irg/iox4 have only one master that is determined by current
instruction

ext is more complex. It is like i2c.
ext[S0]: PREG / TMC0501 out
ext[S1]: COND / set by TMC0501 but can also be set by other chips (card reader)
ext[S2]: HOLD / set by TMC0501 but can also be set by other chips (card reader)
ext[S3..S15]: data from/to TMC0501

== software details ==
struct bus represent the connection between each chips

for efficiency instead of doing transfert bit per bit, it is possible to output/input up to 16bits.
