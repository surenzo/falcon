#pragma once

enum class ProtocolType : uint8_t {
    Connect = 0x00,     // Demande de connexion
    //Acknowledgement = 0x01, // Confirmation de réception
    Stream = 0x03       // Transmission de données en flux
};
