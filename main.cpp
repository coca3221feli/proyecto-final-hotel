// =====================================================================
//  PROYECTO FINAL - ALGORITMOS 2026        Ing. Mag. Jhensser Aguirre
//  Estudiante : Oscar Roberto Perez Velasquez     Carne: 2294-26-7087
//
//  PROYECTO 09 - HOTEL Y HOSPEDAJE
//  VARIANTE B  - Ocupacion y servicios
//    Manejar estados de habitacion, consumos, mantenimiento y cierre
//    de cuenta por huesped.
//  RETO ALGORITMICO
//    Controlar estados mutuamente excluyentes de las habitaciones y
//    evitar reservas incompatibles.
//
//  IDEA GENERAL
//  - El hotel es una MATRIZ de habitaciones: filas = pisos, columnas =
//    habitaciones del piso. La habitacion 302 esta en piso 3, columna 2.
//  - Cada habitacion tiene UN solo estado a la vez (Disponible,
//    Reservada, Ocupada, Limpieza o Mantenimiento).
//  - Los cambios de estado permitidos estan en otra matriz
//    (TRANSICIONES). Si un cambio no esta permitido, no se hace.
//  - Cada huesped tiene una CUENTA donde se cargan las noches y los
//    consumos (restaurante, minibar, lavanderia, spa). Al hacer
//    check-out se cierra la cuenta y se emite la boleta.
// =====================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <cstdlib>

using namespace std;

// =====================================================================
//  0. CONSTANTES Y REGLAS DEL HOTEL
//     (aqui se hacen la mayoria de cambios en vivo)
// =====================================================================
// CMakeLists.txt define CARPETA_DATOS con la ruta de la carpeta "datos".
// Si se compila sin CMake (por ejemplo con g++ en la terminal), se usa la
// carpeta "datos" relativa: ejecute el programa desde la carpeta del proyecto.
#ifndef CARPETA_DATOS
#define CARPETA_DATOS "datos/"
#endif

const string ARCHIVO_HABITACIONES = string(CARPETA_DATOS) + "habitaciones.txt";
const string ARCHIVO_CUENTAS      = string(CARPETA_DATOS) + "cuentas.txt";
const string ARCHIVO_CONSUMOS     = string(CARPETA_DATOS) + "consumos.txt";
const string ARCHIVO_BOLETAS      = string(CARPETA_DATOS) + "boletas.txt";
const string ARCHIVO_REPORTES     = string(CARPETA_DATOS) + "Reportes_Hotel.txt";

// Tamano del hotel (matriz)
const int PISOS          = 4;
const int HABS_POR_PISO  = 5;
const int TOTAL_HABS     = PISOS * HABS_POR_PISO;

// Capacidad de los arreglos
const int MAX_CUENTAS  = 100;
const int MAX_CONSUMOS = 300;

// Limites de captura
const int    MAX_NOCHES       = 30;
const double MAX_MONTO_CONSUMO = 5000.0;

// ---- Tipos de habitacion ----
const int NUM_TIPOS = 3;
const string NOMBRE_TIPO[NUM_TIPOS] = {"Sencilla", "Doble", "Suite"};
const double TARIFA_NOCHE[NUM_TIPOS] = {350.00, 550.00, 950.00};   // quetzales
const int    CAPACIDAD[NUM_TIPOS]    = {1, 2, 4};                  // personas

// ---- Estados de habitacion (mutuamente excluyentes) ----
const int NUM_ESTADOS   = 5;
const int DISPONIBLE    = 0;
const int RESERVADA     = 1;
const int OCUPADA       = 2;
const int LIMPIEZA      = 3;
const int MANTENIMIENTO = 4;
const string NOMBRE_ESTADO[NUM_ESTADOS] = {"Disponible", "Reservada", "Ocupada",
                                           "Limpieza", "Mantenimiento"};
const char LETRA_ESTADO[NUM_ESTADOS] = {'D', 'R', 'O', 'L', 'M'};

// ---- RETO ALGORITMICO: matriz de transiciones permitidas ----
// TRANSICIONES[actual][nuevo] = true si se puede pasar de "actual" a "nuevo".
//                                 D      R      O      L      M
const bool TRANSICIONES[NUM_ESTADOS][NUM_ESTADOS] = {
    /* desde Disponible    */ { false, true,  true,  false, true  },
    /* desde Reservada     */ { true,  false, true,  false, false },
    /* desde Ocupada       */ { false, false, false, true,  false },
    /* desde Limpieza      */ { true,  false, false, false, true  },
    /* desde Mantenimiento */ { true,  false, false, false, false }
};

// ---- Estados de una cuenta de huesped ----
const int CTA_RESERVADA = 0;   // reservo pero aun no llega
const int CTA_ABIERTA   = 1;   // esta hospedado, se le cargan consumos
const int CTA_CERRADA   = 2;   // ya pago y salio
const int CTA_CANCELADA = 3;   // cancelo la reserva
const string NOMBRE_ESTADO_CTA[4] = {"Reservada", "Abierta", "Cerrada", "Cancelada"};

// ---- Servicios que se pueden consumir ----
const int NUM_SERVICIOS = 4;
const string NOMBRE_SERVICIO[NUM_SERVICIOS] = {"Restaurante", "Minibar",
                                               "Lavanderia", "Spa"};

// ---- Metodos de pago ----
const string NOMBRE_PAGO[3] = {"-", "Efectivo", "Tarjeta"};

// =====================================================================
//  1. ESTRUCTURAS DE DATOS
// =====================================================================
struct Habitacion {
    int    numero;          // ej. 302 = piso 3, habitacion 2
    int    tipo;            // 0 Sencilla, 1 Doble, 2 Suite
    int    estado;          // DISPONIBLE, RESERVADA, OCUPADA, LIMPIEZA, MANTENIMIENTO
    int    cuentaActual;    // codigo de la cuenta que la usa, 0 si ninguna
    double ingresos;        // acumulado de todo lo cobrado en esta habitacion
    int    mantenimientos;  // contador de veces que fue a mantenimiento
};

struct Cuenta {
    int    codigo;          // unico, autoincremental (1001, 1002...)
    string nombre;          // nombre del huesped
    string documento;       // DPI de 13 digitos
    int    personas;
    int    habitacion;      // numero de habitacion asignada
    int    noches;
    int    estado;          // CTA_RESERVADA, CTA_ABIERTA, CTA_CERRADA, CTA_CANCELADA
    double totalConsumos;   // acumulador de consumos cargados
    int    metodoPago;      // 0 ninguno, 1 efectivo, 2 tarjeta
    double totalPagado;     // total de la boleta al cerrar
};

