TARGET    := cvgrab
CC        := gcc
CXX 	  := g++
LD        := g++
LDLIBS 	  := -lopencv_core -lopencv_imgproc -lopencv_objdetect -lopencv_highgui -lrt -lpthread -llz4
MODULES   := ./
SRC_DIR   := $(addprefix src/,$(MODULES))
BUILD_DIR := $(addprefix build/,$(MODULES))
SRC       := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
OBJ       := $(patsubst src/%.c,build/%.o,$(SRC))
INCLUDES  := $(addprefix -I,$(SRC_DIR))
CPPFLAGS  := -Wall -ggdb3 -Wno-int-to-pointer-cast
vpath %.c $(SRC_DIR)
define make-goal
$1/%.o: %.c
	$(CXX) $(INCLUDES) $(CPPFLAGS) $(LDLIBS) -c $$< -o $$@
endef
.PHONY: all checkdirs clean
all: checkdirs build/$(TARGET)
build/$(TARGET): $(OBJ)
	$(LD) $(LDLIBS) $^ -o $@
checkdirs: $(BUILD_DIR)
$(BUILD_DIR):
	@mkdir -p $@
clean:
	@$(foreach bdir,$(BUILD_DIR),echo "Cleaning: $(bdir)"; rm -rf $(bdir)*.o;)
$(foreach bdir,$(BUILD_DIR),$(eval $(call make-goal,$(bdir))))
