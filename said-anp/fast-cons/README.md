# AnyNextPacket Experiment
This folder contain to source code used for the demo experiment. The AnyNextPacket forwarding is described in [SAID: A Control Protocol for Scalable and Adaptive Information Dissemination in ICN](http://www.net.informatik.uni-goettingen.de/publications/1972/PDF).

During the demo experiment some configurations were hardcoded in the source-code. For a general implementation of the SAID-based forwarder refer to this repository: [https://github.com/jdebenedetto/said.git](https://github.com/jdebenedetto/said.git)   

This folder contains:
* fast-consumer: consumer limited to a 100 interests window
* slow-consumer: consumer limited to a 50 interests window
* producer
* ANP forwarder

## How to deploy:  

The experiment is supposed to run on a star topology (1 core node and 3 edge nodes):
* On each node an instance of the ANP forwarder must run
* 2 edge nodes acting as consumers (fast and slow)
* 1 edge node act as producer

## Running the experiment


## Expeted outcome


