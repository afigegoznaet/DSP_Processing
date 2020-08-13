TARGET = DSP_Processing
TEMPLATE = subdirs

# Ensure that library is built before application
CONFIG  += ordered

SUBDIRS += fftreal
SUBDIRS += app

fftreal.subdir = 3rdparty/fftreal

app.depends = fftreal 

