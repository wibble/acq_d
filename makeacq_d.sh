#!/bin/bash

make
sudo chown root:wheel acq_d
sudo chmod ugo=rx,u+s acq_d
