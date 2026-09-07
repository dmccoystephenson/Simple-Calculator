CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
# SDL2 + SDL2_image flags (resolved via pkg-config)
SDL_FLAGS = $(shell pkg-config --cflags --libs sdl2 SDL2_image)

# the shared, UI-agnostic core every frontend links against
ENGINE = CalculatorEngine.cpp CalculatorEngine.h

all: simpleCalculator textCalculator testingParsing testingFrontend testingTextFrontend

# GUI frontend (requires SDL2 and SDL2_image)
simpleCalculator: simpleCalculator.cpp $(ENGINE)
	$(CXX) $(CXXFLAGS) simpleCalculator.cpp CalculatorEngine.cpp -o simpleCalculator $(SDL_FLAGS)

# Text/CLI frontend (no SDL dependency)
textCalculator: textCalculator.cpp $(ENGINE)
	$(CXX) $(CXXFLAGS) textCalculator.cpp CalculatorEngine.cpp -o textCalculator

# Engine + parser self-tests (no SDL dependency)
testingParsing: testingParsing.cpp $(ENGINE)
	$(CXX) $(CXXFLAGS) testingParsing.cpp CalculatorEngine.cpp -o testingParsing

# SDL frontend self-tests. It includes simpleCalculator.cpp directly (that file
# has no header), so it links SDL2 the same way the GUI does; it runs headlessly
# via SDL's dummy video driver, which it selects itself.
testingFrontend: testingFrontend.cpp simpleCalculator.cpp $(ENGINE)
	$(CXX) $(CXXFLAGS) testingFrontend.cpp CalculatorEngine.cpp -o testingFrontend $(SDL_FLAGS)

# Text/CLI frontend self-tests. It includes textCalculator.cpp directly (that
# file has no header, and its main() is the thing under test), so — unlike
# testingFrontend — it needs no SDL and no PNG assets.
testingTextFrontend: testingTextFrontend.cpp textCalculator.cpp $(ENGINE)
	$(CXX) $(CXXFLAGS) testingTextFrontend.cpp CalculatorEngine.cpp -o testingTextFrontend

# Rebuilds the self-tests if any source changed, then runs them; a failing
# assertion aborts with a non-zero status, so this fails the make invocation.
# testingFrontend loads the PNG assets by relative path, so it has to run from
# the repository directory — which is where make was invoked.
test: testingParsing testingFrontend testingTextFrontend
	./testingParsing
	./testingFrontend
	./testingTextFrontend

clean:
	rm -f simpleCalculator textCalculator testingParsing testingFrontend testingTextFrontend simpleCalculator.exe testingParsing.exe testingFrontend.exe testingTextFrontend.exe textCalculator.exe

.PHONY: all clean test
