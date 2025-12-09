#include "GameServer.h"
#include "Protocol.h"
#include "Network.h"
#include "Packet.h"
#include "Session.h"
#include <string>



void Game_BroadcastChat(Session* sender, const std::string& msg)
{
	// WARNING: 현재는 Network 전역 접근
	// 추후 Network → Game 호출 구조로 변경 예정	
	
	std::string fullMsg = sender->name + ": " + msg;

	Network_Broadcast(sender, PKT_SC_CHAT, fullMsg.data(), (uint16_t)fullMsg.size());
}

void Game_settingNickName(Session* session, Packet& pkt)
{
	std::string msg = pkt.body;
	session->name = pkt.body;
}

void Game_OnChat(Session* session, Packet& pkt)
{
	// 여기서 드디어 게임 규칙 시작

	std::string msg = pkt.body;

	// 아직 Room 없으니까
	// "서버 정책"으로 전체 브로드캐스트
	Game_BroadcastChat(session, msg);
}

void Game_OnPacket(Session* session, Packet& pkt)
{
	// 여기서는
	// pkt.id는 유효
	// pkt.size는 안전
	// pkt.body는 완전한 메시지

	switch (pkt.header.id)
	{
	case PKT_CS_SETNAME:
		Game_settingNickName(session, pkt);
		break;
	case PKT_CS_CHAT:
		Game_OnChat(session, pkt);
		break;
	}
}

void Handle_CS_CHAT(Session* session, Packet& pkt)
{
	// body를 문자열로 해석 (널 종료 안 되어 있으니 길이 기반으로 처리)
	std::string msg(pkt.body, pkt.body + (pkt.header.size - sizeof(PacketHeader)));
	
	Game_BroadcastChat(session, msg);
}