#pragma once
#include <string>

class Session;
struct Packet;

void Game_BroadcastChat(Session* sender, const std::string& msg);
void Game_OnChat(Session* session, Packet& pkt);
void Game_OnPacket(Session* session, Packet& pkt);
