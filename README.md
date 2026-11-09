# Sistema de ocupación y servicios para hotel (C++)


**Universidad Mariano Gálvez de Guatemala · Algoritmos, Sección A · 2026**

**Estudiante:** Oscar Roberto Pérez Velásquez · **Carné:** 2294-26-7087


**Catedrático:** Ing. Mag. Jhensser Aguirre
**Proyecto 09:** Hotel y hospedaje · **Variante B:** Ocupación y servicios

Sistema de consola en C++17 que administra un hotel de 4 pisos y 20 habitaciones. Permite hacer
reservaciones, check-in, cargar consumos (restaurante, minibar, lavandería y spa), hacer check-out
con boleta, y manejar la limpieza y el mantenimiento de las habitaciones. Todos los datos se guardan
en archivos de texto.

**Reto algorítmico:** controlar estados mutuamente excluyentes de las habitaciones y evitar reservas
incompatibles. Se resuelve con la matriz `TRANSICIONES[5][5]` y la función `cambiarEstadoHabitacion`,
que es la única que cambia el estado de una habitación.

## Contenido del repositorio

| Archivo | Descripción |
|---|---|
| `main.cpp` | Código fuente completo del sistema |
| `CMakeLists.txt` | Configuración para compilar con CMake (CLion u otro IDE) |
| `datos/` | Datos de prueba: `habitaciones.txt`, `cuentas.txt`, `consumos.txt`, `boletas.txt` |

## Cómo compilar y ejecutar

### Opción 1: CLion
1. **File → Open** y elegir la carpeta del proyecto (la que tiene `CMakeLists.txt`).
2. Esperar a que CMake termine de cargar.
3. Presionar **Run ▶** y escribir las opciones en la ventana **Run**.

### Opción 2: terminal con g++
Desde la carpeta del proyecto:

```bash
g++ -std=c++17 main.cpp -o hotel
./hotel          # Linux / macOS
hotel.exe        # Windows
```

> El programa debe ejecutarse **desde la carpeta del proyecto**, para que encuentre la carpeta `datos`.

### Opción 3: CMake desde la terminal
```bash
cmake -S . -B build
cmake --build build
./build/HotelOcupacionServicios
```

## Datos de prueba
La carpeta `datos` ya incluye un ejemplo: la habitación 101 reservada (María Santos, cuenta 1003),
la 103 ocupada (Ana López, cuenta 1001), la 104 en mantenimiento y una cuenta cerrada con su boleta
(Carlos Díaz, habitación 105).
Para empezar con el hotel vacío, borre los archivos `.txt` de `datos`.

## Requisitos técnicos cumplidos
- Menú y submenús con validación de todas las entradas.
- Matriz `habs[4][5]` para el hotel y arreglos para cuentas y consumos.
- Búsquedas secuenciales: por número de habitación, por nombre, por DPI y por estado.
- Ordenamiento manual: **burbuja** (ranking de habitaciones por ingresos) y **selección** (cuentas por total).
- Persistencia con `fstream` en archivos de texto.
- Cuatro reportes: ocupación, ingresos, huéspedes vigentes y mantenimiento.

## Versión final
Etiqueta: **`v1.0-final`**
