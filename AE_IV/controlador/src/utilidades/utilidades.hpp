#ifndef UTILIDADES_HPP
#define UTILIDADES_HPP

#include "../../controlador.hpp"

// Retorna a URL do portal cativo (http://forno.ae/)
String urlMascara();

// Retorna a string JSON equivalente a s, escapando aspas e barras invertidas.
String jsonEscape(const String& s);

// Aplica a cor no LED RGB nativo do ESP32-S3. Retorna true se a cor for válida.
bool aplicarCor(String nome);

#endif // UTILIDADES_HPP