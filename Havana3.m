%%

clc; clear; close all; 

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Clinical FLIm OCT
% 
% v180913 drafted
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% options
full_frame = false; % true : full range frame, false : user-defined range frame

image_view = true; % true : image drawing, false : image non-drawing
circ_view = false; % true : circ view, false : rect view
                   % only valid when image_view == true
                  
rect_write = false; % true : rect image writing, false : rect image non-writing
circ_write = false; % true : circ image writing, false : circ image non-writing
                    % only valid when circ_view == true

% basic parameters
vis_ch = 1; % FLIm emission channel

Nsit = 1; % start frame
term = 1; Nit = Nsit; % end frame
pp_range = Nsit : term : Nit;

%% INI Parameters

% Filename
filelist = dir;
for i = 1 : length(filelist)
    if (length(filelist(i).name) > 5)
        if strcmp(filelist(i).name(end-4:end),'.data')
            dfilenm = filelist(i).name(1:end-5);
        end
    end
end
clear filelist;

% Please check ini file in the folder
fid = fopen(strcat(dfilenm,'.ini'));
config = textscan(fid,'%s');
fclose(fid); 

flimChStartInd = zeros(1,4);
flimDealyOffset = zeros(1,3);   
flimIntensityRange = zeros(3,2);
flimLifetimeRange = zeros(3,2);
octGrayRange = zeros(1,2);
for i = 1 : length(config{:})
        
    if (strfind(config{1}{i},'flimAlines'))
        eq_pos = strfind(config{1}{i},'=');
        flimAlines = str2double(config{1}{i}(eq_pos+1:end));
    end
    if (strfind(config{1}{i},'flimScans'))
        eq_pos = strfind(config{1}{i},'=');
        flimScans = str2double(config{1}{i}(eq_pos+1:end));
    end
    
    if (strfind(config{1}{i},'flimBg'))
        eq_pos = strfind(config{1}{i},'=');
        flimBg = str2double(config{1}{i}(eq_pos+1:end));
    end
    if (strfind(config{1}{i},'flimWidthFactor'))
        eq_pos = strfind(config{1}{i},'=');
        flimWidthFactor = str2double(config{1}{i}(eq_pos+1:end));
    end   
    
    for j = 0 : 3
        ChannelStart = sprintf('flimChStartInd_%d',j);
        if (strfind(config{1}{i},ChannelStart))
            eq_pos = strfind(config{1}{i},'=');
            flimChStartInd(j+1) = str2double(config{1}{i}(eq_pos+1:end));
        end
    end
    for j = 1 : 3
        DelayTimeOffset = sprintf('flimDelayOffset_%d',j);
        if (strfind(config{1}{i},DelayTimeOffset))
            eq_pos = strfind(config{1}{i},'=');
            flimDealyOffset(j) = str2double(config{1}{i}(eq_pos+1:end));
        end
    end
    
    for j = 1 : 3
        if (strfind(config{1}{i},sprintf('flimIntensityRangeMax_Ch%d',j)))
            eq_pos = strfind(config{1}{i},'=');
            flimIntensityRange(j,2) = str2double(config{1}{i}(eq_pos+1:end));
        end
        if (strfind(config{1}{i},sprintf('flimIntensityRangeMin_Ch%d',j)))
            eq_pos = strfind(config{1}{i},'=');
            flimIntensityRange(j,1) = str2double(config{1}{i}(eq_pos+1:end));
        end  
        if (strfind(config{1}{i},sprintf('flimLifetimeRangeMax_Ch%d',j)))
            eq_pos = strfind(config{1}{i},'=');
            flimLifetimeRange(j,2) = str2double(config{1}{i}(eq_pos+1:end));
        end
        if (strfind(config{1}{i},sprintf('flimLifetimeRangeMin_Ch%d',j)))
            eq_pos = strfind(config{1}{i},'=');
            flimLifetimeRange(j,1) = str2double(config{1}{i}(eq_pos+1:end));
        end    
    end
    
    if (strfind(config{1}{i},'flimLifetimeColorTable'))
        eq_pos = strfind(config{1}{i},'=');
        flimLifetimeColorTable = str2double(config{1}{i}(eq_pos+1:end));
    end    
     
    if (strfind(config{1}{i},'octAlines'))
        eq_pos = strfind(config{1}{i},'=');
        octAlines = str2double(config{1}{i}(eq_pos+1:end));
    end
    if (strfind(config{1}{i},'octScans'))
        eq_pos = strfind(config{1}{i},'=');
        octScans = str2double(config{1}{i}(eq_pos+1:end));
    end
    
    if (strfind(config{1}{i},'octGrayRangeMax'))
        eq_pos = strfind(config{1}{i},'=');
        octGrayRange(2) = str2double(config{1}{i}(eq_pos+1:end));
    end
    if (strfind(config{1}{i},'octGrayRangeMin'))
        eq_pos = strfind(config{1}{i},'=');
        octGrayRange(1) = str2double(config{1}{i}(eq_pos+1:end));
    end   
    
    if (strfind(config{1}{i},'octColorTable'))
        eq_pos = strfind(config{1}{i},'=');
        octColorTable = str2double(config{1}{i}(eq_pos+1:end));
    end   
