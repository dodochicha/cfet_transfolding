# 編譯器與選項
CXX = g++
CXXFLAGS = -O3 -Wall -fopenmp

# Z3 本地套件 (z3_env) 設定
Z3_DIR = ./z3_env
ifneq ($(wildcard $(Z3_DIR)),)
    CXXFLAGS += -I$(Z3_DIR)/include
    LDFLAGS += -L$(Z3_DIR)/bin -Wl,-rpath,$(shell pwd)/$(Z3_DIR)/bin
endif

# 程式庫連結
LIBS = -lz3 -fopenmp

# 目錄
SRC_DIR = src
OBJ_DIR = obj

# 源檔案與目標檔案
CPP_SRC = $(wildcard $(SRC_DIR)/*.cpp)
CPP_OBJ = $(CPP_SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# 可執行檔
TARGET = main

# 預設目標
all: $(OBJ_DIR) $(TARGET)

# 編譯 .cpp 檔
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 連結（執行檔放在根目錄）
$(TARGET): $(CPP_OBJ)
	$(CXX) $(CPP_OBJ) -o $@ $(LDFLAGS) $(LIBS)

# 建立資料夾
$(OBJ_DIR):
	mkdir -p $@

# 清除中間檔
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean
