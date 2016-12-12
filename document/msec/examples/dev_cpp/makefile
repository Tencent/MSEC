cgi_main:VOA_cpp_MainLogic.pb.o cgi_main.o
	g++ -o testbin cgi_main.o VOA_cpp_MainLogic.pb.o lib/libsrpcclient.a -ldl

VOA_cpp_MainLogic.pb.o :VOA_cpp_MainLogic.pb.cc
	g++ -c VOA_cpp_MainLogic.pb.cc -I./include -I./include/pb

cgi_main.o:cgi_main.cpp
	g++ -c cgi_main.cpp -I./include -I./include/pb



