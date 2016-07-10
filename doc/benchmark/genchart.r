# Create Line Chart

#write.table(Bench, "Bench.csv")

DATA_FILE <- Sys.getenv("NDRX_BENCH_FILE")
XLAB <- Sys.getenv("NDRX_BENCH_X_LABEL")
YLAB <- Sys.getenv("NDRX_BENCH_Y_LABEL")
TITLE <- Sys.getenv("NDRX_BENCH_TITLE")
OUTFILE <- Sys.getenv("NDRX_BENCH_OUTFILE")

Bench = mydata = read.table(DATA_FILE, header = TRUE)

# convert factor to numeric for convenience 
Bench$ConfigurationNum <- as.numeric(Bench$Configuration) 
nConfigurations <- max(Bench$ConfigurationNum)

# get the range for the x and y axis 
xrange <- range(Bench$MsgSize) 
yrange <- range(Bench$CallsPerSec) 

#write.csv(Bench$Configuration, file = "Configuration.csv")
#write.csv(nConfigurations, file = "nConfigurations.csv")

png(filename=OUTFILE, width = 640, height = 640, units = "px", res=100)

# set up the plot 
#plot(xrange, yrange, type="n", xlab="MsgSize (bytes)",
#        ylab="CallsPerSec (calls/sec)" ) 

plot(xrange, yrange, type="n", xlab=XLAB,
        ylab=YLAB ) 

grid()

colors <- rainbow(nConfigurations) 
linetype <- c(1:nConfigurations) 
plotchar <- seq(18,18+nConfigurations,1)

# add lines 
for (i in 1:nConfigurations) { 
  Configuration <- subset(Bench, ConfigurationNum==i) 
  lines(Configuration$MsgSize, Configuration$CallsPerSec, type="b", lwd=1.5,
    lty=linetype[i], col=colors[i], pch=plotchar[i]) 
} 

# add a title and subtitle 
#title("Example graph of benchmarking", "Enduro/X benchmark")
title(TITLE, "Enduro/X Benchmark")

ux <- unique(Bench$Configuration)

# add a legend 
#legend(xrange[1], yrange[2], legend=c(1:nConfigurations, cex=0.8, col=colors,
#par(xpd=TRUE)
#legend(xrange[1], yrange[2], legend=ux, cex=0.8, col=colors,
legend("topright", legend=ux, cex=0.8, col=colors,
        pch=plotchar, lty=linetype, title="Configuration")
