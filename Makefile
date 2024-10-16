# Makefile

# Nom du compilateur
CXX = g++

# Options de compilation
CXXFLAGS = -Wall -O2 -std=c++11

# Options d'édition de liens
LDFLAGS = -lpthread

# Fichiers source
SOURCES = main.cpp ws2812b.cpp

# Fichiers objets
OBJECTS = $(SOURCES:.cpp=.o)

# Nom de l'exécutable
TARGET = exemple

# Règle par défaut
all: $(TARGET)

# Règle pour créer l'exécutable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Règle pour créer les fichiers objets
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Règle pour nettoyer les fichiers générés
clean:
	rm -f $(OBJECTS) $(TARGET)

# Règle pour tout nettoyer (y compris les fichiers temporaires)
distclean: clean
	rm -f *~

# Règle pour exécuter le programme
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean distclean run
