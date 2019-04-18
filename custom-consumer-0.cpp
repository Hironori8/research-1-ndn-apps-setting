#include "custom-consumer-0.hpp"
#include "src/network/utils/packet-socket-factory.h"
#include <math.h>
#include "src/applications/helper/bulk-send-helper.h"
#include <iostream>
#include <fstream>
#include <string>

#define VERIFICATION_ATTACK_RATE 0.001
//$B967b$N%l!<%H(B
#define VERIFICATION_ATTACK_START 20.0
//$B967b$N3+;O;~4V(B
#define VERIFICATION_ATTACK_FINISH 40.0
//$B967b$N=*N;;~4V(B
#define USER_NUM 10
//$B%f!<%6$N?t(B
#define ATTACK_REQUEST1 0.8
//$BL$G'>Z%3%s%F%s%DMW5a$N=*N;%?%$%_%s%0(B
#define ATTACK_REQUEST2 1.6
//$BL$G'>Z%3%s%F%s%D:FMW5a$N=*N;%?%$%_%s%0(B
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
//$B967b$N3+;O;~4V(B
bool Verification_Attack_Stage1_Frag[USER_NUM] = {};
//$BL$G'>Z%3%s%F%s%DA^F~CJ3,$+$I$&$+$rH=CG$9$k%U%i%0(B
bool Reset_Flag[USER_NUM] = {};
//$B:FMW5a$,=*$o$C$?$3$H$r<($9%U%i%0(B

TypeId
CustomConsumer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("CustomConsumer")
      .SetGroupName("Ndn")
      .SetParent<ConsumerCbr>()
      .AddConstructor<CustomConsumer>()

      .AddAttribute("NumberOfContents", "Number of the Contents in total", 
          StringValue("500"),
          //$BMW5a$9$k%3%s%F%s%D$N<oN`$N?t(B
                    MakeUintegerAccessor(&CustomConsumer::SetNumberOfContents,
                                         &CustomConsumer::GetNumberOfContents),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("q", "parameter of improve rank", 
          StringValue("0.7"),
          //q$BCM$N@_Dj(B
                    MakeDoubleAccessor(&CustomConsumer::SetQ,
                                       &CustomConsumer::GetQ),
                    MakeDoubleChecker<double>())

      .AddAttribute("s", "parameter of power", 
          StringValue("0.7"),
          //s$BCM$N@_Dj(B
                    MakeDoubleAccessor(&CustomConsumer::SetS,
                                       &CustomConsumer::GetS),
                    MakeDoubleChecker<double>());

  return tid;
}

CustomConsumer::CustomConsumer()
  //$B%3%s%9%H%i%/%?(B
  : m_N(100)
    // needed here to make sure when SetQ/SetS are called, there is a valid value of N
  , m_q(0.7)
  , m_s(0.7)
  , m_seqRng(CreateObject<UniformRandomVariable>())
{
  // SetNumberOfContents is called by NS-3 object system during the initialization
}

CustomConsumer::~CustomConsumer()
  //$B%3%s%9%H%i%/%?$rGK2u(B
{
}

void
CustomConsumer::SetNumberOfContents(uint32_t numOfContents)
{
  m_N = numOfContents;

  NS_LOG_DEBUG(m_q << " and " << m_s << " and " << m_N);
  m_Pcum = std::vector<double>(m_N + 1);
  // m_N+1$B8D$N(Bvector$B7?$r@8@.(B
  m_Pcum[0] = 0.0;
  for (uint32_t i = 1; i <= m_N; i++) {
    m_Pcum[i] = m_Pcum[i - 1] + 1.0 / std::pow(i + m_q, m_s);
    //m_Pcum[i-1]+1.0$B$r(Bi+m_q$B$N(Bm_s$B>h$G3d$C$?$b$N$r(Bm_Pcum[i-1]$B$KBeF~(B
  }

  for (uint32_t i = 1; i <= m_N; i++) {
    m_Pcum[i] = m_Pcum[i] / m_Pcum[m_N];
    //m_Pcum[i]$B$r(Bm_Pcum[m_N]$B$G3d$k(B
  }
}
uint32_t
CustomConsumer::GetNumberOfContents() const
//$B%3%s%F%s%D$N<oN`$r@_Dj(B
{
  return m_N;
}
void
CustomConsumer::SetQ(double q)
//q$BCM$r@_Dj(B
{
  m_q = q;
  SetNumberOfContents(m_N);
}
double
CustomConsumer::GetQ() const
//q$BCM$r<hF@(B
{
  return m_q;
}
void
CustomConsumer::SetS(double s)
//s$BCM$r@_Dj(B
{
  m_s = s;
  SetNumberOfContents(m_N);
}
double
CustomConsumer::GetS() const
//s$BCM$r<hF@(B
{
  return m_s;
}
void
CustomConsumer::SendPacket()
//$BMW5a%Q%1%C%H$rAw?.(B
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();
  //$B4X?tL>$r%G%P%C%/$GI=<((B

  uint32_t seq = std::numeric_limits<uint32_t>::max(); 
  //unit32_t$B7?$N:GBgCM$r<hF@$9$k(B


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
  
  //$BMW5a%Q%1%C%H$r:n@.(B
  shared_ptr<Name> nameWithSequence = 
    make_shared<Name>(m_interestName);
  //$BMW5a%Q%1%C%H$NL>A0$r<hF@(B
  nameWithSequence->appendSequenceNumber(seq);
  //$BL>A0$K%i%s%@%`$J?t;z$rIU2C(B 
  shared_ptr<Interest> interest = make_shared<Interest>();
  // interest->setNonce(m_rand->GetValue(0,
        // std::numeric_limits<uint32_t>::max()));
  interest->setNonce(GetNode()->GetId());
  interest->setName(*nameWithSequence);
  NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  //$BMW5a%Q%1%C%H$rAw?.(B
  m_seqTimeouts.insert(SeqTimeout(seq, Simulator::Now()));
  //seq$B$H%7%_%e%l!<%?$N;~4V$r(Bm_seqTimeouts$B$K3JG<(B
  m_seqFullDelay.insert(SeqTimeout(seq, Simulator::Now()));
  //seq$B$H%7%_%e%l!<%?$N;~4V$r(Bm_seqFullDelay$B$K3JG<(B
  m_seqLastDelay.erase(seq);
  m_seqLastDelay.insert(SeqTimeout(seq, Simulator::Now()));
  //seq$B$H%7%_%e%l!<%?$N;~4V$r(Bm_seqLastDelay$B$K3JG<(B

  m_seqRetxCounts[seq]++;
  // std::cout << Simulator::Now() << ":" << interest->getName() << std::endl;
  m_rtt->SentSeq(SequenceNumber32(seq), 1);
  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);
  CustomConsumer::ScheduleNextPacket();
}

