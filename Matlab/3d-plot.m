
% Read from (x,y,z) points from CSV
[x,y,z]=textread('Z:\Host_C\VmTemp\grid.csv','%f,%f,%f');
foo = fit([x,y], z, 'cubicinterp');
% 3D plot
figure;
plot(foo);
% Contour
figure;
plot(foo, 'Style', 'Contour');

[x,y,z]=textread('Z:\Host_C\VmTemp\grid-tps.csv','%f,%f,%f');
foo2 = fit([x,y], z, 'cubicinterp');
figure;
plot(foo2);
figure;
plot(foo2, 'Style', 'Contour');

% 2d scatter plot of z in different color
scatter(x,y,[],z)  % x,y,size,color -> size can also be a vector
