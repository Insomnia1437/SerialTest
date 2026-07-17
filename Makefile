.PHONY: all clean rebuild

BUILD_DIR = build

all: $(BUILD_DIR)/Makefile
	@make -C $(BUILD_DIR)

$(BUILD_DIR)/Makefile:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && qmake ../src/SerialTest.pro

clean:
	@if [ -d $(BUILD_DIR) ]; then \
		make -C $(BUILD_DIR) clean || true; \
	fi

rebuild: clean all