end

% Size parameters
flimFrameSize = flimScans*flimAlines;
octFrameSize = octScans*octAlines;

samp_intv = 1000/400;
flimChStartInd0 = flimChStartInd - flimChStartInd(1) + 1;
flimChStartInd0 = [flimChStartInd0 flimChStartInd0(end) + 30];

if (rect_write), mkdir('rect_image_matlab'); end
if (circ_write), mkdir('circ_image_matlab'); end

if (circ_view)
    % Generate circularize map
    [x,y] = meshgrid(linspace(1,octScans,octScans),linspace(1,octScans,octScans));
    x = x - octScans/2;
    y = y - octScans/2;
    
    % X map: interpolate (-pi,pi) -> (1,n_alines)
    theta = atan2(y,x);
    x_map = (theta + pi) * (octAlines - 1) / (2 * pi) + 1;
    
    % Y map: interpolate (0,radius) -> (1,n2_scans)
    rho = sqrt(x.^2 + y.^2);
    y_map = rho * (octScans / 2 - 1) / (octScans / 2) + 1;
end

clear config ChannelStart DelayTimeOffset eq_pos;

%% parameters for FLIm operation

% FLIm mask
fid_mask = fopen([dfilenm,'.flim_mask'], 'r');
FLIm_mask = fread(fid_mask,'float32');
fclose(fid_mask);

start_ind = find(diff(~FLIm_mask) == 1) + 1;
end_ind   = find(diff(~FLIm_mask) == -1);

is_mask   = sum(FLIm_mask-1) ~= 0;

% saturation indicator
max_val_flim = 65532 - flimBg;

% range data
roi_range = flimChStartInd(1) : flimChStartInd(4) + 30  - 1;
roi_range = roi_range + 1;
scale_factor = 20;
actual_factor = (length(roi_range)*scale_factor-1)/(roi_range(end)-roi_range(1));

roi_range1 = linspace(roi_range(1),roi_range(end),length(roi_range)*scale_factor);    

[xxold,yyold] = meshgrid(1:flimAlines,roi_range);
[xxnew,yynew] = meshgrid(1:flimAlines,roi_range1); 

flimChStartInd1 = round(flimChStartInd * actual_factor);

hg = fspecial('Gaussian',[scale_factor*10 1],scale_factor*2.4);

min_interval = min([diff(flimChStartInd1) round(29*actual_factor)]);
range_pulse = 1 : min_interval;

%% FLIm OCT data

fname11 = sprintf([dfilenm,'.data']);
fid_dx = fopen(fname11,'r','l');
fseek(fid_dx,0,'eof');
n_frame = ftell(fid_dx)/(2*flimFrameSize + octFrameSize);

if (full_frame)
    pp_range = 1 : n_frame;
end

intensity_map = zeros(4,flimAlines,n_frame);
meandelay_map = zeros(4,flimAlines,n_frame);
lifetime_map = zeros(3,flimAlines,n_frame);

