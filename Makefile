# 編譯器
CXX = g++

# 編譯選項
CXXFLAGS = -O3 -Wall -fopenmp

# 目錄
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# 源檔案與目標檔案
CPP_SRC = $(wildcard $(SRC_DIR)/*.cpp)
CPP_OBJ = $(CPP_SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# 可執行檔
TARGET = main

# 預設目標
all: $(BIN_DIR) $(OBJ_DIR) $(BIN_DIR)/$(TARGET)

# 編譯 .cpp 檔
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 連結
$(BIN_DIR)/$(TARGET): $(CPP_OBJ)
	$(CXX) $(CPP_OBJ) -o $@ -lz3 -fopenmp

# 建立資料夾
$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

# 清除中間檔
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
