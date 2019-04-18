#include "custom-consumer-0.hpp"
#include "src/network/utils/packet-socket-factory.h"
#include <math.h>
#include "src/applications/helper/bulk-send-helper.h"
#include <iostream>
#include <fstream>
#include <string>

#define VERIFICATION_ATTACK_RATE 0.001
//攻撃のレート
#define VERIFICATION_ATTACK_START 20.0
//攻撃の開始時間
#define VERIFICATION_ATTACK_FINISH 40.0
//攻撃の終了時間
#define USER_NUM 10
//ユーザの数
#define ATTACK_REQUEST1 0.8
//未認証コンテンツ要求の終了タイミング
#define ATTACK_REQUEST2 1.6
//未認証コンテンツ再要求の終了タイミング
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
//攻撃の開始時間
bool Verification_Attack_Stage1_Frag[USER_NUM] = {};
//未認証コンテンツ挿入段階かどうかを判断するフラグ
bool Reset_Flag[USER_NUM] = {};
//再要求が終わったことを示すフラグ

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
          //要求するコンテンツの種類の数
                    MakeUintegerAccessor(&CustomConsumer::SetNumberOfContents,
                                         &CustomConsumer::GetNumberOfContents),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("q", "parameter of improve rank", 
          StringValue("0.7"),
          //q値の設定
                    MakeDoubleAccessor(&CustomConsumer::SetQ,
                                       &CustomConsumer::GetQ),
                    MakeDoubleChecker<double>())

      .AddAttribute("s", "parameter of power", 
          StringValue("0.7"),
          //s値の設定
                    MakeDoubleAccessor(&CustomConsumer::SetS,
                                       &CustomConsumer::GetS),
                    MakeDoubleChecker<double>());

  return tid;
}

CustomConsumer::CustomConsumer()
  //コンストラクタ
  : m_N(100)
    // needed here to make sure when SetQ/SetS are called, there is a valid value of N
  , m_q(0.7)
  , m_s(0.7)
  , m_seqRng(CreateObject<UniformRandomVariable>())
{
  // SetNumberOfContents is called by NS-3 object system during the initialization
}

CustomConsumer::~CustomConsumer()
  //コンストラクタを破壊
{
}

void
CustomConsumer::SetNumberOfContents(uint32_t numOfContents)
{
  m_N = numOfContents;

  NS_LOG_DEBUG(m_q << " and " << m_s << " and " << m_N);
  m_Pcum = std::vector<double>(m_N + 1);
  // m_N+1個のvector型を生成
  m_Pcum[0] = 0.0;
  for (uint32_t i = 1; i <= m_N; i++) {
    m_Pcum[i] = m_Pcum[i - 1] + 1.0 / std::pow(i + m_q, m_s);
    //m_Pcum[i-1]+1.0をi+m_qのm_s乗で割ったものをm_Pcum[i-1]に代入
  }

  for (uint32_t i = 1; i <= m_N; i++) {
    m_Pcum[i] = m_Pcum[i] / m_Pcum[m_N];
    //m_Pcum[i]をm_Pcum[m_N]で割る
  }
}
uint32_t
CustomConsumer::GetNumberOfContents() const
//コンテンツの種類を設定
{
  return m_N;
}
void
CustomConsumer::SetQ(double q)
//q値を設定
{
  m_q = q;
  SetNumberOfContents(m_N);
}
double
CustomConsumer::GetQ() const
//q値を取得
{
  return m_q;
}
void
CustomConsumer::SetS(double s)
//s値を設定
{
  m_s = s;
  SetNumberOfContents(m_N);
}
double
CustomConsumer::GetS() const
//s値を取得
{
  return m_s;
}
void
CustomConsumer::SendPacket()
//要求パケットを送信
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();
  //関数名をデバックで表示

  uint32_t seq = std::numeric_limits<uint32_t>::max(); 
  //unit32_t型の最大値を取得する


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
  
  //要求パケットを作成
  shared_ptr<Name> nameWithSequence = 
    make_shared<Name>(m_interestName);
  //要求パケットの名前を取得
  nameWithSequence->appendSequenceNumber(seq);
  //名前にランダムな数字を付加 
  shared_ptr<Interest> interest = make_shared<Interest>();
  // interest->setNonce(m_rand->GetValue(0,
        // std::numeric_limits<uint32_t>::max()));
  interest->setNonce(GetNode()->GetId());
  interest->setName(*nameWithSequence);
  NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  //要求パケットを送信
  m_seqTimeouts.insert(SeqTimeout(seq, Simulator::Now()));
  //seqとシミュレータの時間をm_seqTimeoutsに格納
  m_seqFullDelay.insert(SeqTimeout(seq, Simulator::Now()));
  //seqとシミュレータの時間をm_seqFullDelayに格納
  m_seqLastDelay.erase(seq);
  m_seqLastDelay.insert(SeqTimeout(seq, Simulator::Now()));
  //seqとシミュレータの時間をm_seqLastDelayに格納

  m_seqRetxCounts[seq]++;
  // std::cout << Simulator::Now() << ":" << interest->getName() << std::endl;
  m_rtt->SentSeq(SequenceNumber32(seq), 1);
  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);
  CustomConsumer::ScheduleNextPacket();
}

