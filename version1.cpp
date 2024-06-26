#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <iomanip>
#include <cmath>
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
                cerr << "Error: Campo vacío encontrado en la línea " << i + 1 << endl;
                cout<<campos[0]<<" "<<campos[6]<<" "<<campos[9]<<" "<<i<<endl;
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

    vector<string> meses = {"2021-02", "2021-03", "2021-04", "2021-05", "2021-06", "2021-07", "2021-08", "2021-09", "2021-10", "2021-11", "2021-12", "2022-01", "2022-02", "2022-03", "2022-04", "2022-05", "2022-06", "2022-07", "2022-08", "2022-09", "2022-10", "2022-11", "2022-12", "2023-01", "2023-02", "2023-03", "2023-04"};
    map<string, ofstream> archivosMensuales;

    // Crear archivos mensuales a partir del archivo original
    if (!crearArchivosMensuales("pd.csv", archivosMensuales)) {
        cerr << "Error al crear los archivos mensuales." << endl;
        return 1;
    }

    float inflacionAcumulada = 0.0;

    for (size_t i = 1; i < meses.size(); ++i) {
        productosMesActual.clear();
        productosMesAnterior.clear();

        // Procesar el archivo del mes actual y del mes anterior
        if (!procesarArchivoMensual(meses[i] + ".csv", productosMesActual) ||
            !procesarArchivoMensual(meses[i-1] + ".csv", productosMesAnterior)) {
            cerr << "Error al procesar los archivos mensuales." << endl;
            return 1;
        }

        float sumaPreciosMesActual = 0.0;
        float sumaPreciosMesAnterior = 0.0;

        // Sumar los precios mínimos de los productos que existen en ambos meses
        for (const auto& [sku, producto] : productosMesAnterior) {
            if (productosMesActual.find(sku) != productosMesActual.end()) {
                sumaPreciosMesAnterior += producto.precioMinimo;
                sumaPreciosMesActual += productosMesActual[sku].precioMinimo;
            }
        }

        // Calcular la variación porcentual
        float variacionPorcentual = ((sumaPreciosMesActual - sumaPreciosMesAnterior) / sumaPreciosMesAnterior) * 100;
        inflacionAcumulada += variacionPorcentual;

        cout << "Inflación entre " << meses[i-1] << " y " << meses[i] << ": " << variacionPorcentual << "%" << endl;
        
        if(i%12==0 ){
        	cout << "Inflación interanual: " << inflacionAcumulada << "%" << endl;
        	inflacionAcumulada=0;
		}
        
    }

    

    return 0;
}