struct Consumo {
    int    codigoCuenta;
    int    servicio;        // indice de NOMBRE_SERVICIO
    string descripcion;
    double monto;
};

// Todos los datos del sistema juntos, para pasarlos facil a las funciones.
struct Hotel {
    Habitacion habs[PISOS][HABS_POR_PISO];   // MATRIZ de habitaciones
    Cuenta     cuentas[MAX_CUENTAS];         // arreglo de cuentas
    int        totalCuentas = 0;
    Consumo    consumos[MAX_CONSUMOS];       // arreglo de consumos
    int        totalConsumos = 0;
    int        siguienteCodigo = 1001;
};

// =====================================================================
//  2. LECTURA Y VALIDACION DE ENTRADAS
//     Todas leen la linea completa; nada de lo que se escriba traba
//     el programa.
// =====================================================================

// Si la entrada se termina (ventana cerrada, Ctrl+D / Ctrl+Z), se sale de
// forma ordenada. Los datos ya estan guardados porque se guardan en cada operacion.
void finDeEntrada() {
    cout << "\nEntrada finalizada. Cerrando el sistema.\n";
    exit(0);
}
int leerEntero(const string& mensaje, int min, int max) {
    string linea;
    while (true) {
        cout << mensaje;
        if (!getline(cin, linea)) finDeEntrada();
        stringstream ss(linea);
        int valor;
        char sobra;
        if (ss >> valor && !(ss >> sobra)) {
            if (valor >= min && valor <= max) return valor;
            cout << "  Error: debe estar entre " << min << " y " << max << ".\n";
        } else {
            cout << "  Error: ingrese un numero entero.\n";
        }
    }
}

double leerDecimal(const string& mensaje, double min, double max) {
    string linea;
    while (true) {
        cout << mensaje;
        if (!getline(cin, linea)) finDeEntrada();
        stringstream ss(linea);
        double valor;
        char sobra;
        if (ss >> valor && !(ss >> sobra)) {
            if (valor >= min && valor <= max) return valor;
            cout << "  Error: debe estar entre " << fixed << setprecision(2)
                 << min << " y " << max << ".\n";
        } else {
            cout << "  Error: ingrese un numero (use punto decimal).\n";
        }
    }
}

// Quita espacios al inicio y al final.
string recortar(const string& texto) {
    size_t ini = texto.find_first_not_of(' ');
    if (ini == string::npos) return "";
    size_t fin = texto.find_last_not_of(' ');
    return texto.substr(ini, fin - ini + 1);
}

// Texto libre, no vacio y sin '|' (reservado para los archivos).
string leerTexto(const string& mensaje) {
    string linea;
    while (true) {
        cout << mensaje;
        if (!getline(cin, linea)) finDeEntrada();
        linea = recortar(linea);
        if (linea.empty())                     cout << "  Error: no puede estar vacio.\n";
        else if (linea.find('|') != string::npos) cout << "  Error: no use '|'.\n";
        else return linea;
    }
}

// Nombre: solo letras y espacios, minimo 3 caracteres.
string leerNombre(const string& mensaje) {
    while (true) {
        string nombre = leerTexto(mensaje);
        bool valido = nombre.size() >= 3;
        for (size_t i = 0; i < nombre.size() && valido; i++) {
            char c = nombre[i];
            bool esLetra = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                           c == ' ' || (unsigned char)c >= 128;   // >=128: tildes y n
            if (!esLetra) valido = false;
        }
        if (valido) return nombre;
        cout << "  Error: el nombre solo lleva letras y minimo 3 caracteres.\n";
    }
}

// DPI: exactamente 13 digitos.
string leerDocumento(const string& mensaje) {
    while (true) {
        string doc = leerTexto(mensaje);
        bool valido = doc.size() == 13;
        for (size_t i = 0; i < doc.size() && valido; i++) {
            if (doc[i] < '0' || doc[i] > '9') valido = false;
        }
        if (valido) return doc;
        cout << "  Error: el DPI debe tener exactamente 13 digitos.\n";
    }
}

bool confirmar(const string& mensaje) {
    string linea;
    while (true) {
        cout << mensaje << " (S/N): ";
        if (!getline(cin, linea)) finDeEntrada();
        linea = recortar(linea);
        if (linea == "S" || linea == "s") return true;
        if (linea == "N" || linea == "n") return false;
        cout << "  Error: responda S o N.\n";
    }
}

string aMinusculas(string texto) {
    for (size_t i = 0; i < texto.size(); i++) {
        if (texto[i] >= 'A' && texto[i] <= 'Z') texto[i] = texto[i] - 'A' + 'a';
    }
    return texto;
}

void pausar() {
    cout << "\nPresione ENTER para continuar...";
    string tmp;
    if (!getline(cin, tmp)) finDeEntrada();
}

// =====================================================================
//  3. INICIALIZACION DEL HOTEL
// =====================================================================
// Crea las 20 habitaciones vacias. El tipo depende de la columna:
//   columnas 1 y 2 = Sencilla, 3 y 4 = Doble, 5 = Suite.
void inicializarHotel(Hotel& h) {
    for (int p = 0; p < PISOS; p++) {
        for (int c = 0; c < HABS_POR_PISO; c++) {
            Habitacion& hab = h.habs[p][c];
            hab.numero         = (p + 1) * 100 + (c + 1);
            hab.tipo           = (c < 2) ? 0 : (c < 4 ? 1 : 2);
            hab.estado         = DISPONIBLE;
            hab.cuentaActual   = 0;
            hab.ingresos       = 0.0;
            hab.mantenimientos = 0;
        }
    }
    h.totalCuentas    = 0;
    h.totalConsumos   = 0;
    h.siguienteCodigo = 1001;
}

// =====================================================================
//  4. BUSQUEDAS (secuenciales, implementadas a mano)
// =====================================================================

// Busca una habitacion por numero recorriendo la matriz fila por fila.
// Devuelve un puntero a la habitacion o nullptr si no existe.
Habitacion* buscarHabitacion(Hotel& h, int numero) {
    for (int p = 0; p < PISOS; p++) {
        for (int c = 0; c < HABS_POR_PISO; c++) {
            if (h.habs[p][c].numero == numero) return &h.habs[p][c];
        }
    }
    return nullptr;
}

// Busca una cuenta por su codigo. Devuelve la posicion o -1.
int buscarCuentaPorCodigo(const Hotel& h, int codigo) {
    for (int i = 0; i < h.totalCuentas; i++) {
        if (h.cuentas[i].codigo == codigo) return i;
    }
    return -1;
}

