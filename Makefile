CXX = g++
CXXFLAGS = -std=c++17
LDFLAGS = -lpaho-mqttpp3 -lpaho-mqtt3c -lsqlite3
TARGET = MQTT
SRC = MQTT.cpp

all: $(TARGET)
	@export DYLD_LIBRARY_PATH=/usr/local/lib:$$DYLD_LIBRARY_PATH && ./$(TARGET)
MQTT: MQTT.cpp
	$(CXX) $(CXXFLAGS) MQTT.cpp -o MQTT $(LDFLAGS)

clean:
	rm -f MQTT