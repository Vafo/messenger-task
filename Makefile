TARGET := messenger_app
TARGET_TEST := messenger_test

SRC_FOLDER := src
INCLUDE_FOLDER := ./include ./
TEST_FOLDER := test
BUILD_FOLDER := build


# Hardcoded SRCS
SRC := \
	$(SRC_FOLDER)/messenger.cpp \
	$(SRC_FOLDER)/util.cpp  \

# Bad way to separate app and test builds...
# No .o file for reducing build-time
APP_SRC := \
	$(SRC) \
	$(SRC_FOLDER)/messenger_app.cpp

TEST_SRC := \
	$(SRC) \
	catch2/catch_amalgamated.cpp \
	$(TEST_FOLDER)/test_util.cpp \
	\
	$(TEST_FOLDER)/messenger_test.cpp \
	$(TEST_FOLDER)/msg_hdr_test.cpp \
	$(TEST_FOLDER)/util_test.cpp

APP_OBJS := $(APP_SRC:%.cpp=$(BUILD_FOLDER)/%.o)
TEST_OBJS := $(TEST_SRC:%.cpp=$(BUILD_FOLDER)/%.o)

APP_DEP := $(APP_OBJS:%.o=%.d)
TEST_DEP := $(TEST_OBJS:%.o=%.d)

DEP := $(APP_DEP) $(TEST_DEP)

INC := $(addprefix -I, $(INCLUDE_FOLDER))

CC := g++


.PHONY: run test clean

$(TARGET): $(APP_OBJS)
	$(CC) $^ -o $@


$(TARGET_TEST): $(TEST_OBJS) 
	$(CC) $^ -o $@

-include $(DEP)

$(BUILD_FOLDER)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(INC) -MMD -c $< -o $@ 

run: $(TARGET)
	./$(TARGET)

test: $(TARGET_TEST)
	./$(TARGET_TEST)

clean:
	rm -rf $(TARGET) $(TARGET_TEST) $(BUILD_FOLDER)