void
CustomConsumer::SendPacket2()
//要求パケットを送信
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();
  //関数名をデバックで表示

  uint32_t seq = std::numeric_limits<uint32_t>::max(); 
  //unit32_t型の最大値を取得する


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
  
  //要求パケットを作成
  shared_ptr<Name> nameWithSequence = 
    make_shared<Name>(m_interestName);
  //要求パケットの名前を取得
  nameWithSequence->appendSequenceNumber(seq);
  //名前にランダムな数字を付加 
  shared_ptr<Interest> interest = make_shared<Interest>();
  // interest->setNonce(m_rand->GetValue(0,
        // std::numeric_limits<uint32_t>::max()));
  interest->setNonce(GetNode()->GetId());
  interest->setName(*nameWithSequence);
  NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  //要求パケットを送信
  m_seqTimeouts.insert(SeqTimeout(seq, Simulator::Now()));
  //seqとシミュレータの時間をm_seqTimeoutsに格納
  m_seqFullDelay.insert(SeqTimeout(seq, Simulator::Now()));
  //seqとシミュレータの時間をm_seqFullDelayに格納
  m_seqLastDelay.erase(seq);
  m_seqLastDelay.insert(SeqTimeout(seq, Simulator::Now()));
  //seqとシミュレータの時間をm_seqLastDelayに格納

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
//要求パケットを送信
{
  if (!m_active)
    return;
  if(Reset_Flag[user_num] == false){
    start[user_num] = Simulator::Now().GetSeconds();
    Reset_Flag[user_num] = true;
  }
  NS_LOG_FUNCTION_NOARGS();
  //関数名をデバックで表示

  std::string s;
  //要求パケットを作成
  if(0 <= user_num && user_num <= USER_NUM){
    s = "/verification/" + std::to_string(user_num) + "/";
  }
  shared_ptr<Name> nameWithSequence = 
  make_shared<Name>(s);
  //要求パケットの名前を取得
  nameWithSequence->appendSequenceNumber(Count_num[user_num]);
  nameWithSequence->appendSequenceNumber(Verification_num[user_num]);
  //要求パケットに数字を付加
  Verification_num[user_num]++ ;

  if(Simulator::Now().GetSeconds() > start[user_num] + ATTACK_REQUEST1 
      && Verification_Attack_Stage1_Frag[user_num] == false){
    //未認証コンテンツの挿入が終了した場合
    Verification_num[user_num] = 0;
    Verification_Attack_Stage1_Frag[user_num] = true;
  }
  else if(Simulator::Now().GetSeconds() > start[user_num] + ATTACK_REQUEST2){
    //未認証コンテンツの再要求が終了した場合
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
  //未認証要求パケットを送信
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
//要求パケットを送るタイミングを設定
void
CustomConsumer::ScheduleNextPacket()
{

  if (m_firstTime) {
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &CustomConsumer::SendPacket, this);
    m_firstTime = false;
  }else if (Simulator::Now().GetSeconds() > VERIFICATION_ATTACK_START 
      && Simulator::Now().GetSeconds() < VERIFICATION_ATTACK_FINISH){  
    //20秒から30秒の間にVerification Attackを実行
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
}//ndnの名前空間の終わり
}//ns3の名前空間の終わり
