#!/bin/bash
while true ;do
	echo ON
	python noise_on.py
	sleep 5
	echo OFF
	python noise_off.py
	sleep 5
done
