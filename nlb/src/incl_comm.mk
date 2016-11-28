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

%.o: %.cpp
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CXX) $(CXXFLAGS) -c -o $@ $< $(INC) $(CRESET) 	 
%.o: %.c
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET) 	 
clean:
	@rm -f $(OBJ) $(TARGET)
