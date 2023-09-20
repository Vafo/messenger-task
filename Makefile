TARGET := messenger_app
TARGET_TEST := messenger_test

TEST_FOLDER := ./test
BUILD_FOLDER := ./build

# Hardcoded SRCS
SRC := \
	messenger.cpp \
	util.cpp  \

# Bad way to separate app and test builds...
# No .o file for reducing build-time
APP_SRC := \
	messenger_app.cpp

TEST_SRC := \
	catch2/catch_amalgamated.cpp \
	$(TEST_FOLDER)/test_util.cpp \
	\
	$(TEST_FOLDER)/messenger_test.cpp \
	$(TEST_FOLDER)/msg_hdr_test.cpp \
	$(TEST_FOLDER)/util_test.cpp

APP_OBJS := $(SRC:%.cpp=$(BUILD_FOLDER)/%.o) $(APP_SRC:%.cpp=$(BUILD_FOLDER)/%.o)
TEST_OBJS := $(SRC:%.cpp=$(BUILD_FOLDER)/%.o) $(TEST_SRC:%.cpp=$(BUILD_FOLDER)/%.o)


INC := \
	./catch2 

CC := g++

$(BUILD_FOLDER)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) -c -o $@ $< -I$(INC)

.PHONY: run test clean

$(TARGET): $(APP_OBJS)
	$(CC) $^ -o $@


$(TARGET_TEST): $(TEST_OBJS) 
	$(CC) $^ -o $@


run: $(TARGET)
	./$(TARGET)

test: $(TARGET_TEST)
	./$(TARGET_TEST)

clean:
	rm -rf $(TARGET) $(TARGET_TEST) $(BUILD_FOLDER)