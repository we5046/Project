#pragma once
class Session;

void Network_Broadcast(Session* sender, uint16_t packetId, const void* data, uint16_t size);
unsigned int GetWorkerThreadCount();
bool StartWorkerThreads();