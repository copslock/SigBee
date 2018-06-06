#!/bin/bash
grep -n $1 $( find . -name "*.[c,h]") 