// Busca si un DPI ya tiene una cuenta VIGENTE (reservada o abierta).
// Sirve para evitar reservas incompatibles: una persona, una estadia.
int buscarCuentaVigentePorDocumento(const Hotel& h, const string& documento) {
    for (int i = 0; i < h.totalCuentas; i++) {
        bool vigente = h.cuentas[i].estado == CTA_RESERVADA ||
                       h.cuentas[i].estado == CTA_ABIERTA;
        if (vigente && h.cuentas[i].documento == documento) return i;
    }
    return -1;
}

// Segundo criterio: busca huespedes por parte del nombre.
// Guarda las posiciones encontradas en "resultados" y devuelve cuantas son.
int buscarCuentasPorNombre(const Hotel& h, const string& texto, int resultados[]) {
    int n = 0;
    string buscado = aMinusculas(texto);
    for (int i = 0; i < h.totalCuentas; i++) {
        if (aMinusculas(h.cuentas[i].nombre).find(buscado) != string::npos) {
            resultados[n] = i;
            n++;
        }
    }
    return n;
}

// =====================================================================
//  5. RETO ALGORITMICO: CONTROL DE ESTADOS
// =====================================================================

// Consulta la matriz de transiciones.
bool transicionPermitida(int actual, int nuevo) {
    if (actual < 0 || actual >= NUM_ESTADOS || nuevo < 0 || nuevo >= NUM_ESTADOS) {
        return false;
    }
    return TRANSICIONES[actual][nuevo];
}

// UNICA funcion que cambia el estado de una habitacion.
// Si el cambio no es valido, avisa y no toca nada.
bool cambiarEstadoHabitacion(Habitacion& hab, int nuevo) {
    if (!transicionPermitida(hab.estado, nuevo)) {
        cout << "  No permitido: la habitacion " << hab.numero << " esta en "
             << NOMBRE_ESTADO[hab.estado] << " y no puede pasar a "
             << NOMBRE_ESTADO[nuevo] << ".\n";
        return false;
    }
    hab.estado = nuevo;
    return true;
}

// Total actual de una cuenta = noches x tarifa + consumos.
double calcularTotalCuenta(Hotel& h, const Cuenta& cta) {
    Habitacion* hab = buscarHabitacion(h, cta.habitacion);
    double tarifa = (hab != nullptr) ? TARIFA_NOCHE[hab->tipo] : 0.0;
    return cta.noches * tarifa + cta.totalConsumos;
}

// =====================================================================
//  6. VISUALIZACION
// =====================================================================
void imprimirMapa(const Hotel& h) {
    cout << "\n   MAPA DE OCUPACION   (D=Disponible R=Reservada O=Ocupada "
            "L=Limpieza M=Mantenimiento)\n";
    for (int p = PISOS - 1; p >= 0; p--) {           // el piso mas alto arriba
        cout << "   Piso " << (p + 1) << " | ";
        for (int c = 0; c < HABS_POR_PISO; c++) {
            cout << h.habs[p][c].numero << "[" << LETRA_ESTADO[h.habs[p][c].estado] << "]  ";
        }
        cout << "\n";
    }
    cout << "   Tipos por columna: 1-2 Sencilla (Q" << TARIFA_NOCHE[0]
         << "), 3-4 Doble (Q" << TARIFA_NOCHE[1] << "), 5 Suite (Q" << TARIFA_NOCHE[2] << ")\n";
}

void imprimirEncabezadoHab() {
    cout << left << setw(8) << "Hab." << setw(10) << "Tipo" << setw(15) << "Estado"
         << right << setw(10) << "Tarifa" << setw(8) << "Cuenta"
         << setw(12) << "Ingresos" << setw(7) << "Mant." << "\n"
         << string(70, '-') << "\n";
}

void imprimirHabitacion(const Habitacion& hab) {
    cout << left << setw(8) << hab.numero << setw(10) << NOMBRE_TIPO[hab.tipo]
         << setw(15) << NOMBRE_ESTADO[hab.estado] << right << fixed << setprecision(2)
         << setw(10) << TARIFA_NOCHE[hab.tipo] << setw(8)
         << (hab.cuentaActual == 0 ? string("-") : to_string(hab.cuentaActual))
         << setw(12) << hab.ingresos << setw(7) << hab.mantenimientos << "\n";
}

void imprimirEncabezadoCuenta() {
    cout << left << setw(7) << "Cuenta" << setw(22) << "Huesped" << setw(15) << "DPI"
         << setw(6) << "Hab." << right << setw(7) << "Noches" << setw(12) << "Consumos"
         << "  " << left << "Estado" << "\n" << string(80, '-') << "\n";
}

void imprimirCuenta(const Cuenta& c) {
    cout << left << setw(7) << c.codigo << setw(22) << c.nombre.substr(0, 21)
         << setw(15) << c.documento << setw(6) << c.habitacion << right
         << setw(7) << c.noches << setw(12) << fixed << setprecision(2) << c.totalConsumos
         << "  " << left << NOMBRE_ESTADO_CTA[c.estado] << "\n";
}

// Muestra las habitaciones disponibles de un tipo. Devuelve cuantas hay.
int mostrarDisponiblesPorTipo(const Hotel& h, int tipo) {
    int cuantas = 0;
    cout << "Habitaciones " << NOMBRE_TIPO[tipo] << " disponibles: ";
    for (int p = 0; p < PISOS; p++) {
        for (int c = 0; c < HABS_POR_PISO; c++) {
            const Habitacion& hab = h.habs[p][c];
            if (hab.tipo == tipo && hab.estado == DISPONIBLE) {
                cout << hab.numero << " ";
                cuantas++;
            }
        }
    }
    if (cuantas == 0) cout << "ninguna";
    cout << "\n";
    return cuantas;
}

int elegirTipo() {
    cout << "Tipos de habitacion:\n";
    for (int t = 0; t < NUM_TIPOS; t++) {
        cout << "  " << (t + 1) << ". " << NOMBRE_TIPO[t] << " - Q" << fixed
             << setprecision(2) << TARIFA_NOCHE[t] << " por noche, hasta "
             << CAPACIDAD[t] << " persona(s)\n";
    }
    return leerEntero("Seleccione tipo: ", 1, NUM_TIPOS) - 1;
}

