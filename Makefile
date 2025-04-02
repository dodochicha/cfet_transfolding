# 编译器
CXX = g++
NVCC = nvcc

# 编译选项
CXXFLAGS = -O3 -Wall -fopenmp
NVCCFLAGS = -O3 -Xcompiler -fopenmp

# 目录
SRC_DIR = src
CPP_OBJ_DIR = cpp_obj
CU_OBJ_DIR = cu_obj
BIN_DIR = bin

# 源文件
CPP_SRC = $(wildcard $(SRC_DIR)/*.cpp)
CU_SRC = $(wildcard $(SRC_DIR)/*.cu)

# 目标文件
CPP_OBJ = $(CPP_SRC:$(SRC_DIR)/%.cpp=$(CPP_OBJ_DIR)/%.o)
CU_OBJ = $(CU_SRC:$(SRC_DIR)/%.cu=$(CU_OBJ_DIR)/%.o)

# 可执行文件
# TARGET = $(BIN_DIR)/main
TARGET = main

# 默认目标
all: $(BIN_DIR) $(CPP_OBJ_DIR) $(CU_OBJ_DIR) $(TARGET)

# 编译 .cpp 文件
$(CPP_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 编译 .cu 文件
$(CU_OBJ_DIR)/%.o: $(SRC_DIR)/%.cu
	$(NVCC) $(NVCCFLAGS) -c $< -o $@

# 链接目标文件
$(TARGET): $(CPP_OBJ) $(CU_OBJ)
	$(NVCC) $(CPP_OBJ) $(CU_OBJ) -o $(TARGET) -lz3 -Xcompiler -fopenmp

# 清理
clean:
	rm -rf $(CPP_OBJ_DIR) $(CU_OBJ_DIR) $(BIN_DIR)

# 创建目录
$(CPP_OBJ_DIR) $(CU_OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

.PHONY: all clean
