PROJECT_SOURCE_DIR ?= $(abspath ./)
BUILD_DIR ?= $(PROJECT_SOURCE_DIR)/build
INSTALL_DIR ?= $(BUILD_DIR)/install
NUM_JOB ?= 8
JQ ?= jq

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


INPUT_GEOJSON_PATH ?= data/sample1.json
GEOJSON_BASENAME = $(shell basename $(abspath $(INPUT_GEOJSON_PATH)))

OUTPUT_DIR_JS ?= $(BUILD_DIR)/js
OUTPUT_PBF_JS = $(OUTPUT_DIR_JS)/$(GEOJSON_BASENAME).pbf
OUTPUT_JSN_JS = $(OUTPUT_PBF_JS).json

OUTPUT_DIR_CXX ?= $(BUILD_DIR)/cxx
OUTPUT_PBF_CXX = $(OUTPUT_DIR_CXX)/$(GEOJSON_BASENAME).pbf
OUTPUT_JSN_CXX = $(OUTPUT_PBF_CXX).json

roundtrip_test_js:
	@umask 0000 && mkdir -p $(OUTPUT_DIR_JS)
	json2geobuf $(INPUT_GEOJSON_PATH) > $(OUTPUT_PBF_JS)
	build/bin/pbf_decoder $(OUTPUT_PBF_JS) > $(OUTPUT_DIR_JS)/pbf.txt
	geobuf2json $(OUTPUT_PBF_JS) | $(JQ) . > $(OUTPUT_JSN_JS)
	cat $(INPUT_GEOJSON_PATH) | $(JQ) . > $(OUTPUT_DIR_JS)/$(GEOJSON_BASENAME)
roundtrip_test_cpp:
	@umask 0000 && mkdir -p $(OUTPUT_DIR_CXX)
	$(BUILD_DIR)/bin/json2geobuf $(INPUT_GEOJSON_PATH) > $(OUTPUT_PBF_CXX)
	build/bin/pbf_decoder $(OUTPUT_PBF_CXX) > $(OUTPUT_DIR_CXX)/pbf.txt
	$(BUILD_DIR)/bin/geobuf2json $(OUTPUT_PBF_CXX) | $(JQ) . > $(OUTPUT_JSN_CXX)
diff:
	code --diff $(OUTPUT_DIR_JS)/pbf.txt $(OUTPUT_DIR_CXX)/pbf.txt
	code --diff $(OUTPUT_JSN_JS) $(OUTPUT_JSN_CXX)

test:
	make roundtrip_test_js roundtrip_test_cpp diff
