function [data,truth] = VisualISAMGenerateData(options)
% VisualISAMGenerateData: create data for viusalSLAM::iSAM examples
% Authors: Duy Nguyen Ta and Frank Dellaert

%% Generate simulated data
if options.triangle % Create a triangle target, just 3 points on a plane
    nrPoints = 3;
    r = 10;
    for j=1:nrPoints
        theta = (j-1)*2*pi/nrPoints;
        truth.points{j} = gtsamPoint3([r*cos(theta), r*sin(theta), 0]');
    end
else % 3D landmarks as vertices of a cube
    nrPoints = 8;
    truth.points = {gtsamPoint3([10 10 10]'),...
        gtsamPoint3([-10 10 10]'),...
        gtsamPoint3([-10 -10 10]'),...
        gtsamPoint3([10 -10 10]'),...
        gtsamPoint3([10 10 -10]'),...
        gtsamPoint3([-10 10 -10]'),...
        gtsamPoint3([-10 -10 -10]'),...
        gtsamPoint3([10 -10 -10]')};
end

%% Create camera cameras on a circle around the triangle
height = 10; r = 40;
truth.K = gtsamCal3_S2(500,500,0,640/2,480/2);
data.K = truth.K;
for i=1:options.nrCameras
    theta = (i-1)*2*pi/options.nrCameras;
    t = gtsamPoint3([r*cos(theta), r*sin(theta), height]');
    truth.cameras{i} = gtsamSimpleCamera_lookat(t, gtsamPoint3, gtsamPoint3([0,0,1]'), truth.K);
    % Create measurements
    for j=1:nrPoints
        data.z{i,j} = truth.cameras{i}.project(truth.points{j});
    end    
end

%% show images if asked
if options.showImages
    gui = gcf;
    for i=1:options.nrCameras
        figure(2+i);clf;hold on
        set(2+i,'NumberTitle','off','Name',sprintf('Camera %d',i));
        for j=1:nrPoints
            zij = truth.cameras{i}.project(truth.points{j});
            plot(zij.x,zij.y,'*');
            axis([1 640 1 480]);
        end
    end
    figure(gui);
end

%% Calculate odometry between cameras
odometry = truth.cameras{1}.pose.between(truth.cameras{2}.pose);
for i=1:options.nrCameras-1
    data.odometry{i}=odometry;
end