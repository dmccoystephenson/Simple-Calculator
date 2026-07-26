CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
# SDL2 + SDL2_image flags (resolved via pkg-config)
SDL_FLAGS = $(shell pkg-config --cflags --libs sdl2 SDL2_image)

# the shared, UI-agnostic core every frontend links against
ENGINE = CalculatorEngine.cpp CalculatorEngine.h

all: simpleCalculator textCalculator testingParsing

# GUI frontend (requires SDL2 and SDL2_image)
simpleCalculator: simpleCalculator.cpp $(ENGINE)
	$(CXX) $(CXXFLAGS) simpleCalculator.cpp CalculatorEngine.cpp -o simpleCalculator $(SDL_FLAGS)

# Text/CLI frontend (no SDL dependency)
textCalculator: textCalculator.cpp $(ENGINE)
	$(CXX) $(CXXFLAGS) textCalculator.cpp CalculatorEngine.cpp -o textCalculator

# Engine + parser self-tests (no SDL dependency)
testingParsing: testingParsing.cpp $(ENGINE)
	$(CXX) $(CXXFLAGS) testingParsing.cpp CalculatorEngine.cpp -o testingParsing

# Rebuilds the self-tests if any source changed, then runs them; a failing
# assertion aborts with a non-zero status, so this fails the make invocation.
test: testingParsing
	./testingParsing

clean:
	rm -f simpleCalculator textCalculator testingParsing simpleCalculator.exe testingParsing.exe textCalculator.exe

.PHONY: all clean test
