SET(CMAKE_SYSTEM_NAME Linux)
CMAKE_MINIMUM_REQUIRED(VERSION 3.2)
PROJECT(parse_template)

SET(CMAKE_CXX_FLAGS "-g  -std=c++11")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIC -fPIE -pie")

SET(CMAKE_CXX_COMPILER /home/SENSETIME/duanzhengbing/develop_tools/toolchain/aarch64-linux-android/bin/aarch64-linux-android-g++)
SET(CMAKE_C_COMPILER /home/SENSETIME/duanzhengbing/develop_tools/toolchain/aarch64-linux-android/bin/aarch64-linux-android-gcc)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)
AUX_SOURCE_DIRECTORY(./ SRC_LIST)


SET(LIB_PROTOBUF ${PROJECT_SOURCE_DIR}/lib/libprotobuf.a)
SET(LIB_SDK_FRAME ${PROJECT_SOURCE_DIR}/lib/libsdk_framework.a)
SET(LIB_ST_IMAGE_HELPER ${PROJECT_SOURCE_DIR}/lib/libst_imagehelper.a)

ADD_EXECUTABLE(${PROJECT_NAME} ${SRC_LIST})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_PROTOBUF} ${LIB_SDK_FRAME} ${LIB_ST_IMAGE_HELPER})

