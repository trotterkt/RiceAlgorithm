% Purpose:
% This MatLab routine is to display an image representation for a
% raw data file prior to compression or subsequent to decompression.
% It assumes that the matrixes are 1024x1024x6, of uint16 samples,
% where the '1024X1024' are the X/Y matrix size, and the '6' represents
% the number of bands.
%
% Created by: Keir Trotter
% California State University, Fullerton
% MSE, CPSC 597, Graduate Project
% 
% Copyright 2016 Keir Trotter
%
function DisplayRawImage(rawFilePath)

% define the matrix dimensions
row=1024; col=1024; band=6;

% open the raw file
fin=fopen(rawFilePath, 'r');

% read in the entire file
oneDimArray=fread(fin, row*col*band, 'uint16=>uint16');

% define the 3-D matrix
threeDimMatrix=reshape(oneDimArray,row,col,band);

% slice out the individual bands
band1Slice=threeDimMatrix(:,:,1);
band2Slice=threeDimMatrix(:,:,2);
band3Slice=threeDimMatrix(:,:,3);
band4Slice=threeDimMatrix(:,:,4);
band5Slice=threeDimMatrix(:,:,5);
band6Slice=threeDimMatrix(:,:,6);

% shape the bands
band1Matrix=reshape(band1Slice, row, col);
band2Matrix=reshape(band2Slice, row, col);
band3Matrix=reshape(band3Slice, row, col);
band4Matrix=reshape(band4Slice, row, col);
band5Matrix=reshape(band5Slice, row, col);
band6Matrix=reshape(band6Slice, row, col);


% transpose the matrixes
band1Matrix=band1Matrix';
band2Matrix=band2Matrix';
band3Matrix=band3Matrix';
band4Matrix=band4Matrix';
band5Matrix=band5Matrix';
band6Matrix=band6Matrix';

% Reference http://landsat.gsfc.nasa.gov/?page_id=5377 for discussion
% of Landsat 8 bands. Band 1-4 are visable light. Matrixes bands do not
% nessassarily coorespond to the actual Band designation. I picked 3 and
% 6 because it seemed to match closest to the CCSDS supplied image. Of the
% 11 bands, 5 are probabily masked off. Therefore, without the masking, I
% can only guess at whih ones are active.

% Also look at 
% http://www.mathworks.com/matlabcentral/answers/259134-how-can-i-combine-four-bands-into-an-image
% for notes

warning('off', 'Images:initSize:adjustingMag');

figure('Name','Band 6','NumberTitle','off')
band6Handle = imshow(band6Matrix);


%C2 = imfuse(band1Matrix, band2Matrix);
%C3 = imfuse(C2, band3Matrix);

%C3 = imfuse(band5Matrix, band6Matrix);
%C4 = imfuse(C1, C2);
%C5 = imfuse(C3, C4);


% display final image
%finalImage = imshow(C5);
%finalImage = imshow(C1);
%finalImage = imshow(C2);
%finalImage = imshow(C3);
%finalImage = imshow(C1);

%figure('Name','Bands 1 and 2','NumberTitle','off')
%figure(imshow([band1Matrix, band2Matrix]));

%figure('Name','Bands 3 and 4','NumberTitle','off')
%figure(imshow([band3Matrix, band4Matrix]));

%figure('Name','Bands 5 and 6','NumberTitle','off')
%figure(imshow([band5Matrix, band6Matrix]));

%finalImage = imshow(band1Matrix);
%finalImage = imshow(band2Matrix);
%finalImage = imshow(band3Matrix);
%finalImage = imshow(band4Matrix);
%finalImage = imshow(band5Matrix);
%finalImage = imshow(band6Matrix);
