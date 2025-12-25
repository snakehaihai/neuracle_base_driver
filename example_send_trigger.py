#! /usr/bin/env python  
#  -*- coding:utf-8 -*-
#
# Author: FANG Junying, fangjunying@neuracle.cn
#
# Versions:
# 	v0.1: 2020-02-25, orignal
#
# Copyright (c) 2020 Neuracle, Inc. All Rights Reserved. http://neuracle.cn/

from neuracle_lib.triggerBox import TriggerBox,PackageSensorPara,TriggerIn
import time
# from psychopy import  visual, event,core


if __name__ == '__main__':
    isMac = False
    isTriggerIn = False
    isTriggerBox = True  # can verify response from serial

    if isMac:
        # comID = '/dev/cu.usbserial-DK0C1IG2'  # W4
        comID = '/dev/cu.usbserial-D308GAEC'  # W3
    else:
        comID = 'COM3'

    if isTriggerIn:
        ## example send triggers through TriggerIn
        triggerin = TriggerIn(comID)
        flag = triggerin.validate_device()
        if flag:
            for i in range(1, 10):
                triggerin.output_event_data(i)
                time.sleep(1)
        else:
            raise Exception("Invalid Serial!")
        triggerin.closeSerial()

    if isTriggerBox:
        ## example send triggers by TriggerBox
        triggerbox = TriggerBox(comID)
        for i in range(1, 10):
            print('send trigger: {0}'.format(i))
            triggerbox.output_event_data(i)
            time.sleep(1)
        triggerbox.closeSerial()