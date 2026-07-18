.PHONY: all clean configure rebuild test

BUILD_DIR = build

all: configure
	cmake --build $(BUILD_DIR) --parallel

configure:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON

test: all
	ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	cmake --build $(BUILD_DIR) --target clean

rebuild: clean all
