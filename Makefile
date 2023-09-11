# Hardcoded SRCS
src := \
	messenger_test.cpp \
	messenger.cpp \
	util.cpp

TARGET := messenger_app

run:
	g++ $(src) -o $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)