void
CustomConsumer::SendPacket2()
//$BMW5a%Q%1%C%H$rAw?.(B
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();
  //$B4X?tL>$r%G%P%C%/$GI=<((B

  uint32_t seq = std::numeric_limits<uint32_t>::max(); 
  //unit32_t$B7?$N:GBgCM$r<hF@$9$k(B


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
  
  //$BMW5a%Q%1%C%H$r:n@.(B
  shared_ptr<Name> nameWithSequence = 
    make_shared<Name>(m_interestName);
  //$BMW5a%Q%1%C%H$NL>A0$r<hF@(B
  nameWithSequence->appendSequenceNumber(seq);
  //$BL>A0$K%i%s%@%`$J?t;z$rIU2C(B 
  shared_ptr<Interest> interest = make_shared<Interest>();
  // interest->setNonce(m_rand->GetValue(0,
        // std::numeric_limits<uint32_t>::max()));
  interest->setNonce(GetNode()->GetId());
  interest->setName(*nameWithSequence);
  NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  //$BMW5a%Q%1%C%H$rAw?.(B
  m_seqTimeouts.insert(SeqTimeout(seq, Simulator::Now()));
  //seq$B$H%7%_%e%l!<%?$N;~4V$r(Bm_seqTimeouts$B$K3JG<(B
  m_seqFullDelay.insert(SeqTimeout(seq, Simulator::Now()));
  //seq$B$H%7%_%e%l!<%?$N;~4V$r(Bm_seqFullDelay$B$K3JG<(B
  m_seqLastDelay.erase(seq);
  m_seqLastDelay.insert(SeqTimeout(seq, Simulator::Now()));
  //seq$B$H%7%_%e%l!<%?$N;~4V$r(Bm_seqLastDelay$B$K3JG<(B

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
CustomConsumer::VerificationAttack(int user_num)
//$BMW5a%Q%1%C%H$rAw?.(B
{
  if (!m_active)
    return;
  if(Reset_Flag[user_num] == false){
    start[user_num] = Simulator::Now().GetSeconds();
    Reset_Flag[user_num] = true;
  }
  NS_LOG_FUNCTION_NOARGS();
  //$B4X?tL>$r%G%P%C%/$GI=<((B

  std::string s;
  //$BMW5a%Q%1%C%H$r:n@.(B
  if(0 <= user_num && user_num <= USER_NUM){
    s = "/verification/" + std::to_string(user_num) + "/";
  }
  shared_ptr<Name> nameWithSequence = 
  make_shared<Name>(s);
  //$BMW5a%Q%1%C%H$NL>A0$r<hF@(B
  nameWithSequence->appendSequenceNumber(Count_num[user_num]);
  nameWithSequence->appendSequenceNumber(Verification_num[user_num]);
  //$BMW5a%Q%1%C%H$K?t;z$rIU2C(B
  Verification_num[user_num]++ ;

  if(Simulator::Now().GetSeconds() > start[user_num] + ATTACK_REQUEST1 
      && Verification_Attack_Stage1_Frag[user_num] == false){
    //$BL$G'>Z%3%s%F%s%D$NA^F~$,=*N;$7$?>l9g(B
    Verification_num[user_num] = 0;
    Verification_Attack_Stage1_Frag[user_num] = true;
  }
  else if(Simulator::Now().GetSeconds() > start[user_num] + ATTACK_REQUEST2){
    //$BL$G'>Z%3%s%F%s%D$N:FMW5a$,=*N;$7$?>l9g(B
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
  //$BL$G'>ZMW5a%Q%1%C%H$rAw?.(B
  CustomConsumer::ScheduleNextPacket();
}


uint32_t
CustomConsumer::GetNextSeq()
{
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
//$BMW5a%Q%1%C%H$rAw$k%?%$%_%s%0$r@_Dj(B
void
CustomConsumer::ScheduleNextPacket()
{

  if (m_firstTime) {
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &CustomConsumer::SendPacket, this);
    m_firstTime = false;
  }else if (Simulator::Now().GetSeconds() > VERIFICATION_ATTACK_START 
      && Simulator::Now().GetSeconds() < VERIFICATION_ATTACK_FINISH){  
    //20$BIC$+$i(B30$BIC$N4V$K(BVerification Attack$B$r<B9T(B
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
}//ndn$B$NL>A06u4V$N=*$o$j(B
}//ns3$B$NL>A06u4V$N=*$o$j(B
