
#include <iostream>
#include "../inc/falcon_API.h"
#include <cstring>
#include <array>


FalconClient::FalconClient() {
    // call the constructor of the base class
    Falcon();
    // 🔹 Initialiser les streams
    m_streams = std::unordered_map<uint64_t, std::unique_ptr<Stream>>();
}

FalconClient::~FalconClient() {
    // call the destructor of the base class
    Falcon::~Falcon();
}

void FalconClient::ConnectTo(const std::string& ip, uint16_t port) {
    
    std::string new_ip = ip;
    uint16_t new_port = 0;
    auto pos = ip.find_last_of (':');
    if (pos != std::string::npos) {
        new_ip = ip.substr (0,pos);
        std::string port_str = ip.substr (++pos);
        new_port = atoi(port_str.c_str());
    }

    auto m_falcon = Falcon::Connect(new_ip, port);
    if (!m_falcon) {
        std::cerr << "Erreur : Impossible de se connecter à " << ip << ":" << port << std::endl;
        return;
    }

    
    m_isListening = true;
    std::thread listenerThread(&FalconClient::ListenForMessages, this);
    listenerThread.detach();

   
    uint8_t protocolType = static_cast<uint8_t>(ProtocolType::Connect);
    std::vector<char> connectMessage = { static_cast<char>(protocolType) };

  
    m_falcon->SendTo(new_ip, new_port, std::span(connectMessage.data(), connectMessage.size()));

}


/*void FalconClient::ConnectTo(const std::string& ip, uint16_t port) {

    std::string new_ip = ip;
    uint16_t new_port = 0;
    auto pos = ip.find_last_of (':');
    if (pos != std::string::npos) {
        new_ip = ip.substr (0,pos);
        std::string port_str = ip.substr (++pos);
        new_port = atoi(port_str.c_str());
    }

    auto falcon = Falcon::Connect(new_ip, port);
    if (!falcon) {
        std::cerr << "Erreur : Impossible de se connecter à " << ip << ":" << port << std::endl;
        return;
    }

    // 🔥 Étape 1 : Envoyer un paquet de connexion (0x00)
    uint8_t connectionPacket = static_cast<uint8_t>(ProtocolType::Connect);
    falcon->SendTo(new_ip, new_port, std::span {reinterpret_cast<const char*>(&connectionPacket), sizeof(connectionPacket)});


    // 🔥 Étape 2 : Attendre un UUID du serveur
    std::string from_ip;
    from_ip.resize(255);
    std::array<char, 65535> buffer;
    int recv_size = falcon->ReceiveFrom(from_ip, std::span<char, 65535>(buffer));
    printf("Received %d bytes\n", recv_size);

    if (recv_size == sizeof(UUID)) {
        memcpy(&m_clientId, buffer.data(), sizeof(UUID));

        if (m_connectionHandler) {
            m_connectionHandler(true, m_clientId);
            m_ip = new_ip;
            m_port = new_port;
        }
    } else {
        if (m_connectionHandler) {
            m_connectionHandler(false, 0);
        }
    }

}*/

void FalconClient::ListenForMessages() {
    while (m_isListening) {
        std::array<char, 65535> buffer;
        std::string from_ip;
        from_ip.resize(255);

        
        int recv_size = m_falcon->ReceiveFrom(from_ip, std::span<char, 65535>(buffer));
        if (recv_size <= 0) continue;

        
        uint8_t protocolType = buffer.data()[0];

        
        switch (static_cast<ProtocolType>(protocolType)) {
            case ProtocolType::Connect:
                HandleConnectMessage(buffer);
                break;

            case ProtocolType::Acknowledgement:
                HandleAcknowledgementMessage(buffer);
                break;

            case ProtocolType::Reco:
                HandleRecoMessage(buffer);
                break;

            case ProtocolType::Stream:
                HandleStreamMessage(buffer);
                break;

            default:
                std::cerr << "Paquet inconnu reçu" << std::endl;
                break;
        }
    }
}

void FalconClient::HandleConnectMessage(const std::array<char, 65535>& buffer) {
    std::cout << "Message de connexion reçu." << std::endl;


    UUID clientId = *reinterpret_cast<const UUID*>(&buffer[1]);

    std::cout << "UUID du client : " << clientId << std::endl;

    if (m_connectionHandler) {
        m_connectionHandler(true, clientId);
    }
}


void FalconClient::HandleAcknowledgementMessage(const std::array<char, 65535>& buffer) {
   
    UUID clientId = *reinterpret_cast<const UUID*>(&buffer[1]);

    
    std::cout << "Confirmation (Acknowledgement) reçue pour le client avec UUID: " << clientId << std::endl;

    
}

void FalconClient::HandleRecoMessage(const std::array<char, 65535>& buffer) {
    
    UUID clientId = *reinterpret_cast<const UUID*>(&buffer[1]);

    
    std::cout << "Reconnexion demandée pour le client avec UUID: " << clientId << std::endl;

}


void FalconClient::HandleStreamMessage(const std::array<char, 65535>& buffer) {
    
    uint32_t streamId = *reinterpret_cast<const uint32_t*>(&buffer[1]);  

    
    std::span<const char> data(buffer.data() + sizeof(streamId), buffer.size() - sizeof(streamId));

    std::cout << "Stream ID " << streamId << " reçu. Données: " << data.size() << " bytes." << std::endl;

    
}



void FalconClient::OnConnectionEvent(std::function<void(bool, UUID)> handler){
    m_connectionHandler = handler;
}

void FalconClient::OnDisconnect(std::function<void()> handler){
    m_disconnectHandler = handler;
}

std::unique_ptr<Stream> FalconClient::CreateStream(bool reliable) {
    Falcon* falcon = static_cast<Falcon*>(this);
    uint32_t streamId = m_nextStreamId++;
    auto stream = std::make_unique<Stream>(*falcon, m_clientId, streamId, reliable, m_ip, m_port);
    m_streams[m_nextStreamId] = std::move(stream);
    return std::move(m_streams[m_nextStreamId]);
}

void FalconClient::StopListening() {
    m_isListening = false;
}
