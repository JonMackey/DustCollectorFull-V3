# Dust Collector Full V3

This is the 3rd version of my dust collector full project.  Versions 1 & 2 only monitored the dust bin.  This version adds detection of when the filter is loaded and needs to be cleaned.

The software covers two AVR boards, the controller and the sensor.

The sensor board determines when a blast gate is open and sends message via CAN to the controller when the gate is opened or closed.  For a detailed description of the sensor, see my [Blast Gate Sensor](https://www.instructables.com/Blast-Gate-Sensor/) instructable.

See my 
[Dust Collector Monitor](https://www.instructables.com/Duct-Collector-Monitor/) instructable for more information.

# Till further notice use SDFat lib v1.1.0.  The current software isn't compatible with SDFat lib v2.