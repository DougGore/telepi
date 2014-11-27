all:
	gcc telepi.c encode.c -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi -I /opt/vc/include/ -I /opt/vc/include/interface/vcos/pthreads/ -I /opt/vc/include/interface/vmcs_host/linux/ -I /opt/vc/src/hello_pi/libs/ilclient/ -L /opt/vc/lib -L /opt/vc/src/hello_pi/libs/ilclient -lpthread -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lilclient -o telepi
