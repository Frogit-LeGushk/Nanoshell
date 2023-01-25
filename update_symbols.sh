#!/bin/bash

fileName="map_callbacks.txt"
sharedLibName="map_callbacks.so"

nm "$sharedLibName" | grep " T " | grep -oE "[a-zA-Z0-9]*$" > "$fileName"

