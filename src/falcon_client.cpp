
#include <iostream>
#include "../inc/falcon_API.h"
#include <cstring>
#include <array>
#include <utility>


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

std::unique_ptr<FalconClient> FalconClient::ConnectTo(const std::string& ip, uint16_t port) {
    std::string new_ip;
    uint16_t new_port = 0;
    std::tie(new_ip, new_port) = GetClientAddress(ip);

    auto m_falcon = std::make_unique<FalconClient>();
    m_falcon->Connect("127.0.0.1", port);
    if (!m_falcon) {
        std::cerr << "Erreur : Impossible de se connecter à " << ip << ":" << port << std::endl;
        return nullptr;
    }
    m_falcon->StartListening();
    m_falcon->m_ip = new_ip;
    m_falcon->m_port =new_port;

    m_falcon->m_lastPing = std::chrono::steady_clock::now();
    m_falcon->m_lastPong = std::chrono::steady_clock::now();

    uint8_t protocolType = static_cast<uint8_t>(ProtocolType::Connect);
    std::vector<char> connectMessage = { static_cast<char>(protocolType) };

    m_falcon->SendTo(new_ip, new_port, std::span(connectMessage.data(), connectMessage.size()));
    return m_falcon;
}


void FalconClient::Update() {
    // ping the server if the time exceeds 0.1s without receiving any message
    if (std::chrono::steady_clock::now() - m_lastPong > std::chrono::seconds(2) &&
        std::chrono::steady_clock::now() - m_lastPing > std::chrono::seconds(2)) {
        std::cout << "Envoi d'un ping" << std::endl;
        uint8_t protocolType = static_cast<uint8_t>(ProtocolType::Ping);
        std::vector<char> pingMessage = { static_cast<char>(protocolType) };
        SendTo(m_ip, m_port, std::span(pingMessage.data(), pingMessage.size()));
        m_lastPing = std::chrono::steady_clock::now();
    }
    if (std::chrono::steady_clock::now() - m_lastPong > std::chrono::seconds(10)) {
        if (m_disconnectHandler) {
            m_disconnectHandler();
        }
        raise(SIGABRT);
    }

    while (true) {
        auto message = GetNextMessage();
        if (!message.has_value()) break;

        m_lastPong = std::chrono::steady_clock::now();

        std::array<char, 65535> buffer{};
        std::ranges::copy(message->first, buffer.begin());

        uint8_t protocolType = buffer[0];


        switch (static_cast<ProtocolType>(protocolType)) {
            case ProtocolType::Connect:
                HandleConnectMessage(buffer);
            break;

            /*
            case ProtocolType::Acknowledgement:
                HandleAcknowledgementMessage(buffer);
            break;*/

            case ProtocolType::Stream:
                HandleStreamMessage(buffer);
            break;

            case ProtocolType::Ping:
                HandlePing(buffer);
            break;

            case ProtocolType::Pong:
                m_lastPong = std::chrono::steady_clock::now();
            std::cout << "Pong received from " << std::endl;
            break;


            default:
                std::cerr << "Paquet inconnu reçu" << std::endl;
            break;
        }

    }
}

void FalconClient::HandleConnectMessage(const std::array<char, 65535>& buffer) {
    m_clientId = *reinterpret_cast<const UUID*>(&buffer[1]);

    std::cout << "UUID du client : " << m_clientId << std::endl;

    if (m_connectionHandler) {
        m_connectionHandler(true, m_clientId);
    }
}


/*
void FalconClient::HandleAcknowledgementMessage(const std::array<char, 65535>& buffer) {
   
    uint8_t protocolType = *reinterpret_cast<const UUID*>(&buffer[1]);

    switch (protocolType) {
        case ProtocolType::Connect:
            uint64_t clientId = *reinterpret_cast<const UUID*>(&buffer[2]);
            break;
        default:
            std::cerr << "Paquet inconnu reçu Acknowledgement" << std::endl;
    }

    
    std::cout << "Confirmation (Acknowledgement) reçue" << std::endl;

    
}*/


void FalconClient::HandleStreamMessage(const std::array<char, 65535>& buffer) {
    
    uint32_t streamId = *reinterpret_cast<const uint32_t*>(&buffer[1]);  

    
    std::span<const char> data(buffer.data() + sizeof(streamId), buffer.size() - sizeof(streamId));

    std::cout << "Stream ID " << streamId << " reçu. Données: " << data.size() << " bytes." << std::endl;

    
}

void FalconClient::HandlePing(const std::array<char, 65535>& buffer) {

    std::cout << "Ping received " << std::endl;


    uint8_t protocolType = static_cast<uint8_t>(ProtocolType::Pong);
    std::vector<char> pongMessage = { static_cast<char>(protocolType) };
    SendTo(m_ip, m_port, std::span(pongMessage.data(), pongMessage.size()));
}



void FalconClient::OnConnectionEvent(std::function<void(bool, UUID)> handler){
    m_connectionHandler = std::move(handler);
}

void FalconClient::OnDisconnect(std::function<void()> handler){
    m_disconnectHandler = std::move(handler);
}

std::unique_ptr<Stream> FalconClient::CreateStream(bool reliable) {
    Falcon* falcon = static_cast<Falcon*>(this);
    uint32_t streamId = m_nextStreamId++;
    auto stream = std::make_unique<Stream>(*falcon, m_clientId, streamId, reliable, m_ip, m_port);
    m_streams[m_nextStreamId] = std::move(stream);
    return std::move(m_streams[m_nextStreamId]);
}
