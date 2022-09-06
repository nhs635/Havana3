
clc; clear;

%% Basic parameters

octScans = 1024; octAlines = 1024;
ring_thickness = 160;

[xc_map,yc_map] = circ_init(zeros(octScans,octAlines),2*octScans);

fid = fopen('sepia.colortable','r');
sepia = reshape(fread(fid,'uint8'),[256 3])/255;
fclose(fid);

colorc = ['84c06f'; '649254'; 'd5d52b'; 'ffffff'; 'ff2323'; '860005'; '000000';];
colorc = [ hex2dec(colorc(:,1:2)) hex2dec(colorc(:,3:4)) hex2dec(colorc(:,5:6)) ];

%% Load data

oct_flim = get_oct_flim_raw_data('');

%% Visualization

pp = 200;
th = 256;

% composition en face map
compo_map = oct_flim.compo_map;

% axial cross section
oct_rect = uint8(255*ind2rgb(oct_flim.oct_images(:,:,pp),sepia));

flim_ring = repmat(imresize(permute(compo_map(:,pp,:),[2 1 3]),[1 octAlines]),[ring_thickness 1 1]);
oct_rect(end-ring_thickness+1:end,:,:) = flim_ring;

oct_circ = zeros(2*octScans,2*octScans,3,'uint8');
for c = 1 : 3
    oct_circ(:,:,c) = uint8(interp2(double(oct_rect(:,:,c)),xc_map,yc_map,'linear'));
end

% longitudinal cross section
oct_longi = squeeze(cat(1,oct_flim.oct_images(end:-1:1,th,:),oct_flim.oct_images(:,th + octAlines/2,:)));
oct_longi = uint8(255*ind2rgb(oct_longi,sepia));

compo_map4 = imresize(compo_map,[4*size(compo_map,1) size(compo_map,2)]);
flim_ring1 = repmat(compo_map4(th,:,:),[ring_thickness 1 1]);
flim_ring2 = repmat(compo_map4(th + octAlines/2,:,:),[ring_thickness 1 1]);
oct_longi(1:ring_thickness,:,:) = flim_ring1;
oct_longi(end-ring_thickness+1:end,:,:) = flim_ring2;

% composition proportion at this pullback
inten_weight = mat2gray(oct_flim.intensity_map(:,:,2),[0.0 0.2]);
compo_pb = squeeze(sum(repmat(inten_weight,[1 1 7]).*oct_flim.prob_map(:,:,:),[1 2])) / sum(inten_weight,[1 2]);
compo_pb1 = round(compo_pb*size(oct_longi,2));
compo_pb2 = [0; cumsum(compo_pb1);];
compo_pb_bar = zeros(1,size(oct_longi,2),3,'uint8');
for c = 1 : 7
    compo_pb_bar(1,compo_pb2(c)+1:compo_pb2(c+1),:) = repmat(reshape(colorc(c,:),[1 1 3]),[compo_pb1(c) 1 1]);
end
compo_pb_bar = repmat(compo_pb_bar,[30 1 1]);
compo_pb_bar(1:5,:,:) = 0;

% composition proportion at this frame
inten_weight = mat2gray(oct_flim.intensity_map(:,pp,2),[0.0 0.2]);
compo_fr = squeeze(sum(repmat(inten_weight,[1 1 7]).*oct_flim.prob_map(:,pp,:),[1 2])) / sum(inten_weight,[1 2]);
compo_fr1 = round(compo_fr*2*octScans);
compo_fr2 = [0; cumsum(compo_fr1);];
compo_fr_bar = zeros(1,2*octScans,3,'uint8');
for c = 1 : 7
    compo_fr_bar(1,compo_fr2(c)+1:compo_fr2(c+1),:) = repmat(reshape(colorc(c,:),[1 1 3]),[compo_fr1(c) 1 1]);
end
compo_fr_bar = repmat(compo_fr_bar,[100 1 1]);

% visualization
figure(1); set(gcf,'Position',[80 550 1650 750]);
subplot(2,4,[1,2,5,6]); imagesc([oct_circ; compo_fr_bar;]); axis image; axis off; title(sprintf('%d / %d',pp,size(oct_longi,2)));
subplot(2,4,3:4); imagesc(oct_longi); axis off; hold on; plot([1 1]*pp,[1 octAlines],'c','LineWidth',2); 
plot([1 1]*pp,[1 octAlines]+octAlines,'y','LineWidth',2); hold off;
subplot(2,4,7:8); imagesc([compo_map; compo_pb_bar;]); axis off; hold on; plot([1 1]*pp,[1 octAlines/4],'c','LineWidth',2); 
plot([1 size(oct_longi,2)],[1 1]*(th/4),'c','LineWidth',2); hold off;
