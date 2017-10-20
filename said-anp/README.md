# AnyNextPacket Experiment
This folder contain to source code used for the demo experiment. The AnyNextPacket forwarding is described in the paper PAPER.  

During the demo experiment some configurations were hardcoded in the source-code. For a general implementation of the SAID-based forwarder refer to this repository: [https://github.com/jdebenedetto/said.git](https://github.com/jdebenedetto/said.git)   

This folder contains:
* fast-consumer: consumer limited to a 100 interests window
* slow-consumer: consumer limited to a 50 interests window
* producer
* ANP forwarder

## How to deploy:  

The experiment is supposed to run on a star topology (1 core node and 3 edge nodes):
* On each node must run an instance of the [ANP forwarder](https://github.com/jdebenedetto/icn17_demo/tree/master/said-anp/forwarder)
* 2 edge nodes acting as consumers ([fast](https://github.com/jdebenedetto/icn17_demo/tree/master/said-anp/fast-cons) and [slow](https://github.com/jdebenedetto/icn17_demo/tree/master/said-anp/slow-cons))
* 1 edge node act as [producer](https://github.com/jdebenedetto/icn17_demo/tree/master/said-anp/prod)

All the modules have dependencies on a modified version of the [ccnx-libraries](https://github.com/jdebenedetto/icn17_demo/tree/master/said-anp/ccnx-libraries).  

For other depencies please follow the instruction of the [CICN project](https://wiki.fd.io/view/Cicn).  



