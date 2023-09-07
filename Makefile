src ?= 
TARGET := messenger_app

run:
	g++ $(src) -o $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)