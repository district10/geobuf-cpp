PROJECT_SOURCE_DIR ?= $(abspath ./)
BUILD_DIR ?= $(PROJECT_SOURCE_DIR)/build
INSTALL_DIR ?= $(BUILD_DIR)/install
NUM_JOB ?= 8

all:
	@echo nothing special

lint: lintcpp lintcmake
lintcpp:
	python3 -m mdk_tools.cli.cpp_lint .
lintcmake:
	python3 -m mdk_tools.cli.cmake_lint .
.PHONY: lint lintcpp lintcmake

reset_submodules:
	git submodule update --init --recursive

clean:
	rm -rf $(BUILD_DIR)

CMAKE_ARGS ?= \
	-DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR) \
	-DBUILD_SHARED_LIBS=OFF
build:
	mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && \
	cmake $(PROJECT_SOURCE_DIR) $(CMAKE_ARGS) && \
	make -j$(NUM_JOB) && make install
.PHONY: build

test_all:
	@cd build && for t in $(wildcard $(BUILD_DIR)/bin/test_*); do echo $$t && eval $$t >/dev/null 2>&1 && echo 'ok' || echo $(RED)Not Ok$(NC); done
