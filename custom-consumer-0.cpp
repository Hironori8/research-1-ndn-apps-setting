#include "custom-consumer-0.hpp"
#include "src/network/utils/packet-socket-factory.h"
#include <math.h>
#include "src/applications/helper/bulk-send-helper.h"
#include <iostream>
#include <fstream>
#include <string>

#define VERIFICATION_ATTACK_RATE 0.001
#define VERIFICATION_ATTACK_START 20.0
#define VERIFICATION_ATTACK_FINISH 40.0
#define USER_NUM 10
#define ATTACK_REQUEST1 0.8
#define ATTACK_REQUEST2 1.6
#define ATTACKER_NUM 2
NS_LOG_COMPONENT_DEFINE("CustomConsumer");
#define Verification_Attack_Rate 1

namespace ns3 {
namespace ndn {

		NS_OBJECT_ENSURE_REGISTERED(CustomConsumer);

		//define
		int Count_num[USER_NUM] = {};
		//example /verification/Count_num/Verification_num;
		int Verification_num[USER_NUM] = {};
		//example /verification/Count_num/Verification_num;
		double start[USER_NUM] = {};
		bool Verification_Attack_Stage1_Frag[USER_NUM] = {};
		bool Reset_Flag[USER_NUM] = {};

		TypeId CustomConsumer::GetTypeId(void){
				static TypeId tid =
						TypeId("CustomConsumer")
								.SetGroupName("Ndn")
								.SetParent<ConsumerCbr>()
								.AddConstructor<CustomConsumer>()

								.AddAttribute("NumberOfContents", "Number of the Contents in total", 
												StringValue("500"),
																						MakeUintegerAccessor(&CustomConsumer::SetNumberOfContents,
																																											&CustomConsumer::GetNumberOfContents),
																						MakeUintegerChecker<uint32_t>())

								.AddAttribute("q", "parameter of improve rank", 
												StringValue("0.7"),
																						MakeDoubleAccessor(&CustomConsumer::SetQ,
																																									&CustomConsumer::GetQ),
																						MakeDoubleChecker<double>())

								.AddAttribute("s", "parameter of power", 
												StringValue("0.7"),
																						MakeDoubleAccessor(&CustomConsumer::SetS,
																																									&CustomConsumer::GetS),
																						MakeDoubleChecker<double>());

				return tid;
		}

		CustomConsumer::CustomConsumer()
				: m_N(100)
						// needed here to make sure when SetQ/SetS are called, there is a valid value of N
				, m_q(0.7)
				, m_s(0.7)
				, m_seqRng(CreateObject<UniformRandomVariable>()){
				// SetNumberOfContents is called by NS-3 object system during the initialization
		}

		CustomConsumer::~CustomConsumer(){
		}

		void 
		CustomConsumer::SetNumberOfContents(uint32_t numOfContents){
				m_N = numOfContents;

				NS_LOG_DEBUG(m_q << " and " << m_s << " and " << m_N);
				m_Pcum = std::vector<double>(m_N + 1);
				m_Pcum[0] = 0.0;
				for (uint32_t i = 1; i <= m_N; i++) {
						m_Pcum[i] = m_Pcum[i - 1] + 1.0 / std::pow(i + m_q, m_s);
				}

				for (uint32_t i = 1; i <= m_N; i++) {
						m_Pcum[i] = m_Pcum[i] / m_Pcum[m_N];
				}
		}

		uint32_t 
		CustomConsumer::GetNumberOfContents() const {
				return m_N;
		}

		void 
		CustomConsumer::SetQ(double q) {
				m_q = q;
				SetNumberOfContents(m_N);
		}

		double 
		CustomConsumer::GetQ() const {
				return m_q;
		}

		void 
		CustomConsumer::SetS(double s) {
				m_s = s;
				SetNumberOfContents(m_N);
		}

		double 
		CustomConsumer::GetS() const {
				return m_s;
		}

		void 
		CustomConsumer::SendPacket() {
				if (!m_active)
						return;

				NS_LOG_FUNCTION_NOARGS();

				uint32_t seq = std::numeric_limits<uint32_t>::max(); 


				while (m_retxSeqs.size()) {
						seq = *m_retxSeqs.begin();
						m_retxSeqs.erase(m_retxSeqs.begin());
						
						break;
				}

				if (seq == std::numeric_limits<uint32_t>::max()) 
						// no retransmission
				{
						if (m_seqMax != std::numeric_limits<uint32_t>::max()) {
								if (m_seq >= m_seqMax) {
										return; // we are totally done
								}
						}

						seq = CustomConsumer::GetNextSeq();
						//seq =1;
						m_seq++;
				}
				
				shared_ptr<Name> nameWithSequence = 
						make_shared<Name>(m_interestName);
				nameWithSequence->appendSequenceNumber(seq);
				shared_ptr<Interest> interest = make_shared<Interest>();
				// interest->setNonce(m_rand->GetValue(0,
										// std::numeric_limits<uint32_t>::max()));
				interest->setNonce(GetNode()->GetId());
				interest->setName(*nameWithSequence);
				NS_LOG_INFO ("Requesting Interest: \n" << *interest);
				m_seqTimeouts.insert(SeqTimeout(seq, Simulator::Now()));
				m_seqFullDelay.insert(SeqTimeout(seq, Simulator::Now()));
				m_seqLastDelay.erase(seq);
				m_seqLastDelay.insert(SeqTimeout(seq, Simulator::Now()));

				m_seqRetxCounts[seq]++;
				// std::cout << Simulator::Now() << ":" << interest->getName() << std::endl;
				m_rtt->SentSeq(SequenceNumber32(seq), 1);
				m_transmittedInterests(interest, this, m_face);
				m_appLink->onReceiveInterest(*interest);
				CustomConsumer::ScheduleNextPacket();
		}

