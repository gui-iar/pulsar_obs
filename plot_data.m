clc
clear
close all
bins=4096;
file=uigetfile;
aux=strsplit(file,"_");
bins=str2num((aux{1,3}))
freq=str2double((aux{1,5}))
rate=str2double((aux{1,4}))
test2=read_float_binary(file);
fft_aux=reshape(test2,bins,size(test2,1)/bins);
fft_total=mean(fft_aux(:,:)');
tot_min=size(fft_aux,2)*2/60
figure()
plot(fft_total);
grid on
title(['Spectral Data',aux,'total minutes ',tot_min])
xlabel('FFT-Bin')
ylabel('Counts')
xlim([0,bins])
power_aux=mean(fft_aux(:,:));
power_total=mean(power_aux)
file_sp=strcat(file(1:end-8),"_spec.jpeg");
saveas(1,file_sp);
figure()
plot(power_aux);
grid on
title(['Power Data',aux,'total minutes ',tot_min])
xlabel('Time Sample,default sample every 2 sec')
ylabel('Counts')
file_tp=strcat(file(1:end-8),"_power.jpeg");
saveas(1,file_tp);
file_mat=strcat(file(1:end-8),"_data.mat")
save(file_mat,"-binary")
b = waitforbuttonpress ()
