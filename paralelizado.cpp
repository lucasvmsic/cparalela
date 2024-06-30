#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <ctime>
#include <omp.h>
//FECHAS: 2021-02  -  2024-04
using namespace std;

// Clase para almacenar la información del producto
class Producto {
public:
    string sku;
    float precioMinimo;
    bool encontrado;

    // Constructor por defecto
    Producto() : sku(""), precioMinimo(100000000), encontrado(false) {}

    // Constructor con parámetros
    Producto(string sku, float precioMinimo, bool encontrado)
        : sku(sku), precioMinimo(precioMinimo), encontrado(encontrado) {}
};

// Clase para almacenar los datos de cada fecha y precio PARIDADES
class FechaPrecio {
public:
    string fechaOriginal;
    string fechaYYYYMMDD;
    double precio;

    // Constructor predeterminado
    FechaPrecio() : fechaOriginal(""), fechaYYYYMMDD(""), precio(0.0) {}

    // Constructor con parámetros
    FechaPrecio(const string& f, const string& fYYYYMMDD, double p) : fechaOriginal(f), fechaYYYYMMDD(fYYYYMMDD), precio(p) {}
};

// Función para dividir una línea en campos usando un delimitador
vector<string> splitLine(const string& linea, char delimitador) {
    vector<string> campos;
    string campo;
    bool dentroDeComillas = false;

    for (char c : linea) {
        if (c == '"') {
            dentroDeComillas = !dentroDeComillas;
        } else if (c == delimitador && !dentroDeComillas) {
            campos.push_back(campo);
            campo.clear();
        } else {
            campo += c;
        }
    }
    campos.push_back(campo); // Agregar el último campo

    return campos;
}

bool crearArchivosMensuales(const string& archivoEntrada, map<string, ofstream>& archivosMensuales) {
    ifstream archivo(archivoEntrada); // Abrir archivo de entrada
    string linea;
    char delimitador = ';'; // Delimitador para separar campos en el archivo CSV
    int i = 0;

    // Verificar si se pudo abrir el archivo
    if (!archivo.is_open()) {
        cerr << "No se pudo abrir el archivo " << archivoEntrada << "." << endl;
        return false;
    }

    // Leemos la primer línea para descartarla, ya que es el encabezado


    string acumulador;
    bool acumulando = false;
	getline(archivo, linea);
    // Leemos hasta 2000000 líneas o hasta el final del archivo
    while (getline(archivo, linea)) {
        try {
            if (acumulando) {
                acumulador += "\n" + linea;
            } else {
                acumulador = linea;
            }

            vector<string> campos = splitLine(acumulador, delimitador);

            // Verificar si el número de campos es el esperado (10 en este caso)
            if (campos.size() == 10) {
                acumulando = false; // Línea completa
            } else {
                acumulando = true; // Línea incompleta, seguir acumulando
                continue; // Leer la siguiente línea
            }

            // Procesar la línea completa
            string fecha = campos[0];
            string idProducto = campos[6];
            string precioStr = campos[9];

            // Eliminar comillas
            if (fecha.size() >= 2 && fecha.front() == '"' && fecha.back() == '"') {
                fecha = fecha.substr(1, fecha.size() - 2);
            }
            if (idProducto.size() >= 2 && idProducto.front() == '"' && idProducto.back() == '"') {
                idProducto = idProducto.substr(1, idProducto.size() - 2);
            }
            if (precioStr.size() >= 2 && precioStr.front() == '"' && precioStr.back() == '"') {
                precioStr = precioStr.substr(1, precioStr.size() - 2);
            }

            // Verificar si los campos están vacíos
            if (fecha.empty() || idProducto.empty() || precioStr.empty()) {
              //  cerr << "Error: Campo vacío encontrado en la línea " << i + 1 << endl;
               // cout<<campos[0]<<" "<<campos[6]<<" "<<campos[9]<<" "<<i<<endl;
                continue; // Saltar esta línea si algún campo está vacío
            }

            // Convertir el precio a float
            float precio;
            try {
                precio = stof(precioStr);
            } catch (const invalid_argument& e) {
                cerr << "Error: El campo precio no es un número válido en la línea " << i + 1 << ": " << precioStr << endl;
                continue; // Saltar esta línea si el precio no es válido
            }
//2021-12-01 15:21:06.602  0.00 8343411
            // Extraer el mes y año de la fecha (ISO extendido)
            string mesAno = fecha.substr(0, 7); // YYYY-MM

            // Crear archivo para el mes si no existe
            if (archivosMensuales.find(mesAno) == archivosMensuales.end()) {
                archivosMensuales[mesAno].open(mesAno + ".csv");
            }

            // Escribir la línea en el archivo correspondiente
            archivosMensuales[mesAno] << fecha << ";" << idProducto << ";" << precio << endl;

            i++; // Incrementar contador de líneas leídas
        } catch (const exception& e) {
            cerr << "Error al procesar la línea " << i + 1 << ": " << e.what() << endl;
            continue; // Saltar esta línea si ocurre un error
        }
    }

    archivo.close(); // Cerrar archivo de entrada

    // Cerrar todos los archivos mensuales
    for (auto& [mesAno, archivoSalida] : archivosMensuales) {
        archivoSalida.close();
    }

    cout << "Se han procesado " << i << " líneas de " << archivoEntrada << " y generado archivos mensuales." << endl;
    return true;
}

