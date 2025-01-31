
CXX = g++
CXXFLAGS = -std=c++17
LDFLAGS = -lpaho-mqttpp3 -lpaho-mqtt3c

TARGET = MQTT

SRC = MQTT.cpp

# The build rule
MQTT: MQTT.cpp
	$(CXX) $(CXXFLAGS) MQTT.cpp -o MQTT $(LDFLAGS)

# Clean up build artifacts
clean:
	rm -f MQTT
