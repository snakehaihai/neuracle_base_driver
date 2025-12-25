

% triggerbox test
% Other m-files required: none
% Subfunctions: none
% MAT-files required: none
%
% Author: Junying FANG, fangjunying@neuracle.cn
%
% Versions: 
%    v0.1: 2016-08-24, orignal
%  
% Copyright (c) 2016 Neuracle, Inc. All Rights Reserved. http://neuracle.cn/
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

clear all
clc
close all
instrreset
N = 10;
waitTime = 1;
%% Global configuration
% Default parameters
isTriggerLight = false;
isTriggerCOM = true;
isParallelPort = false;
% input parameters
prompt = {'TriggerBox COM:',...
            'TriggerBox Light:',...
            'ParallelPort:'};
dlgTitle = 'Configurate devices';
dlgSize = [1 50];
defaultans = {num2str(isTriggerCOM),...
            num2str(isTriggerLight),...
            num2str(isParallelPort)};
answer = inputdlg(prompt,dlgTitle,dlgSize,defaultans,'on');
isTriggerCOM = logical(str2double(answer{1}));
isTriggerLight = logical(str2double(answer{2}));
isParallelPort = logical(str2double(answer{3}));

%% System configuration
% TriggerBox
if isTriggerLight
    triggerBoxLight = TriggerBox();
    sensorID = 1;
    triggerBoxLight.InitLightSensor(sensorID);
    triggerBoxLight.SetEventData(sensorID, 0, 0);
end
if isTriggerCOM
    triggerBoxCOM = TriggerBox();
end
if isParallelPort
    portPP = hex2dec('DEFC');
    config_io;
    global cogent;
    if( cogent.io.status ~= 0 )
       error('inp/outp installation failed');
    end
    outp(portPP, 0);
end


%% send trigger
for n  = 1: N
    for i = 1:255
        if isTriggerCOM
            triggerBoxCOM.OutputEventData(i);
             fprintf('send trigger %d by com\n',i)
        end
        if isTriggerLight 
            triggerBoxLight.SetEventData(sensorID, i);
            fprintf('send trigger %d by light\n',i)
        end

        if isParallelPort
            outp(portPP,i);
            pause(0.004);
            outp(portPP, 0);
            fprintf('send trigger %d by parallel port\n',i)
        end
        pause(waitTime);
    end
end
