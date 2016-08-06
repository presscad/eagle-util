# install.packages('fields')

grid <- read.table("C:/VmTemp/grid.csv", header=F, sep=",")
names(grid) <- c("lng", "lat", "h")

xy <- data.frame(grid$lng, grid$lat)
XY <- data.matrix(xy)
H <- grid$h

library(fields)
fit<-Tps(XY,H)

#tells you all the stuff about the fitted thin-plate spline, as below
summary(fit)

#this assigns the name ¡°sp.out¡± (choose your own name) to the predicted surface from the fitted tp spline.
sp.out<-predict.surface(fit)
#if want the same size
# sp.out<-predict.surface(fit, nx=nrow(xy), ny=nrow(xy))

#gives a shaded 2-d plot of the spline surface using heat colours
# image(sp.out)
#gives a 3-d perspective surface and a contour map - not v satisfactory output
# surface(sp.out)

#gives a 3-d perspective surface
# persp(sp.out, axes=TRUE, xlab="LNG", ylab="LAT", zlab="OD COUNT")

#theta describes the rotation of the surface in degrees from which to view the perspective
# persp(sp.out, theta=270)

#Plot a - perspective
# persp(sp.out, theta=210, phi=25, d=5, shade=1, axes=TRUE, ticktype="detailed", xlab="LNG", ylab="LAT", zlab="OD COUNT")
persp(sp.out, theta=-40, phi=25, d=5, shade=1, axes=TRUE, ticktype="detailed", xlab="LNG", ylab="LAT", zlab="OD COUNT")

#MAKES THE FILLED CONTOURS
image(sp.out, xlab="LNG", ylab="LAT", lwd=2, col=heat.colors(36)) 
#overlays the contour lines
contour(sp.out, add = TRUE, drawlabels = TRUE, lwd=2)