// =====================================================================
//  7. PERSISTENCIA (fstream). Se guarda despues de cada operacion.
// =====================================================================
bool guardarTodo(const Hotel& h) {
    ofstream fh(ARCHIVO_HABITACIONES), fc(ARCHIVO_CUENTAS), fs(ARCHIVO_CONSUMOS);
    if (!fh || !fc || !fs) {
        cout << "  Error: no se pudieron abrir los archivos de datos para guardar.\n";
        return false;
    }
    // habitaciones: numero|tipo|estado|cuentaActual|ingresos|mantenimientos
    for (int p = 0; p < PISOS; p++) {
        for (int c = 0; c < HABS_POR_PISO; c++) {
            const Habitacion& x = h.habs[p][c];
            fh << x.numero << '|' << x.tipo << '|' << x.estado << '|' << x.cuentaActual
               << '|' << x.ingresos << '|' << x.mantenimientos << '\n';
        }
    }
    // cuentas: codigo|nombre|documento|personas|habitacion|noches|estado|consumos|pago|pagado
    for (int i = 0; i < h.totalCuentas; i++) {
        const Cuenta& x = h.cuentas[i];
        fc << x.codigo << '|' << x.nombre << '|' << x.documento << '|' << x.personas << '|'
           << x.habitacion << '|' << x.noches << '|' << x.estado << '|' << x.totalConsumos
           << '|' << x.metodoPago << '|' << x.totalPagado << '\n';
    }
    // consumos: codigoCuenta|servicio|descripcion|monto
    for (int i = 0; i < h.totalConsumos; i++) {
        const Consumo& x = h.consumos[i];
        fs << x.codigoCuenta << '|' << x.servicio << '|' << x.descripcion << '|'
           << x.monto << '\n';
    }
    return true;
}

// Separa una linea por '|' en el arreglo "campos". Devuelve cuantos encontro.
int separarCampos(const string& linea, string campos[], int maxCampos) {
    stringstream ss(linea);
    int n = 0;
    while (n < maxCampos && getline(ss, campos[n], '|')) n++;
    return n;
}

void cargarTodo(Hotel& h) {
    inicializarHotel(h);
    string linea, campo[10];
    int invalidas = 0;

    ifstream fh(ARCHIVO_HABITACIONES);
    if (!fh) {
        cout << "Aviso: no hay datos guardados, el hotel inicia vacio.\n";
        return;
    }
    while (getline(fh, linea)) {
        if (separarCampos(linea, campo, 6) != 6) { if (!linea.empty()) invalidas++; continue; }
        stringstream ss(campo[0] + " " + campo[2] + " " + campo[3] + " " + campo[4] + " " + campo[5]);
        int numero, estado, cuenta, mant;
        double ingresos;
        Habitacion* hab = nullptr;
        if (ss >> numero >> estado >> cuenta >> ingresos >> mant)
            hab = buscarHabitacion(h, numero);
        if (hab == nullptr || estado < 0 || estado >= NUM_ESTADOS) { invalidas++; continue; }
        hab->estado = estado;
        hab->cuentaActual = cuenta;
        hab->ingresos = ingresos;
        hab->mantenimientos = mant;
    }

    ifstream fc(ARCHIVO_CUENTAS);
    while (fc && getline(fc, linea) && h.totalCuentas < MAX_CUENTAS) {
        if (separarCampos(linea, campo, 10) != 10) { if (!linea.empty()) invalidas++; continue; }
        Cuenta x;
        x.nombre = campo[1];
        x.documento = campo[2];
        stringstream ss(campo[0] + " " + campo[3] + " " + campo[4] + " " + campo[5] + " " +
                        campo[6] + " " + campo[7] + " " + campo[8] + " " + campo[9]);
        if (!(ss >> x.codigo >> x.personas >> x.habitacion >> x.noches >> x.estado
                 >> x.totalConsumos >> x.metodoPago >> x.totalPagado) ||
            x.estado < 0 || x.estado > 3 || buscarCuentaPorCodigo(h, x.codigo) != -1) {
            invalidas++;
            continue;
        }
        h.cuentas[h.totalCuentas++] = x;
        if (x.codigo >= h.siguienteCodigo) h.siguienteCodigo = x.codigo + 1;
    }

    ifstream fs(ARCHIVO_CONSUMOS);
    while (fs && getline(fs, linea) && h.totalConsumos < MAX_CONSUMOS) {
        if (separarCampos(linea, campo, 4) != 4) { if (!linea.empty()) invalidas++; continue; }
        Consumo x;
        x.descripcion = campo[2];
        stringstream ss(campo[0] + " " + campo[1] + " " + campo[3]);
        if (!(ss >> x.codigoCuenta >> x.servicio >> x.monto) ||
            x.servicio < 0 || x.servicio >= NUM_SERVICIOS) {
            invalidas++;
            continue;
        }
        h.consumos[h.totalConsumos++] = x;
    }

    cout << "Datos cargados: " << h.totalCuentas << " cuenta(s), " << h.totalConsumos
         << " consumo(s).";
    if (invalidas > 0) cout << " Se omitieron " << invalidas << " linea(s) danadas.";
    cout << "\n";
}

// =====================================================================
//  8. OPERACIONES DEL HOTEL
// =====================================================================

// Pide los datos del huesped y una habitacion valida; crea la cuenta.
// estadoInicial = CTA_RESERVADA (reservacion) o CTA_ABIERTA (llega sin reserva).
// Devuelve true si se pudo crear.
bool crearEstadia(Hotel& h, int estadoInicial) {
    if (h.totalCuentas >= MAX_CUENTAS) {
        cout << "Error: se alcanzo el maximo de " << MAX_CUENTAS << " cuentas.\n";
        return false;
    }
    string documento = leerDocumento("DPI del huesped (13 digitos): ");
    int vigente = buscarCuentaVigentePorDocumento(h, documento);
    if (vigente != -1) {
        cout << "Error: ese huesped ya tiene la cuenta " << h.cuentas[vigente].codigo
             << " (" << NOMBRE_ESTADO_CTA[h.cuentas[vigente].estado] << ") en la habitacion "
             << h.cuentas[vigente].habitacion << ". No se permiten dos estadias a la vez.\n";
        return false;
    }
    string nombre = leerNombre("Nombre del huesped: ");

    int tipo = elegirTipo();
    if (mostrarDisponiblesPorTipo(h, tipo) == 0) {
        cout << "No hay habitaciones de ese tipo disponibles.\n";
        return false;
    }
    int numero = leerEntero("Numero de habitacion: ", 101, PISOS * 100 + HABS_POR_PISO);
    Habitacion* hab = buscarHabitacion(h, numero);
    if (hab == nullptr) {
        cout << "Error: la habitacion " << numero << " no existe.\n";
        return false;
    }
    if (hab->tipo != tipo) {
        cout << "Error: la habitacion " << numero << " es " << NOMBRE_TIPO[hab->tipo]
             << ", no " << NOMBRE_TIPO[tipo] << ".\n";
        return false;
    }
    if (hab->estado != DISPONIBLE) {   // estados excluyentes: solo se asigna si esta libre
        cout << "Error: la habitacion " << numero << " esta " << NOMBRE_ESTADO[hab->estado] << ".\n";
        return false;
    }
    int personas = leerEntero("Numero de personas: ", 1, 10);
    if (personas > CAPACIDAD[hab->tipo]) {
        cout << "Error: una habitacion " << NOMBRE_TIPO[hab->tipo] << " admite maximo "
             << CAPACIDAD[hab->tipo] << " persona(s).\n";
        return false;
    }
    int noches = leerEntero("Noches de estadia (1-" + to_string(MAX_NOCHES) + "): ", 1, MAX_NOCHES);

    int nuevoEstadoHab = (estadoInicial == CTA_RESERVADA) ? RESERVADA : OCUPADA;
    if (!cambiarEstadoHabitacion(*hab, nuevoEstadoHab)) return false;

    Cuenta c;
    c.codigo        = h.siguienteCodigo++;
    c.nombre        = nombre;
    c.documento     = documento;
    c.personas      = personas;
    c.habitacion    = numero;
    c.noches        = noches;
    c.estado        = estadoInicial;
    c.totalConsumos = 0.0;
    c.metodoPago    = 0;
    c.totalPagado   = 0.0;
    h.cuentas[h.totalCuentas++] = c;
    hab->cuentaActual = c.codigo;

    cout << "\nListo. Cuenta " << c.codigo << " creada para " << nombre << " en la habitacion "
         << numero << " (" << NOMBRE_ESTADO[hab->estado] << ").\n";
    cout << "Hospedaje estimado: " << noches << " noche(s) x Q" << fixed << setprecision(2)
         << TARIFA_NOCHE[hab->tipo] << " = Q" << noches * TARIFA_NOCHE[hab->tipo] << "\n";
    return true;
}

