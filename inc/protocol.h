#pragma once

enum class ProtocolType : uint8_t {
    Connect = 0x00,     // Demande de connexion
    Acknowledgement = 0x01, // Confirmation de réception
    Reco = 0x02,        // Reconnexion ou état
    Stream = 0x03       // Transmission de données en flux
};
