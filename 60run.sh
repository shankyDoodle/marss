#!/bin/bash

qemu/qemu-system-x86_64 -hda ../full_system_img/parsecROI.qcow2 -simconfig s60simconfig.txt -k en-us