void reservar(Hotel& h) {
    cout << "\n=== NUEVA RESERVACION ===\n";
    if (crearEstadia(h, CTA_RESERVADA)) guardarTodo(h);
}

void checkIn(Hotel& h) {
    cout << "\n=== CHECK-IN ===\n1. Huesped con reservacion\n2. Huesped sin reservacion\n0. Cancelar\n";
    int op = leerEntero("Opcion: ", 0, 2);
    if (op == 2) {
        if (crearEstadia(h, CTA_ABIERTA)) guardarTodo(h);
        return;
    }
    if (op != 1) return;

    int pos = buscarCuentaPorCodigo(h, leerEntero("Codigo de la reservacion: ", 1, 999999));
    if (pos == -1) {
        cout << "Error: no existe esa reservacion.\n";
        return;
    }
    Cuenta& c = h.cuentas[pos];
    if (c.estado != CTA_RESERVADA) {
        cout << "Error: la cuenta " << c.codigo << " esta " << NOMBRE_ESTADO_CTA[c.estado]
             << ", no es una reservacion pendiente.\n";
        return;
    }
    Habitacion* hab = buscarHabitacion(h, c.habitacion);
    if (hab == nullptr || !cambiarEstadoHabitacion(*hab, OCUPADA)) return;
    c.estado = CTA_ABIERTA;
    cout << "Bienvenido(a) " << c.nombre << ". Habitacion " << c.habitacion << " ocupada.\n";
    guardarTodo(h);
}

void cancelarReservacion(Hotel& h) {
    cout << "\n=== CANCELAR RESERVACION ===\n";
    int pos = buscarCuentaPorCodigo(h, leerEntero("Codigo de la reservacion: ", 1, 999999));
    if (pos == -1) {
        cout << "Error: no existe esa reservacion.\n";
        return;
    }
    Cuenta& c = h.cuentas[pos];
    if (c.estado != CTA_RESERVADA) {
        cout << "Error: solo se cancelan reservaciones pendientes (esta cuenta esta "
             << NOMBRE_ESTADO_CTA[c.estado] << ").\n";
        return;
    }
    if (!confirmar("Cancelar la reservacion de " + c.nombre)) return;
    Habitacion* hab = buscarHabitacion(h, c.habitacion);
    if (hab == nullptr || !cambiarEstadoHabitacion(*hab, DISPONIBLE)) return;
    hab->cuentaActual = 0;
    c.estado = CTA_CANCELADA;
    cout << "Reservacion cancelada. Habitacion " << c.habitacion << " disponible.\n";
    guardarTodo(h);
}

// Busca la cuenta ABIERTA de una habitacion ocupada. Devuelve la posicion o -1.
int cuentaAbiertaDeHabitacion(Hotel& h, int numero) {
    Habitacion* hab = buscarHabitacion(h, numero);
    if (hab == nullptr) {
        cout << "Error: la habitacion " << numero << " no existe.\n";
        return -1;
    }
    if (hab->estado != OCUPADA) {
        cout << "Error: la habitacion " << numero << " no esta ocupada ("
             << NOMBRE_ESTADO[hab->estado] << ").\n";
        return -1;
    }
    int pos = buscarCuentaPorCodigo(h, hab->cuentaActual);
    if (pos == -1 || h.cuentas[pos].estado != CTA_ABIERTA) {
        cout << "Error: la habitacion no tiene una cuenta abierta valida.\n";
        return -1;
    }
    return pos;
}

void registrarConsumo(Hotel& h) {
    cout << "\n=== REGISTRAR CONSUMO ===\n";
    if (h.totalConsumos >= MAX_CONSUMOS) {
        cout << "Error: se alcanzo el maximo de consumos registrados.\n";
        return;
    }
    int pos = cuentaAbiertaDeHabitacion(h, leerEntero("Habitacion: ", 101, PISOS * 100 + HABS_POR_PISO));
    if (pos == -1) return;
    Cuenta& c = h.cuentas[pos];
    cout << "Huesped: " << c.nombre << " (cuenta " << c.codigo << ")\nServicios:\n";
    for (int s = 0; s < NUM_SERVICIOS; s++) cout << "  " << (s + 1) << ". " << NOMBRE_SERVICIO[s] << "\n";

    Consumo x;
    x.codigoCuenta = c.codigo;
    x.servicio     = leerEntero("Servicio: ", 1, NUM_SERVICIOS) - 1;
    x.descripcion  = leerTexto("Descripcion (ej. Desayuno, Agua, Planchado): ");
    x.monto        = leerDecimal("Monto Q (0.01-5000): ", 0.01, MAX_MONTO_CONSUMO);

    h.consumos[h.totalConsumos++] = x;
    c.totalConsumos += x.monto;                       // acumulador de la cuenta
    cout << "Consumo cargado. Consumos acumulados de la cuenta: Q" << fixed << setprecision(2)
         << c.totalConsumos << "\n";
    guardarTodo(h);
}

