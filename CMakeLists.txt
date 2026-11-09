cmake_minimum_required(VERSION 3.16)
project(HotelOcupacionServicios CXX)

# El curso exige C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(HotelOcupacionServicios main.cpp)

# Ruta de la carpeta "datos" de ESTE proyecto (se calcula en cada computadora,
# así el programa encuentra sus archivos sin rutas fijas).
target_compile_definitions(HotelOcupacionServicios PRIVATE
        CARPETA_DATOS="${CMAKE_SOURCE_DIR}/datos/")