for pp = pp_range
    clc; pp 
  
    % load data
    fid_dx = fopen(fname11,'r','l');
    fseek(fid_dx, (pp-1)*(2*flimFrameSize + octFrameSize), 'bof');
            
    % Process FLIm data %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%  
    % Deinterleave data
    f_pulse = fread(fid_dx,flimFrameSize,'uint16=>single');
    f_flim = double(reshape(f_pulse,flimScans,flimAlines));
    f_flim1 = f_flim(roi_range,:) - flimBg;

    % Saturation detection
    is_saturation = zeros(4,flimAlines);
    for ch = 1 : 4
        for ii = 1 : flimAlines
            is_saturation(ch,ii) = ...
                length(find(f_flim1(flimChStartInd0(ch):flimChStartInd0(ch+1)-1,ii) == max_val_flim));
        end
    end

    % Masking for removal of RJ artifact (if necessary)
    if (is_mask)
        for ii = 1 : flimAlines
            for i = 1 : 4
                win_range = start_ind(i):end_ind(i);
                if (i == 1)
                    f_flim1(win_range,ii) = 0;
                else
                    [~,max_ind] = max(f_flim1(win_range,ii));
                    max_ind = max_ind + win_range(1) - 1;
                    win_range1 = max_ind - 4:max_ind + 3;
                    f_flim1(win_range1,ii) = 0;
                end
            end  
        end
    end
        
    % Spline interpolation   
    f_flim_ext = interp2(xxold,yyold,f_flim1,xxnew,yynew,'spline');   

    % Software broadening
    f_flim_ext1 = zeros(size(f_flim_ext));
    for ii = 1 : flimAlines
        f_flim_ext1(:,ii) = filter(hg,1,f_flim_ext(:,ii));
    end
    
    % Fluorescence extraction    
    flu_intensity  = zeros(4, flimAlines);
    flu_mean_delay = zeros(4, flimAlines);
    flu_lifetime   = zeros(3, flimAlines);    

    for ii = 1 : flimAlines   

        data0 = f_flim_ext (:,ii);
        data1 = f_flim_ext1(:,ii);   

        % Find mean delay
        for jj = 1 : 4
            offset = (flimChStartInd1(jj) - flimChStartInd1(1));
            if (offset + min_interval > size(f_flim_ext,1))
                offset = size(f_flim_ext,1) - min_interval;
            end
            range_pulse1 = range_pulse + offset;
            data2 = data0(range_pulse1);

            % Find intensity
            flu_intensity(jj,ii) = sum(data2);

            if (sum(data2) > 0.001)
                % Find width of the pulse (adjustable window method)
                r = 0; l = 0;
                data2 = data1(range_pulse1);
                [peakval,peakind] = max(data2);            
                for k = peakind : min_interval
                    if (data2(k) < peakval * 0.5), r = k; break; end
                end
                for k = peakind : -1 : 1
                    if (data2(k) < peakval * 0.5), l = k; break; end
                end

                irf_width = flimWidthFactor * (r - l + 1);
                left = floor((irf_width - 1)/2);    
                md_new = peakind;

                % Find mean delay
                for iter = 1 : 5 
                    start = round(md_new) - left;
                    range_pulse2 = start : start + irf_width - 1;            
                    range_pulse2 = range_pulse2(range_pulse2<=length(data2)&(range_pulse2>0));
                    md_new = (range_pulse2) * data2(range_pulse2) / sum(data2(range_pulse2));
                    if isempty(md_new), md_new = 1; break; end  
                    if isnan(md_new), md_new = 1; break; end 
                end

                md_new1 = md_new + offset + flimChStartInd(1) * actual_factor;
                flu_mean_delay(jj,ii) = (md_new1 - 1) * samp_intv/actual_factor;

                % Find lifetime
                if (jj ~= 1)  
                    flu_lifetime(jj-1,ii) = (flu_mean_delay(jj,ii) - flu_mean_delay(1,ii)) - flimDealyOffset(jj-1);
                end      
            else
                if (jj ~= 1)  
                    flu_lifetime(jj-1,ii) = 0;
                end   
            end
        end

        % Find normalized intensity
        flu_intensity(:,ii) = flu_intensity(:,ii)./flu_intensity(1,ii);   
    end   

    intensity_map(:,:,pp) = flu_intensity;
    lifetime_map(:,:,pp) = flu_lifetime;
    meandelay_map(:,:,pp) = flu_mean_delay; 
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%    
    
    % Process OCT data %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    % Deinterleave data
    f_oct = fread(fid_dx,octFrameSize,'uint8');
    
    % Scaling 8 bit image
    xyim1 = 255 * mat2gray(reshape(f_oct,[octAlines octScans]),octGrayRange);
    xyim2 = medfilt2(xyim1,[3 3],'symmetric');
    
    if (rect_write)
        bmpname = sprintf(['rect_1',dfilenm,'_%03d.bmp'],pp);
        imwrite(uint8(xyim2), strcat('./rect_image_matlab/',bmpname), 'bmp');    
    end
    
    if (circ_view) 
        circ2 = interp2(xyim1(1:octScans/2,:), x_map, y_map, 'linear', 0);
        circ2 = medfilt2(circ2,[3 3]);

        if (circ_write)
            bmpname = sprintf(['circ_1',dfilenm,'_%03d.bmp'],pp);
            imwrite(uint8(xyim2), strcat('./circ_image_matlab/',bmpname), 'bmp');    
        end
    end
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
       
    if (image_view)        
        intensity1 = medfilt1(flu_intensity(vis_ch+1,:),1);
        lifetime1  = medfilt1(flu_lifetime (vis_ch,:),  1);  
 
        figure(1); %set(gcf,'Position',[420 180 516 778]); 
        if (~circ_view)        
            imshow(uint8(xyim2)); colormap gray;           
            freezeColors; hold on;
            h1 = imshow(imresize(repmat(lifetime1,[50 1]),[50 octAlines])); 
            caxis([flimLifetimeRange(vis_ch,1) flimLifetimeRange(vis_ch,2)]); colormap hsv; freezeColors;
            h1.YData = [octScans-80 octScans];
            h2 = imshow(imresize(repmat(intensity1,[50 1]),[50 octAlines])); 
            caxis([flimIntensityRange(vis_ch,1) flimIntensityRange(vis_ch,2)]); colormap hot; freezeColors; hold off;
            h2.YData = [octScans-160 octScans-80];
       
            figure(2); %set(gcf,'Position',[952 538 560 420]);
            subplot(211); plot(intensity1); hold on;
            plot(find(is_saturation(vis_ch+1,:)~=0),intensity1(is_saturation(vis_ch+1,:)~=0),'ro'); 
            plot(find(is_saturation(1,:)~=0),       intensity1(is_saturation(1,:)~=0),       'ko'); hold off;
            axis([0 flimAlines 0 2]); ylabel('normalized intensity');
            title(sprintf('normalized intensity (hot): %.4f +- %.4f AU',mean(intensity1), std(intensity1))); 

            subplot(212); plot(lifetime1); hold on;
            plot(find(is_saturation(vis_ch+1,:)~=0),lifetime1(is_saturation(vis_ch+1,:)~=0),'ro'); 
            plot(find(is_saturation(1,:)~=0),lifetime1(is_saturation(1,:)~=0),'ko'); hold off;
            axis([0 flimAlines 0 10]); ylabel('lifetime'); 
            title(sprintf('lifetime (jet): %.4f +- %.4f nsec', mean(lifetime1), std(lifetime1)));  
        end 
    end           
    
    fclose('all');    
