#!/usr/bin/env python

import time
import RPi.GPIO as GPIO


class GPIORelay:
	"""docstring for GPIORelay"""
	def __init__(self, pin):
		self.pin = pin
		GPIO.setmode(GPIO.BCM)
		GPIO.setup(pin, GPIO.OUT)
		GPIO.output(pin, GPIO.LOW)
	def __del__(self):
		GPIO.cleanup()
	def openDoor(self):
		GPIO.output(self.pin, GPIO.HIGH)
		time.sleep(0.5)
		GPIO.output(self.pin, GPIO.LOW)
