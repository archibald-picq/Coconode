# Coconode
Arduino code to act as slave in i2c home network

# Installation
Checkout, open main file Coconode.ino in Arduino editor, edit modules manualy around line 200 (dynamic configuration coming soon).

Upload to your board with Ctrl-U

It should launch and display something like that in the serial console :
```
+ Coco node: edf, i2c id: 7
Compiled: Apr  2 2017, 22:48:10, 4.9.2
+-----------------+
+ init modules
call setup:  --> setup power meter on pin 14 calibrated for 100 (20A)
call setup:  --> setup power meter on pin 15 calibrated for 100 (20A)
call setup:  --> setup power meter on pin 16 calibrated for 100 (20A)
call setup:  --> setup power meter on pin 17 calibrated for 66 (30A)
+-----------------+
```

# Running
The firmware has no special feature indicating it is running. You must connect it through i2c using [Cocoproxy](https://github.com/archibald-picq/Cocoproxy).


