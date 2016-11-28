RED = \\e[1m\\e[31m
DARKRED = \\e[31m 
GREEN = \\e[1m\\e[32m
DARKGREEN = \\e[32m 
BLUE = \\e[1m\\e[34m
DARKBLUE = \\e[34m 
YELLOW = \\e[1m\\e[33m
DARKYELLOW = \\e[33m 
MAGENTA = \\e[1m\\e[35m
DARKMAGENTA = \\e[35m 
CYAN = \\e[1m\\e[36m
DARKCYAN = \\e[36m 
RESET = \\e[m
CRESET =  ;echo -ne \\e[m; test -s $@

$(TARGET):$(OBJ)
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED) 
	@$(CXX) -o $@ $^ $(CFLAGS) $(LIB) $(CRESET)
	install $(TARGET) ../../bin/
%.o: %.cpp
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CXX) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET) 	 
%.o: %.c
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET) 	 
%.pb.h:%.proto
	@echo -e Generating $(BLUE)$@$(RESET)...
	@protoc  $<  --cpp_out=$(shell dirname $<)  --proto_path=$(shell dirname $<) \
	--proto_path=$(SYS_PB_PATH) --proto_path=$(FRAME_INC_PATH)
%.pb.o:%.proto
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(C_ARGS) -c -o $@ ` echo $< | sed 's/.proto$$/.pb.cc/g' ` $(INCCOMM) $(CRESET)
clean:
	@rm -f $(OBJ) $(TARGET)