end

fclose('all');

%%

% if (full_frame)
%     ch = 2;
% 
%     i_map = reshape(intensity_map(ch+1,:,:),[flimAlines n_frame]); 
%     t_map = reshape(lifetime_map(ch,:,:),[flimAlines n_frame]); 
% 
%     i_map = medfilt2(i_map,[5 3],'symmetric');
%     t_map = t_map .* (i_map > 0.001);
%     t_map = medfilt2(t_map,[5 3],'symmetric');
% 
%     figure(1);
%     subplot(121); imagesc(circshift(i_map,-round(galvoshift/4))); 
%     caxis([0 0.5]); colormap hot; freezeColors;
%     subplot(122); imagesc(circshift(t_map,-round(galvoshift/4))); 
%     caxis([0 5]); colormap jet; freezeColors;
%     
%     if (write)        
%         mkdir('FLIMres');
%         
%         itn_name = 'FLIMres/intensity_ch%d.flimres';
%         lft_name = 'FLIMres/lifetime_ch%d.flimres';
%         
%         for i = 1 : 3
%             fid_itn = fopen(sprintf(itn_name,i),'wb');
%             fid_lft = fopen(sprintf(lft_name,i),'wb');
%             
%             i_map = intensity_map(i+1,:,:);
%             t_map = lifetime_map(i,:,:);
%             
%             fwrite(fid_itn,i_map(:),'float');
%             fwrite(fid_lft,t_map(:),'float');
%             
%             fclose(fid_itn);
%             fclose(fid_lft);
%         end
%     end
%     
% end
