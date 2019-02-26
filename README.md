## Description

Nanotode is a framework for running lightweight simulations of the C. elegans
nematode's nervous system. It employs a compact binary representation of the
neuronal connections of the organism, and is designed for use in
resource-constained environments (e.g.  microcontrollers).

Its spiking neural network utilizes a simplified leaky integrate-and-fire model,
with inter-neuron connection weights derived from data available via the
OpenWorm project [1].

It is my hope that this framework spurs further research (by myself and others)
into how accurately we can render this tiny organism on a small computer---possibly
inspiring a new set of educational tools for young people with an interest in biology or robotics.

## Project Layout

Some notable files in the nanotode repository:
* `CElegansNeuronTables`:
Directory containing a CSV-converted representation of
the OpenWorm project's CElegansNeuronTables.xls spreadsheet, which contains the
layout of the C. elegans nervous system [2]
* `python_parsing`:
Contains utilities for parsing the aforementioned CSV files
into a JSON representation, and parsing that JSON representation into a binary
one suitable for consumption by the nanotode simulation (see file: source/neural_rom.c)
* `source`:
Headers and source files for the nanotode framework
* `test`:
Contains an example main() function that ouputs a file representing
the A and B-type motor neuron activity of the worm ('motor_ab.dat'), before and
after sensory stimulation
* `python_plotting`:
Two scripts for creating plots which visualize the data in
a motor_ab.dat file

<p align="center"><img width=500 src="/images/motor_ab.png"></p> <p
align="center"><b>Figure 1.</b><br><i>A and B-type motor neuron activation in
the connectome simulation.</i>.</p>

## Projects Using the Nanotode Library

* [nematode.farm](https://github.com/nategri/nematode_farm): A simple web-assembly based game
utilizing C. elegans simulations
* [nematoduino](https://github.com/nategri/nematoduino)

## Related Projects

Busbice, T. *Extending the C. Elegans Connectome to Robotics*. URL: https://goo.gl/pxavvY

Busbice, T., Garrett, G., Churchill G. *GoPiGo C. elegans Connectome Code*. Github repository.
URL: https://github.com/Connectome/GoPiGo
*Notes: Progenitor of this project.*

## Known Issues

* Currently any cell that lacks post-synaptic connections is blindly categorized
as a 'muscle' as per the source code's language (this is a purely cosmetic problem)
* Only connections moderated by the GABA neurotransmitter are currently marked as
being 'inhibitory' (i.e. having negative weight)---this could be more subtle

## Open Questions

* Neuron firing threshold and maximum idle time before falling back to zero
'potential' are essentially free parameters. What are the best ones? How do we
define 'best'?
* Is "Number of Connections" (as per CElegansNeuronTables) the best first-order
pass at inter-neuron weights? Would love to hear from an OpenWorm researcher
about this.

## References

[1] http://openworm.org/

[2] https://github.com/openworm/CElegansNeuroML