// Imprime la boleta en cualquier salida (pantalla o archivo).
void imprimirBoleta(Hotel& h, const Cuenta& c, ostream& out) {
    Habitacion* hab = buscarHabitacion(h, c.habitacion);
    double tarifa = (hab != nullptr) ? TARIFA_NOCHE[hab->tipo] : 0.0;
    out << "\n========== BOLETA - HOTEL ==========\n"
        << "Cuenta    : " << c.codigo << "\n"
        << "Huesped   : " << c.nombre << "   DPI: " << c.documento << "\n"
        << "Habitacion: " << c.habitacion << " ("
        << (hab ? NOMBRE_TIPO[hab->tipo] : string("?")) << ")\n"
        << "------------------------------------\n" << fixed << setprecision(2)
        << "Hospedaje " << c.noches << " noche(s) x Q" << tarifa << " = Q" << c.noches * tarifa << "\n";
    for (int i = 0; i < h.totalConsumos; i++) {
        if (h.consumos[i].codigoCuenta == c.codigo) {
            out << "  " << left << setw(12) << NOMBRE_SERVICIO[h.consumos[i].servicio]
                << setw(20) << h.consumos[i].descripcion.substr(0, 19)
                << " Q" << right << setw(9) << h.consumos[i].monto << "\n";
        }
    }
    out << "Total consumos: Q" << c.totalConsumos << "\n"
        << "------------------------------------\n"
        << "TOTAL A PAGAR : Q" << calcularTotalCuenta(h, c) << "\n";
    if (c.estado == CTA_CERRADA) out << "Pagado con    : " << NOMBRE_PAGO[c.metodoPago] << "\n";
    out << "====================================\n";
}

// CIERRE DE CUENTA (check-out): cobra, emite boleta y manda a limpieza.
void checkOut(Hotel& h) {
    cout << "\n=== CHECK-OUT Y CIERRE DE CUENTA ===\n";
    int pos = cuentaAbiertaDeHabitacion(h, leerEntero("Habitacion: ", 101, PISOS * 100 + HABS_POR_PISO));
    if (pos == -1) return;
    Cuenta& c = h.cuentas[pos];
    imprimirBoleta(h, c, cout);
    double total = calcularTotalCuenta(h, c);

    cout << "\nMetodo de pago:\n1. Efectivo\n2. Tarjeta\n0. Cancelar\n";
    int metodo = leerEntero("Opcion: ", 0, 2);
    if (metodo == 0) {
        cout << "Check-out cancelado, la cuenta sigue abierta.\n";
        return;
    }
    if (metodo == 1) {
        double recibido = leerDecimal("Efectivo recibido Q: ", total, 1000000.0);
        cout << "Vuelto: Q" << fixed << setprecision(2) << recibido - total << "\n";
    }

    Habitacion* hab = buscarHabitacion(h, c.habitacion);
    if (hab == nullptr || !cambiarEstadoHabitacion(*hab, LIMPIEZA)) return;
    hab->cuentaActual = 0;
    hab->ingresos    += total;            // acumulador por habitacion
    c.estado      = CTA_CERRADA;
    c.metodoPago  = metodo;
    c.totalPagado = total;

    ofstream boletas(ARCHIVO_BOLETAS, ios::app);   // se agregan al final del archivo
    if (boletas) imprimirBoleta(h, c, boletas);
    cout << "Cuenta cerrada. Gracias por su visita. La habitacion " << c.habitacion
         << " pasa a Limpieza.\n";
    guardarTodo(h);
}

// Limpieza y mantenimiento (estados controlados por la matriz de transiciones).
void gestionarHabitacion(Hotel& h) {
    cout << "\n=== LIMPIEZA Y MANTENIMIENTO ===\n"
         << "1. Terminar limpieza (Limpieza -> Disponible)\n"
         << "2. Enviar a mantenimiento\n"
         << "3. Terminar mantenimiento (Mantenimiento -> Disponible)\n0. Regresar\n";
    int op = leerEntero("Opcion: ", 0, 3);
    if (op == 0) return;

    Habitacion* hab = buscarHabitacion(h, leerEntero("Habitacion: ", 101, PISOS * 100 + HABS_POR_PISO));
    if (hab == nullptr) {
        cout << "Error: esa habitacion no existe.\n";
        return;
    }
    bool ok = false;
    if (op == 1) {
        if (hab->estado != LIMPIEZA) cout << "  La habitacion no esta en Limpieza.\n";
        else ok = cambiarEstadoHabitacion(*hab, DISPONIBLE);
    } else if (op == 2) {
        ok = cambiarEstadoHabitacion(*hab, MANTENIMIENTO);
        if (ok) hab->mantenimientos++;                // contador
    } else {
        if (hab->estado != MANTENIMIENTO) cout << "  La habitacion no esta en Mantenimiento.\n";
        else ok = cambiarEstadoHabitacion(*hab, DISPONIBLE);
    }
    if (ok) {
        cout << "Habitacion " << hab->numero << " ahora esta " << NOMBRE_ESTADO[hab->estado] << ".\n";
        guardarTodo(h);
    }
}

// =====================================================================
//  9. ORDENAMIENTOS (manuales, sin std::sort)
// =====================================================================

// BURBUJA: ranking de habitaciones por ingresos (mayor a menor).
// Primero copia la matriz a un arreglo de una dimension.
void rankingHabitacionesPorIngresos(const Hotel& h) {
    Habitacion lista[TOTAL_HABS];
    int n = 0;
    for (int p = 0; p < PISOS; p++)
        for (int c = 0; c < HABS_POR_PISO; c++)
            lista[n++] = h.habs[p][c];

    for (int pasada = 0; pasada < n - 1; pasada++) {
        bool huboCambio = false;
        for (int j = 0; j < n - 1 - pasada; j++) {
            // desordenado si el de la izquierda gana menos; si empatan, numero menor primero
            bool desordenado = lista[j].ingresos < lista[j + 1].ingresos ||
                               (lista[j].ingresos == lista[j + 1].ingresos &&
                                lista[j].numero > lista[j + 1].numero);
            if (desordenado) {
                Habitacion tmp = lista[j];
                lista[j] = lista[j + 1];
                lista[j + 1] = tmp;
                huboCambio = true;
            }
        }
        if (!huboCambio) break;
    }
    cout << "\n--- RANKING DE HABITACIONES POR INGRESOS (burbuja) ---\n";
    imprimirEncabezadoHab();
    for (int i = 0; i < n; i++) imprimirHabitacion(lista[i]);
}