bool procesarArchivoMensual(const string& nombreArchivo, map<string, Producto>& productos) {
    ifstream archivo(nombreArchivo);
    string linea;
    char delimitador = ';';

    // Verificar si se pudo abrir el archivo
    if (!archivo.is_open()) {
        cerr << "No se pudo abrir el archivo " << nombreArchivo << "." << endl;
        return false;
    }

    // Leemos la primer línea para descartarla, ya que es el encabezado
    //getline(archivo, linea);
	getline(archivo, linea);
    while (getline(archivo, linea)) {
        try {
            vector<string> campos = splitLine(linea, delimitador);

            string fecha = campos[0];
            string idProducto = campos[1];
            string precioStr = campos[2];

            // Convertir el precio a float
            float precio;
            try {
                precio = stof(precioStr);
            } catch (const invalid_argument& e) {
                cerr << "Error: El campo precio no es un número válido: " << precioStr << endl;
                continue; // Saltar esta línea si el precio no es válido
            }

            // Actualizar el map con el precio mínimo para cada SKU
            if (productos.find(idProducto) == productos.end()) {
                productos[idProducto] = Producto(idProducto, precio, true);
            } else {
                if (precio < productos[idProducto].precioMinimo) {
                    productos[idProducto].precioMinimo = precio;
                }
                productos[idProducto].encontrado = true;
            }
        } catch (const exception& e) {
            cerr << "Error al procesar la línea: " << e.what() << endl;
            continue; // Saltar esta línea si ocurre un error
        }
    }

    archivo.close(); // Cerrar archivo de entrada
    return true;
}

