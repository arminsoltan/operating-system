#!/bin/bash
echo "Making serial"
(cd serial && make)
echo "Making parallel"
(cd parallel && make)
echo
echo "Running serial"
time (./serial/PhonePricePrediction.out dataset/)
echo
echo "Running parallel"
time (./parallel/PhonePricePrediction.out dataset/)