// Regla de prioridad para ordenar cuentas: mayor total primero;
// si empatan, la de codigo menor (la mas antigua) primero.
bool cuentaVaAntes(Hotel& h, const Cuenta& a, const Cuenta& b) {
    double ta = calcularTotalCuenta(h, a), tb = calcularTotalCuenta(h, b);
    if (ta != tb) return ta > tb;
    return a.codigo < b.codigo;
}

// SELECCION: cuentas ordenadas por total (no se altera el arreglo original).
void cuentasOrdenadasPorTotal(Hotel& h) {
    if (h.totalCuentas == 0) {
        cout << "No hay cuentas registradas.\n";
        return;
    }
    Cuenta copia[MAX_CUENTAS];
    for (int i = 0; i < h.totalCuentas; i++) copia[i] = h.cuentas[i];
    int n = h.totalCuentas;

    for (int i = 0; i < n - 1; i++) {
        int mejor = i;
        for (int j = i + 1; j < n; j++) {
            if (cuentaVaAntes(h, copia[j], copia[mejor])) mejor = j;
        }
        if (mejor != i) {
            Cuenta tmp = copia[i];
            copia[i] = copia[mejor];
            copia[mejor] = tmp;
        }
    }
    cout << "\n--- CUENTAS ORDENADAS POR TOTAL (seleccion) ---\n";
    cout << left << setw(7) << "Cuenta" << setw(22) << "Huesped" << setw(6) << "Hab."
         << setw(11) << "Estado" << right << setw(12) << "Total Q" << "\n" << string(58, '-') << "\n";
    for (int i = 0; i < n; i++) {
        cout << left << setw(7) << copia[i].codigo << setw(22) << copia[i].nombre.substr(0, 21)
             << setw(6) << copia[i].habitacion << setw(11) << NOMBRE_ESTADO_CTA[copia[i].estado]
             << right << setw(12) << fixed << setprecision(2) << calcularTotalCuenta(h, copia[i]) << "\n";
    }
}

// =====================================================================
//  10. REPORTES (contadores y acumuladores)
// =====================================================================

// Reporte 1: ocupacion general y por piso.
void reporteOcupacion(const Hotel& h, ostream& out) {
    int porEstado[NUM_ESTADOS] = {0};
    out << "\n--- REPORTE 1: OCUPACION ---\n";
    for (int p = 0; p < PISOS; p++) {
        int ocupadasPiso = 0;
        for (int c = 0; c < HABS_POR_PISO; c++) {
            porEstado[h.habs[p][c].estado]++;
            if (h.habs[p][c].estado == OCUPADA) ocupadasPiso++;
        }
        out << "Piso " << (p + 1) << ": " << ocupadasPiso << "/" << HABS_POR_PISO << " ocupadas\n";
    }
    for (int e = 0; e < NUM_ESTADOS; e++) {
        out << left << setw(15) << NOMBRE_ESTADO[e] << ": " << porEstado[e] << "\n";
    }
    out << "Porcentaje de ocupacion: " << fixed << setprecision(1)
        << porEstado[OCUPADA] * 100.0 / TOTAL_HABS << "%\n";
}

// Reporte 2: ingresos por hospedaje y por servicio.
void reporteIngresos(Hotel& h, ostream& out) {
    double porServicio[NUM_SERVICIOS] = {0.0};
    double hospedajeCobrado = 0.0, totalCobrado = 0.0, pendiente = 0.0;
    int cerradas = 0, efectivo = 0, tarjeta = 0;

    for (int i = 0; i < h.totalConsumos; i++) {
        porServicio[h.consumos[i].servicio] += h.consumos[i].monto;
    }
    for (int i = 0; i < h.totalCuentas; i++) {
        const Cuenta& c = h.cuentas[i];
        if (c.estado == CTA_CERRADA) {
            cerradas++;
            totalCobrado     += c.totalPagado;
            hospedajeCobrado += c.totalPagado - c.totalConsumos;
            if (c.metodoPago == 1) efectivo++;
            if (c.metodoPago == 2) tarjeta++;
        } else if (c.estado == CTA_ABIERTA) {
            pendiente += calcularTotalCuenta(h, c);
        }
    }
    out << "\n--- REPORTE 2: INGRESOS ---\n" << fixed << setprecision(2);
    out << "Cuentas cerradas       : " << cerradas << " (efectivo " << efectivo
        << ", tarjeta " << tarjeta << ")\n";
    out << "Cobrado por hospedaje  : Q" << hospedajeCobrado << "\n";
    out << "Consumos por servicio (todas las cuentas):\n";
    for (int s = 0; s < NUM_SERVICIOS; s++) {
        out << "   " << left << setw(12) << NOMBRE_SERVICIO[s] << ": Q" << porServicio[s] << "\n";
    }
    out << "TOTAL COBRADO          : Q" << totalCobrado << "\n";
    out << "Pendiente (abiertas)   : Q" << pendiente << "\n";
    if (cerradas > 0) out << "Promedio por cuenta    : Q" << totalCobrado / cerradas << "\n";
}

// Reporte 3: huespedes hospedados ahora y reservaciones pendientes.
void reporteCuentasVigentes(Hotel& h, ostream& out) {
    out << "\n--- REPORTE 3: HUESPEDES Y RESERVACIONES VIGENTES ---\n";
    int abiertas = 0, reservadas = 0;
    for (int i = 0; i < h.totalCuentas; i++) {
        const Cuenta& c = h.cuentas[i];
        if (c.estado == CTA_ABIERTA || c.estado == CTA_RESERVADA) {
            out << "  [" << c.codigo << "] Hab. " << c.habitacion << "  " << left << setw(22)
                << c.nombre.substr(0, 21) << setw(10) << NOMBRE_ESTADO_CTA[c.estado]
                << " saldo Q" << fixed << setprecision(2) << calcularTotalCuenta(h, c) << "\n";
            if (c.estado == CTA_ABIERTA) abiertas++;
            else reservadas++;
        }
    }
    out << "Hospedados: " << abiertas << "   Reservaciones pendientes: " << reservadas << "\n";
}

// Reporte 4: limpieza y mantenimiento.
void reporteMantenimiento(const Hotel& h, ostream& out) {
    out << "\n--- REPORTE 4: LIMPIEZA Y MANTENIMIENTO ---\n";
    int pendientes = 0, totalMant = 0;
    for (int p = 0; p < PISOS; p++) {
        for (int c = 0; c < HABS_POR_PISO; c++) {
            const Habitacion& hab = h.habs[p][c];
            totalMant += hab.mantenimientos;
            if (hab.estado == LIMPIEZA || hab.estado == MANTENIMIENTO) {
                out << "  Hab. " << hab.numero << " -> " << NOMBRE_ESTADO[hab.estado] << "\n";
                pendientes++;
            }
        }
    }
    if (pendientes == 0) out << "  Ninguna habitacion fuera de servicio.\n";
    out << "Fuera de servicio ahora: " << pendientes
        << "   Mantenimientos realizados en total: " << totalMant << "\n";
}