		void 
		CustomConsumer::SendPacket2() {
				if (!m_active)
						return;

				NS_LOG_FUNCTION_NOARGS();

				uint32_t seq = std::numeric_limits<uint32_t>::max(); 


				while (m_retxSeqs.size()) {
						seq = *m_retxSeqs.begin();
						m_retxSeqs.erase(m_retxSeqs.begin());
						
						break;
				}

				if (seq == std::numeric_limits<uint32_t>::max()) {
						if (m_seqMax != std::numeric_limits<uint32_t>::max()) {
								if (m_seq >= m_seqMax) {
										return; // we are totally done
								}
						}

						seq = CustomConsumer::GetNextSeq();
						//seq =1;
						m_seq++;
				}
				
				shared_ptr<Name> nameWithSequence = 
						make_shared<Name>(m_interestName);
				nameWithSequence->appendSequenceNumber(seq);
				shared_ptr<Interest> interest = make_shared<Interest>();
				// interest->setNonce(m_rand->GetValue(0,
										// std::numeric_limits<uint32_t>::max()));
				interest->setNonce(GetNode()->GetId());
				interest->setName(*nameWithSequence);
				NS_LOG_INFO ("Requesting Interest: \n" << *interest);
				m_seqTimeouts.insert(SeqTimeout(seq, Simulator::Now()));
				m_seqFullDelay.insert(SeqTimeout(seq, Simulator::Now()));
				m_seqLastDelay.erase(seq);
				m_seqLastDelay.insert(SeqTimeout(seq, Simulator::Now()));

				m_seqRetxCounts[seq]++;
				// std::cout << Simulator::Now() << ":" << interest->getName() << std::endl;
				m_rtt->SentSeq(SequenceNumber32(seq), 1);
				m_transmittedInterests(interest, this, m_face);
				m_appLink->onReceiveInterest(*interest);
				if(0<= GetNode()->GetId() <= USER_NUM){
						Verification_num[GetNode()->GetId()]++;
				}
				CustomConsumer::ScheduleNextPacket();
		}

		void 
		CustomConsumer::VerificationAttack(int user_num){
				if (!m_active)
						return;
				if(Reset_Flag[user_num] == false){
						start[user_num] = Simulator::Now().GetSeconds();
						Reset_Flag[user_num] = true;
				}
				NS_LOG_FUNCTION_NOARGS();

				std::string s;
				if(0 <= user_num && user_num <= USER_NUM){
						s = "/verification/" + std::to_string(user_num) + "/";
				}
				shared_ptr<Name> nameWithSequence = 
				make_shared<Name>(s);
				nameWithSequence->appendSequenceNumber(Count_num[user_num]);
				nameWithSequence->appendSequenceNumber(Verification_num[user_num]);
				Verification_num[user_num]++ ;

				if(Simulator::Now().GetSeconds() > start[user_num] + ATTACK_REQUEST1 
								&& Verification_Attack_Stage1_Frag[user_num] == false){
						Verification_num[user_num] = 0;
						Verification_Attack_Stage1_Frag[user_num] = true;
				}
				else if(Simulator::Now().GetSeconds() > start[user_num] + ATTACK_REQUEST2){
						Verification_num[user_num] = 0;
						Count_num[user_num]++;
						Reset_Flag[user_num] = false;
						Verification_Attack_Stage1_Frag[user_num] = false;
				}
				shared_ptr<Interest> interest = make_shared<Interest>();
				interest->setNonce(GetNode()->GetId());
				interest->setName(*nameWithSequence);
				m_transmittedInterests(interest, this, m_face);
				m_appLink->onReceiveInterest(*interest);
				CustomConsumer::ScheduleNextPacket();
		}


		uint32_t 
		CustomConsumer::GetNextSeq(){
				uint32_t content_index = 1; 
				double p_sum = 0;

				double p_random = m_seqRng->GetValue();
				while (p_random == 0) {
						p_random = m_seqRng->GetValue();
				}
				
				for (uint32_t i = 1; i <= m_N; i++) {
						p_sum = m_Pcum[i];

						if (p_random <= p_sum) {
								content_index = i;
								break;
						} // if
				}   // for
				return content_index;
		}

		void 
		CustomConsumer::ScheduleNextPacket(){

				if (m_firstTime) {
						m_sendEvent = Simulator::Schedule(Seconds(0.0), &CustomConsumer::SendPacket, this);
						m_firstTime = false;
				}else if (Simulator::Now().GetSeconds() > VERIFICATION_ATTACK_START 
								&& Simulator::Now().GetSeconds() < VERIFICATION_ATTACK_FINISH){  
						if(0 <= GetNode()->GetId() && GetNode()->GetId() < ATTACKER_NUM){
										m_sendEvent = Simulator::Schedule(Seconds(VERIFICATION_ATTACK_RATE)
														,&CustomConsumer::VerificationAttack, this, GetNode()->GetId());
						}else if (!m_sendEvent.IsRunning()){
								m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
																																																								: Seconds(m_random->GetValue()),
																																								&CustomConsumer::SendPacket, this);
						}
				}else if (!m_sendEvent.IsRunning()){
						m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
																																																								: Seconds(m_random->GetValue()),
																																								&CustomConsumer::SendPacket, this);
				}
		}

// namespace
}
}
