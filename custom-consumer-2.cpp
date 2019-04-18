
#include "custom-consumer-2.hpp"
#include <math.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include "ns3/simulator.h"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
NS_LOG_COMPONENT_DEFINE("CustomConsumer2");

namespace ns3 {
namespace ndn {

// std::unordered_map<Name, double> InterestTimeTable;
// using InterestTimeSet = std::pair<Name, double>;
// std::ofstream outputfile("DataArriveTime.csv");

NS_OBJECT_ENSURE_REGISTERED(CustomConsumer2);

TypeId
CustomConsumer2::GetTypeId(void)
{
  static TypeId tid =
    TypeId("CustomConsumer2")
      .SetGroupName("Ndn")
      .SetParent<ConsumerCbr>()
      .AddConstructor<CustomConsumer2>()

      .AddAttribute("NumberOfContents", "Number of the Contents in total",
          StringValue("500"),
                    MakeUintegerAccessor(&CustomConsumer2::SetNumberOfContents,
                                         &CustomConsumer2::GetNumberOfContents),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("q", "parameter of improve rank",
          StringValue("0.7"),
                    MakeDoubleAccessor(&CustomConsumer2::SetQ,
                                       &CustomConsumer2::GetQ),
                    MakeDoubleChecker<double>())

      .AddAttribute("s", "parameter of power",
          StringValue("0.7"),
                    MakeDoubleAccessor(&CustomConsumer2::SetS,
                                       &CustomConsumer2::GetS),
                    MakeDoubleChecker<double>());

  return tid;
}

CustomConsumer2::CustomConsumer2()
  :m_N(100)
  , m_q(0.7)
  , m_s(0.7)
  , m_seqRng(CreateObject<UniformRandomVariable>())
{
}

CustomConsumer2::~CustomConsumer2()
{
}

void
CustomConsumer2::SetNumberOfContents(uint32_t numOfContents)
{
  m_N = numOfContents;

  NS_LOG_DEBUG(m_q << " and " << m_s << " and " << m_N);

  m_Pcum = std::vector<double>(m_N + 1);

  m_Pcum[0] = 0.0;
  for (uint32_t i = 1; i <= m_N; i++) {
    m_Pcum[i] = m_Pcum[i - 1] + 1.0 / std::pow(i + m_q, m_s);
  }

  for (uint32_t i = 1; i <= m_N; i++) {
    m_Pcum[i] = m_Pcum[i] / m_Pcum[m_N];
    NS_LOG_LOGIC("Cumulative probability [" << i << "]=" << m_Pcum[i]);
  }
}

uint32_t
CustomConsumer2::GetNumberOfContents() const
{
  return m_N;
}

void
CustomConsumer2::SetQ(double q)
{
  m_q = q;
  SetNumberOfContents(m_N);
}

double
CustomConsumer2::GetQ() const
{
  return m_q;
}

void
CustomConsumer2::SetS(double s)
{
  m_s = s;
  SetNumberOfContents(m_N);
}

double
CustomConsumer2::GetS() const
{
  return m_s;
}

void
CustomConsumer2::SendPacket()
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();

  uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid


  while (m_retxSeqs.size()) {
    seq = *m_retxSeqs.begin();
    m_retxSeqs.erase(m_retxSeqs.begin());

    NS_LOG_DEBUG("=interest seq " << seq << " from m_retxSeqs");
    break;
  }

  if (seq == std::numeric_limits<uint32_t>::max()) // no retransmission
  {
    if (m_seqMax != std::numeric_limits<uint32_t>::max()) {
      if (m_seq >= m_seqMax) {
        return; // we are totally done
      }
    }

    seq = CustomConsumer2::GetNextSeq();
    m_seq++;
  }


  shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
  nameWithSequence->appendSequenceNumber(seq);

  shared_ptr<Interest> interest = make_shared<Interest>();
  // interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setNonce(GetNode()->GetId());
  interest->setName(*nameWithSequence);
  // ndn::StackHelper::getKeyChain().sign(*interest);
  NS_LOG_INFO("> Interest for " << seq << ", Total: " << m_seq << ", face: " << m_face->getId());
  NS_LOG_DEBUG("Trying to add " << seq << " with " << Simulator::Now() << ". already "
                                << m_seqTimeouts.size() << " items");

  m_seqTimeouts.insert(SeqTimeout(seq, Simulator::Now()));
  m_seqFullDelay.insert(SeqTimeout(seq, Simulator::Now()));

  m_seqLastDelay.erase(seq);
  m_seqLastDelay.insert(SeqTimeout(seq, Simulator::Now()));

  m_seqRetxCounts[seq]++;

  m_rtt->SentSeq(SequenceNumber32(seq), 1);

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);

  // double time = ns3::Simulator::Now().GetSeconds();
  // outputfile << time << "OnInterest:" << interest->getName() << std::endl;
  // InterestTimeTable.insert(InterestTimeSet{interest->getName(),time});

  CustomConsumer2::ScheduleNextPacket();
}

uint32_t
CustomConsumer2::GetNextSeq()
{
  uint32_t content_index = 1; //[1, m_N]
  double p_sum = 0;

  double p_random = m_seqRng->GetValue();
  while (p_random == 0) {
    p_random = m_seqRng->GetValue();
  }
  // if (p_random == 0)
  NS_LOG_LOGIC("p_random=" << p_random);
  for (uint32_t i = 1; i <= m_N; i++) {
    p_sum = m_Pcum[i]; 

    if (p_random <= p_sum) {
      content_index = i;
      break;
    } // if
  }   // for
  // content_index = 1;
  NS_LOG_DEBUG("RandomNumber=" << content_index);
  return content_index;
}

void
CustomConsumer2::ScheduleNextPacket()
{

  if (m_firstTime) {
    m_sendEvent = Simulator::Schedule(Seconds(0.0),
        &CustomConsumer2::SendPacket, this);
    m_firstTime = false;
  }
  else if (!m_sendEvent.IsRunning())
    m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
                                                      : Seconds(m_random->GetValue()),
                                      &CustomConsumer2::SendPacket, this);
}
// void
// CustomConsumer2::OnData(std::shared_ptr<const ndn::Data> data)
// {
  // auto itr = InterestTimeTable.find(data->getName());
  // double ArriveTime = 
    // ns3::Simulator::Now().GetSeconds() - itr->second;
  // outputfile << "OnData" << data->getName() <<":"<< ArriveTime<< std::endl;
  // InterestTimeTable.erase(itr);
// }
} /* namespace ndn */
} /* namespace ns3 */