void exportarReportes(Hotel& h) {
    ofstream out(ARCHIVO_REPORTES);
    if (!out) {
        cout << "Error: no se pudo crear el archivo de reportes.\n";
        return;
    }
    out << "REPORTES DEL HOTEL\n";
    reporteOcupacion(h, out);
    reporteIngresos(h, out);
    reporteCuentasVigentes(h, out);
    reporteMantenimiento(h, out);
    cout << "Reportes guardados en " << ARCHIVO_REPORTES << "\n";
}

// =====================================================================
//  11. MENUS
// =====================================================================
void menuConsultas(Hotel& h) {
    int resultados[MAX_CUENTAS];
    int op;
    do {
        cout << "\n--- CONSULTAS ---\n"
             << "1. Mapa de ocupacion\n2. Buscar habitacion por numero\n"
             << "3. Habitaciones por estado\n4. Buscar huesped por nombre\n"
             << "5. Buscar huesped por DPI\n6. Ver detalle de una cuenta\n"
             << "7. Listar todas las cuentas\n0. Regresar\n";
        op = leerEntero("Opcion: ", 0, 7);
        if (op == 1) {
            imprimirMapa(h);
        } else if (op == 2) {
            Habitacion* hab = buscarHabitacion(h, leerEntero("Habitacion: ", 101, PISOS * 100 + HABS_POR_PISO));
            if (hab == nullptr) cout << "Esa habitacion no existe.\n";
            else { imprimirEncabezadoHab(); imprimirHabitacion(*hab); }
        } else if (op == 3) {
            for (int e = 0; e < NUM_ESTADOS; e++) cout << "  " << (e + 1) << ". " << NOMBRE_ESTADO[e] << "\n";
            int estado = leerEntero("Estado: ", 1, NUM_ESTADOS) - 1;
            int n = 0;
            imprimirEncabezadoHab();
            for (int p = 0; p < PISOS; p++)
                for (int c = 0; c < HABS_POR_PISO; c++)
                    if (h.habs[p][c].estado == estado) { imprimirHabitacion(h.habs[p][c]); n++; }
            cout << n << " habitacion(es).\n";
        } else if (op == 4) {
            int n = buscarCuentasPorNombre(h, leerTexto("Nombre o parte del nombre: "), resultados);
            if (n == 0) cout << "No se encontraron huespedes.\n";
            else {
                imprimirEncabezadoCuenta();
                for (int i = 0; i < n; i++) imprimirCuenta(h.cuentas[resultados[i]]);
            }
        } else if (op == 5) {
            string doc = leerDocumento("DPI: ");
            int n = 0;
            for (int i = 0; i < h.totalCuentas; i++) {
                if (h.cuentas[i].documento == doc) {
                    if (n == 0) imprimirEncabezadoCuenta();
                    imprimirCuenta(h.cuentas[i]);
                    n++;
                }
            }
            if (n == 0) cout << "No hay estadias con ese DPI.\n";
        } else if (op == 6) {
            int pos = buscarCuentaPorCodigo(h, leerEntero("Codigo de cuenta: ", 1, 999999));
            if (pos == -1) cout << "Esa cuenta no existe.\n";
            else imprimirBoleta(h, h.cuentas[pos], cout);
        } else if (op == 7) {
            if (h.totalCuentas == 0) cout << "No hay cuentas.\n";
            else {
                imprimirEncabezadoCuenta();
                for (int i = 0; i < h.totalCuentas; i++) imprimirCuenta(h.cuentas[i]);
            }
        }
    } while (op != 0);
}

void menuReportes(Hotel& h) {
    int op;
    do {
        cout << "\n--- REPORTES ---\n"
             << "1. Ocupacion\n2. Ingresos\n3. Huespedes y reservaciones vigentes\n"
             << "4. Limpieza y mantenimiento\n5. Ranking de habitaciones por ingresos\n"
             << "6. Cuentas ordenadas por total\n7. Exportar reportes a archivo\n0. Regresar\n";
        op = leerEntero("Opcion: ", 0, 7);
        switch (op) {
            case 1: reporteOcupacion(h, cout); break;
            case 2: reporteIngresos(h, cout); break;
            case 3: reporteCuentasVigentes(h, cout); break;
            case 4: reporteMantenimiento(h, cout); break;
            case 5: rankingHabitacionesPorIngresos(h); break;
            case 6: cuentasOrdenadasPorTotal(h); break;
            case 7: exportarReportes(h); break;
        }
    } while (op != 0);
}

void mostrarMenuPrincipal(const Hotel& h) {
    int ocupadas = 0;
    for (int p = 0; p < PISOS; p++)
        for (int c = 0; c < HABS_POR_PISO; c++)
            if (h.habs[p][c].estado == OCUPADA) ocupadas++;

    cout << "\n==================================================\n"
         << "   HOTEL - OCUPACION Y SERVICIOS (Variante B)\n"
         << "   Ocupadas: " << ocupadas << "/" << TOTAL_HABS << "\n"
         << "==================================================\n"
         << "1. Nueva reservacion\n"
         << "2. Check-in\n"
         << "3. Registrar consumo\n"
         << "4. Check-out y cierre de cuenta\n"
         << "5. Cancelar reservacion\n"
         << "6. Limpieza y mantenimiento\n"
         << "7. Consultas\n"
         << "8. Reportes\n"
         << "0. Salir\n";
}

// =====================================================================
//  PROGRAMA PRINCIPAL: solo coordina; la logica esta en las funciones.
// =====================================================================
int main() {
    // "static" para que los arreglos grandes no llenen la pila
    static Hotel hotel;
    cargarTodo(hotel);

    int op;
    do {
        mostrarMenuPrincipal(hotel);
        op = leerEntero("Seleccione una opcion: ", 0, 8);
        switch (op) {
            case 1: reservar(hotel); break;
            case 2: checkIn(hotel); break;
            case 3: registrarConsumo(hotel); break;
            case 4: checkOut(hotel); break;
            case 5: cancelarReservacion(hotel); break;
            case 6: gestionarHabitacion(hotel); break;
            case 7: menuConsultas(hotel); break;
            case 8: menuReportes(hotel); break;
            case 0:
                if (guardarTodo(hotel)) cout << "Datos guardados. Hasta luego.\n";
                break;
        }
        if (op >= 1 && op <= 6) pausar();
    } while (op != 0);
    return 0;
}