int main() {
    map<string, Producto> productosMesActual;
    map<string, Producto> productosMesAnterior;

  vector<string> meses = {"2021-02", "2021-03", "2021-04", "2021-05", "2021-06", "2021-07", "2021-08", "2021-09", "2021-10", "2021-11", "2021-12", "2022-01", "2022-02", "2022-03", "2022-04", "2022-05", "2022-06", "2022-07", "2022-08", "2022-09", "2022-10", "2022-11", "2022-12", "2023-01", "2023-02", "2023-03", "2023-04", "2023-05", "2023-06", "2023-07", "2023-08", "2023-09", "2023-10", "2023-11", "2023-12", "2024-01", "2024-02", "2024-03", "2024-04"};
    map<string, ofstream> archivosMensuales;
    
            int num_hilos = 12;
    omp_set_num_threads(num_hilos);
    
        ifstream archivo("paridades.csv");
    if (!archivo.is_open()) {
        cerr << "Error al abrir el archivo" << endl;
        return 1;
    }
	double varAcum=0;
    string linea;
    vector<FechaPrecio> registros;
    map<string, FechaPrecio> preciosMinimosPorMes;

    // Ignorar las primeras siete líneas del archivo
    for (int i = 0; i < 7; ++i) {
        if (!getline(archivo, linea)) {
            cerr << "Error al leer las primeras líneas" << endl;
            return 1;
        }
    }

std::clock_t start = std::clock();

    // Leer el archivo línea por línea
    while (getline(archivo, linea)) {
        stringstream ss(linea);
        string fecha, precioStr, varStr, acumStr;

        getline(ss, fecha, ',');    // Leer campo fecha
        getline(ss, precioStr, ','); // Leer campo precio
        getline(ss, varStr, ',');    // Leer campo var
        getline(ss, acumStr, ',');   // Leer campo acum

        // Eliminar caracteres no deseados del precio
        precioStr.erase(remove(precioStr.begin(), precioStr.end(), '$'), precioStr.end());
        precioStr.erase(remove(precioStr.begin(), precioStr.end(), ' '), precioStr.end());
        precioStr.erase(remove(precioStr.begin(), precioStr.end(), '"'), precioStr.end());

        try {
            double precio = stod(precioStr);

            // Extraer año, mes y día de la fecha
            stringstream ssFecha(fecha);
            string dia, mes, anio;
            getline(ssFecha, dia, '/');
            getline(ssFecha, mes, '/');
            getline(ssFecha, anio, '/');

            // Formatear la fecha como YYYYMMDD
            string fechaYYYYMMDD = anio + (mes.size() == 1 ? "0" + mes : mes) + (dia.size() == 1 ? "0" + dia : dia);

            // Formatear la fecha como YYYY-MM para la clave del mapa
            string anioMes = anio + "-" + (mes.size() == 1 ? "0" + mes : mes);

            // Verificar si ya existe una entrada para ese mes y actualizar si el precio actual es menor
            if (preciosMinimosPorMes.find(anioMes) == preciosMinimosPorMes.end() || preciosMinimosPorMes[anioMes].precio > precio) {
                preciosMinimosPorMes[anioMes] = FechaPrecio(fecha, fechaYYYYMMDD, precio);
            }
        } catch (const invalid_argument& e) {
            cerr << "Error de conversión en la línea: " << linea << endl;
            cerr << "Fecha: " << fecha << ", PrecioStr: " << precioStr << endl;
        }
    }

    // Almacenar los resultados en el vector de registros
    for (const auto& entry : preciosMinimosPorMes) {
        registros.push_back(entry.second);
    }

    // Ordenar los registros por fechaYYYYMMDD
    sort(registros.begin(), registros.end(), [](const FechaPrecio& a, const FechaPrecio& b) {
        return a.fechaYYYYMMDD < b.fechaYYYYMMDD;
    });

    // Imprimir los resultados y calcular la variación porcentual
    cout << "Mes\tFecha\t\tPrecio Mínimo\tVariación Porcentual" << endl;
    double precioAnterior = 0.0;
    for (size_t i = 0; i < registros.size(); ++i) {
        const auto& registro = registros[i];
        double variacionPorcentual = 0.0;

        if (i > 0) {
            variacionPorcentual = ((registro.precio - precioAnterior) / precioAnterior) * 100;
            varAcum+=variacionPorcentual;
        }

        cout << registro.fechaYYYYMMDD.substr(0, 4) + "-" + registro.fechaYYYYMMDD.substr(4, 2) << "\t" << registro.fechaOriginal << "\t" << registro.precio << "\t\t" << (i > 0 ? to_string(variacionPorcentual) + "%" : "N/A") << endl;
        precioAnterior = registro.precio;
        
        if(i%12==0){
        	cout<< "Inflacion interanual: "<<varAcum<<"%"<<endl;
        	varAcum=0;
		}
    }

    archivo.close();
    
    
    // Crear archivos mensuales a partir del archivo original
    if (!crearArchivosMensuales("pd.csv", archivosMensuales)) {
        cerr << "Error al crear los archivos mensuales." << endl;
        return 1;
    }

   float inflacionAcumulada = 0.0;
    float variacionPorcentual = 0.0;
// Procesamiento paralelo con OpenMP
    float sumaPreciosMesActual = 0.0;
    float sumaPreciosMesAnterior = 0.0;
#pragma omp parallel for ordered reduction(+: inflacionAcumulada) private(productosMesActual, productosMesAnterior, sumaPreciosMesActual, sumaPreciosMesAnterior, variacionPorcentual)
for (size_t i = 1; i < meses.size(); ++i) {
    // Procesar el archivo del mes actual y del mes anterior
    if (!procesarArchivoMensual(meses[i] + ".csv", productosMesActual) ||
        !procesarArchivoMensual(meses[i-1] + ".csv", productosMesAnterior)) {
        #pragma omp critical
        {
            cerr << "Error al procesar los archivos mensuales." << endl;
        }
        continue; // Continuar con la siguiente iteración si hay un error
    }

    // Calcular la suma de precios mínimos para ambos meses
    sumaPreciosMesActual = 0.0;
    sumaPreciosMesAnterior = 0.0;

    for (const auto& [sku, producto] : productosMesAnterior) {
        if (productosMesActual.find(sku) != productosMesActual.end()) {
            sumaPreciosMesAnterior += producto.precioMinimo;
            sumaPreciosMesActual += productosMesActual[sku].precioMinimo;
        }
    }

    // Calcular la variación porcentual
    variacionPorcentual = 0.0;
    if (sumaPreciosMesAnterior != 0.0) {
        variacionPorcentual = ((sumaPreciosMesActual - sumaPreciosMesAnterior) / sumaPreciosMesAnterior) * 100;
    }

    // Impresión ordenada usando #pragma omp ordered
    #pragma omp ordered
    {
        cout << "Inflación entre " << meses[i-1] << " y " << meses[i] << ": " << variacionPorcentual << "%" << endl;
        inflacionAcumulada += variacionPorcentual;

        if (i % 12 == 0) {
            cout << "Inflación interanual: " << inflacionAcumulada << "%" << endl;
            inflacionAcumulada = 0; // Reiniciar para el próximo ciclo de 12 meses
        }
    }
}

        std::clock_t end = std::clock();

    // Calcula el tiempo transcurrido
    double elapsed_time = double(end - start) / CLOCKS_PER_SEC;

    // Muestra el resultado
    std::cout << "Tiempo de ejecución: " << elapsed_time << " segundos" << std::endl;
    

    return 0;
}