# Distributed Air Quality Monitoring System

This repository contains client and server-side software for a distributed air-quality monitoring system using Particle boards and Sensirion SPS30 sensors.

Each sensor module consists of a particle board for data processing and cellular communication and two SPS30 sensors. The reasearch project for which the software was designed aims to use a pair particle sensors, with and without a magnetic filter, to obtain the concentration of ferromagnetic particulate matter in the air.

The client side firmware collects data from the sensors, calculates a running average, compresses, and sends the data as a Particle IOT package. The server side receives the packages from the Particle API, decodes the data, and saves it in the form of dated csv files in a separate folders for each sensor.

The firmware implements a double redundancy mechanism: In addition to sending
the packages to the server, the client saves them on a local SD card. Additionally, a circular buffer of the most recent packages is stored 
in the flash memory of the Particle board. If some packages are lost due to disruptions
in the connection, the server will retrieve the data automatically when
the client is back online.

The command line utility `packet_binary_decoder` can be used to decode raw binary packages (e. g., when reading from the SD card).

This software was designed for the Biosensor Research Group at KIST Europe in Saabrucken, Germany.
