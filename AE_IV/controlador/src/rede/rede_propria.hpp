#ifndef REDE_PROPRIA_HPP
#define REDE_PROPRIA_HPP

#include "../../rede.hpp"

// Inicia o mDNS (hostname.local) e o servidor web (WebServer)
void iniciarMDNS();

// Inicia a rede própria (modo AP) e o servidor DNS (DNSServer)
void iniciarAP();

// Fecha a rede própria (modo AP) e o servidor DNS (DNSServer)
void fecharAP();

#endif // REDE_PROPRIA_HPP